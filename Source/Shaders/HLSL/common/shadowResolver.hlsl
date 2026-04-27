// Poisson Disk Sample Pattern (16 optimized offsets)
static const float2 PoissonDisk[16] =
{
	float2(-0.94201624, -0.39906216),
	float2(0.94558609, -0.76890725),
	float2(-0.094184101, -0.92938870),
	float2(0.34495938, 0.29387760),
	float2(-0.91588581, 0.45771432),
	float2(-0.81544232, -0.87912464),
	float2(-0.38277543, 0.27676845),
	float2(0.97484398, 0.75648379),
	float2(0.44323325, -0.97511554),
	float2(0.53742981, -0.47373420),
	float2(-0.26496911, -0.41893023),
	float2(0.79197514, 0.19090188),
	float2(-0.24188840, 0.99706507),
	float2(-0.81409955, 0.91437590),
	float2(0.19984126, 0.78641367),
	float2(0.14383161, -0.14100790)
};

// PCSS tuning for the cube-shadow path (sun shadows now go through
// SunShadowRTPass — no PCSS evaluator on that path).
// LIGHT_SIZE scales (receiverDepth - blockerDepth) → penumbra-size in texels.
// PENUMBRA_MAX_TEXELS bounds the PCF kernel radius so near-camera blockers
// don't explode the sample area.
#define LIGHT_SIZE 2.0
#define PENUMBRA_MAX_TEXELS 20.0
#define MIN_SHADOW_BIAS 0.0001
#define MAX_SHADOW_BIAS 0.0003

// Interleaved Gradient Noise — Jorge Jimenez, Next Generation Post Processing
// in Call of Duty: Advanced Warfare (SIGGRAPH 2014). Returns angle in radians.
// Per-pixel rotation of the Poisson kernel hides the fixed dot pattern that
// makes a naive PCF look aliased.
float ShadowKernelRotationAngle(float2 pixelCoord)
{
	float n = frac(52.9829189 * frac(dot(pixelCoord, float2(0.06711056, 0.00583715))));
	return n * 6.28318530718; // 2*pi
}

float2 RotateOffset(float2 offset, float sinA, float cosA)
{
	return float2(offset.x * cosA - offset.y * sinA,
				  offset.x * sinA + offset.y * cosA);
}

// Compute Adaptive Shadow Bias (Prevents Light Leaks)
float ComputeShadowBias(float3 normalWS, float3 lightDir)
{
	float cosTheta = saturate(dot(normalWS, lightDir));
	return lerp(MAX_SHADOW_BIAS, MIN_SHADOW_BIAS, cosTheta);
}

// Blocker-search radius, in texels. Larger radius finds more occluders
// and produces larger penumbras; smaller radius is cheaper and keeps
// shadow contacts crisp. Ramps mildly with depth so near-camera contact
// shadows stay tight and far shadows get broader sampling.
float GetBlockerSearchSize(float receiverDepth)
{
	return lerp(4.0, 12.0, saturate(receiverDepth));
}

// Find Average Blocker Depth in Shadow Map. Returns 1.0 when no occluders
// were found so the caller can early-out and skip the PCF loop entirely.
// The rotation (sinA, cosA) is a per-pixel twist of the Poisson kernel
// shared with the PCF pass.
float FindBlockerDepth(float3 projCoords, Texture2DArray shadowMap, SamplerState in_sampler, int shadowMapIndex, float currentDepth, float2 texelSize, float sinA, float cosA, out int outBlockerCount)
{
	int blockerCount = 0;
	float totalBlockerDepth = 0.0;

	float searchSize = GetBlockerSearchSize(currentDepth);

	const int searchSamples = 16;
	for (int i = 0; i < searchSamples; ++i)
	{
		float2 offset = RotateOffset(PoissonDisk[i], sinA, cosA) * texelSize * searchSize;
		float3 coord = float3(projCoords.xy + offset, shadowMapIndex);
		float depthSample = shadowMap.SampleLevel(in_sampler, coord, 0).r;

		if (depthSample < currentDepth)
		{
			totalBlockerDepth += depthSample;
			blockerCount++;
		}
	}

	outBlockerCount = blockerCount;
	if (blockerCount == 0)
		return 1.0;

	return totalBlockerDepth / blockerCount;
}

// Compute Penumbra Size (Soft Shadow Spread), in texels.
float ComputePenumbraSize(float receiverDepth, float blockerDepth)
{
	return clamp((receiverDepth - blockerDepth) * LIGHT_SIZE, 0.0f, PENUMBRA_MAX_TEXELS);
}

// Cube-face selection for an omnidirectional shadow map. Returns face index
// 0..5 = +X, -X, +Y, -Y, +Z, -Z. The face whose forward axis dominates the
// input world-space direction wins. Matches the look-at orientation in
// LightDataService_PointShadow.inl (face 0 = +X looks toward +X, etc.).
//
// UV is computed below by re-running the caster's per-face view + projection
// matrices on the receiver position — that avoids re-deriving the per-face
// (s, t) basis here and drifting from the caster, which is the class of bug
// feedback_verify_source_before_chasing.md warns against.
int CubeFaceFromDirection(float3 dir)
{
	float3 a = abs(dir);
	if (a.x >= a.y && a.x >= a.z) return dir.x > 0 ? 0 : 1;
	if (a.y >= a.z)               return dir.y > 0 ? 2 : 3;
	return dir.z > 0 ? 4 : 5;
}

// Cube-shadow PCSS. shadowMapIndex is `atlasBaseSlot + face`. Returns shadow
// ∈ [0,1] where 1 = fully shadowed; caller computes Visibility = 1 - shadow.
float PointPCSS(float2 faceUV, Texture2DArray shadowMap, SamplerState in_sampler, int shadowMapIndex, float currentDepth, float2 texelSize, float shadowBias, float sinA, float cosA)
{
	float3 projCoords = float3(faceUV, 0.0f);
	int blockerCount = 0;
	float blockerDepth = FindBlockerDepth(projCoords, shadowMap, in_sampler, shadowMapIndex, currentDepth, texelSize, sinA, cosA, blockerCount);
	if (blockerCount == 0)
		return 0.0f;

	float penumbraSize = ComputePenumbraSize(currentDepth, blockerDepth);

	float shadow = 0.0f;
	const int filterSamples = 16;
	for (int i = 0; i < filterSamples; ++i)
	{
		float2 offset = RotateOffset(PoissonDisk[i], sinA, cosA) * texelSize * penumbraSize;
		float3 coord = float3(faceUV + offset, shadowMapIndex);
		float depthSample = shadowMap.SampleLevel(in_sampler, coord, 0).r;
		shadow += (currentDepth - shadowBias > depthSample) ? 1.0f : 0.0f;
	}
	return shadow / filterSamples;
}

// Cube-shadow resolver. Returns shadow ∈ [0,1] where 1 = fully shadowed;
// caller computes Visibility = 1 - shadow.
//
// lightSlot is the per-light slot the LightDataService allocator stamped on
// PointLight_CB / SphereLight_CB. INVALID_ATLAS_SLOT means the light is
// non-shadow-casting (m_CastShadow=false) or exceeded the per-frame budget;
// the resolver returns 0 (lit) immediately.
//
// UV computation: re-applies the caster's per-face view × projection on the
// receiver position. This guarantees the resolver hits exactly the texel the
// caster wrote, regardless of the per-face axis convention chosen in
// LightDataService_PointShadow.inl.
float PointShadowResolver(float3 positionWS, float3 normalWS, Texture2DArray shadowMap, SamplerState in_sampler, uint lightSlot, PointShadow_CB lightCB, uint2 screenCoord)
{
	if (lightSlot == INVALID_ATLAS_SLOT || lightCB.isActive == 0)
		return 0.0f;

	float3 lightPos = lightCB.lightPosWS_range.xyz;
	float range = lightCB.lightPosWS_range.w;
	if (range <= 0.0f)
		return 0.0f;

	// Direction = receiver - light. linearDist matches the caster's PS output
	// (linearDist = length(posWS - lightPos) / range). Range comparison metric
	// is unit-domain so the resolver is identical for all cube faces.
	float3 dir = positionWS - lightPos;
	float distance = length(dir);
	float currentDepth = distance / range;
	if (currentDepth >= 1.0f)
		return 0.0f;

	int face = CubeFaceFromDirection(dir);
	int shadowMapIndex = (int)lightCB.atlasBaseSlot + face;

	// Re-run caster's transform on receiver position to recover the same UV
	// the rasterizer wrote. mul(row-vector, row-major matrix) order — same
	// convention as the GS (pointShadowGeometryProcessPass.geom).
	float4 posV = mul(float4(positionWS, 1.0f), lightCB.v[face]);
	float4 posCS = mul(posV, lightCB.p);
	posCS /= posCS.w;
	// NDC → texture UV: x maps directly (UV.x ∈ [0,1] left-right matches
	// NDC.x ∈ [-1,1]); y flips because texture origin is top-left while
	// NDC.y ∈ [-1,1] is bottom-up.
	float2 faceUV = float2(posCS.x * 0.5f + 0.5f, 1.0f - (posCS.y * 0.5f + 0.5f));

	float2 shadowMapSize;
	float level, elements;
	shadowMap.GetDimensions(0, shadowMapSize.x, shadowMapSize.y, elements, level);
	float2 texelSize = 1.0f / shadowMapSize;

	float shadowBias = ComputeShadowBias(normalWS, normalize(-dir));

	float angle = ShadowKernelRotationAngle(float2(screenCoord));
	float sinA, cosA;
	sincos(angle, sinA, cosA);

	return PointPCSS(faceUV, shadowMap, in_sampler, shadowMapIndex, currentDepth, texelSize, shadowBias, sinA, cosA);
}
