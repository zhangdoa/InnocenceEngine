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

// Soft Shadow Tuning Parameters.
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

// PCSS Soft Shadows. Early-outs when no blockers were seen in the search
// (fragment is lit — skip the 16-tap PCF). The (sinA, cosA) rotation is
// applied to the same Poisson disk used for blocker search, so the two
// passes share a coherent per-pixel pattern.
float PCSS(float3 projCoords, Texture2DArray shadowMap, SamplerState in_sampler, int shadowMapIndex, float currentDepth, float2 texelSize, float shadowBias, float sinA, float cosA)
{
	int blockerCount = 0;
	float blockerDepth = FindBlockerDepth(projCoords, shadowMap, in_sampler, shadowMapIndex, currentDepth, texelSize, sinA, cosA, blockerCount);

	if (blockerCount == 0)
		return 0.0;

	float penumbraSize = ComputePenumbraSize(currentDepth, blockerDepth);

	float shadow = 0.0;
	const int filterSamples = 16;

	for (int i = 0; i < filterSamples; ++i)
	{
		float2 offset = RotateOffset(PoissonDisk[i], sinA, cosA) * texelSize * penumbraSize;
		float3 coord = float3(projCoords.xy + offset, shadowMapIndex);
		float depthSample = shadowMap.SampleLevel(in_sampler, coord, 0).r;

		shadow += (currentDepth - shadowBias > depthSample) ? 1.0 : 0.0;
	}

	return shadow / filterSamples;
}

// Sun Shadow Resolver (CSM Support + PCSS). screenCoord is the compute-shader
// thread index; used only as a stable seed for the per-pixel Poisson rotation.
float SunShadowResolver(float3 positionWS, float3 normalWS, Texture2DArray shadowMap, SamplerState in_sampler, float3 lightDir, uint2 screenCoord)
{
	int splitIndex = NR_CSM_SPLITS;
	[unroll]
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (positionWS.x >= CSMs[i].AABBMin.x &&
			positionWS.y >= CSMs[i].AABBMin.y &&
			positionWS.z >= CSMs[i].AABBMin.z &&
			positionWS.x <= CSMs[i].AABBMax.x &&
			positionWS.y <= CSMs[i].AABBMax.y &&
			positionWS.z <= CSMs[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	if (splitIndex == NR_CSM_SPLITS)
		return 0.0;

	float2 shadowMapSize;
	float level;
	float elements;
	shadowMap.GetDimensions(0, shadowMapSize.x, shadowMapSize.y, elements, level);
	float2 texelSize = 1.0 / shadowMapSize;

	float4 lightSpacePos = mul(float4(positionWS, 1.0f), CSMs[splitIndex].v);
	lightSpacePos = mul(lightSpacePos, CSMs[splitIndex].p);
	// Don't need perspective divide for othrographic projection
	
	float3 projCoords = lightSpacePos.xyz;
	if (projCoords.x > 1.0 || projCoords.x < -1.0 || projCoords.y > 1.0 || projCoords.y < -1.0 || projCoords.z > 1.0 || projCoords.z < 0.0)
	{
		return 0.0;
	}

	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	projCoords.y = 1.0 - projCoords.y;

	float currentDepth = projCoords.z;

	// Compute adaptive shadow bias to prevent light leaks
	float shadowBias = ComputeShadowBias(normalWS, lightDir);

	// Per-pixel rotation of the Poisson kernel. Both blocker-search and PCF
	// loops share the same (sinA, cosA) so a given pixel samples a coherent
	// rotated pattern; across pixels the pattern varies and hides the
	// 16-tap dot signature.
	float angle = ShadowKernelRotationAngle(float2(screenCoord));
	float sinA, cosA;
	sincos(angle, sinA, cosA);

	return PCSS(projCoords, shadowMap, in_sampler, splitIndex, currentDepth, texelSize, shadowBias, sinA, cosA);
}
