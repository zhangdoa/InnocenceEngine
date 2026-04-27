// shadertype=hlsl
//
// LightPass direct-lighting evaluators: sun + tiled point lights.
// Each evaluator writes into the (direct, indirect-seed) accumulator
// pair via AccumulateLightContribution from lightPassCommon.hlsl.
//
// Sphere area lights and volumetric fog used to live alongside these
// in the kernel but were never wired up; if/when re-enabled they
// belong here as additional Evaluate*Lighting() functions, not as
// commented-out blocks in lightPass.comp.

#ifndef LIGHTPASS_DIRECT_LIGHTING_HLSL
#define LIGHTPASS_DIRECT_LIGHTING_HLSL

// Sun light. Constructs the directional light vector with sun-disc
// clamping (when V points inside the sun disc, L collapses to V), runs
// the BSDF accumulator, then attenuates by CSM PCSS shadow.
//
// io_DirectLuminance and io_IndirectSeedLuminance are accumulators —
// the function adds the sun's contribution, it does not overwrite.
void EvaluateSunLighting(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in Texture2DArray in_SunShadow,
	in SamplerState in_LinearSampler,
	in MaterialAttributes in_Material,
	in float3 in_PositionWS,
	in float3 in_NormalWS,
	in float3 in_V,
	in uint2 in_ScreenCoord,
	inout float3 io_DirectLuminance,
	inout float3 io_IndirectSeedLuminance)
{
	// Sun-disc clamp: when the view ray lands inside the sun disc, treat
	// the light direction as V (the sun fills the BRDF lobe). Otherwise
	// take the sun direction tilted toward V by the disc radius.
	float3 D = normalize(g_Frame.sun_direction.xyz);
	float r = sin(SUN_ANGULAR_RADIUS);
	float d = cos(SUN_ANGULAR_RADIUS);
	float DdotV = dot(D, in_V);
	float3 S = in_V - DdotV * D;
	float3 L = DdotV < d ? normalize(d * D + normalize(S) * r) : in_V;

	float3 l_SunDirect = float3(0.0, 0.0, 0.0);
	float3 l_SunIndirectSeed = float3(0.0, 0.0, 0.0);
	AccumulateLightContribution(
		in_BRDFLUT, in_BRDFMSLUT, in_PointSampler,
		in_Material, in_V, in_NormalWS, L,
		g_Frame.sun_illuminance.xyz, 1.0,
		l_SunDirect, l_SunIndirectSeed);

	float l_ShadowFactor = SunShadowResolver(in_PositionWS, in_NormalWS, in_SunShadow, in_LinearSampler, L, in_ScreenCoord);
	float l_Visibility = 1.0 - l_ShadowFactor;
	io_DirectLuminance += l_SunDirect * l_Visibility;
	io_IndirectSeedLuminance += l_SunIndirectSeed * l_Visibility;
}

// Tiled point lights. Looks the visible-light list for this tile out of
// the LightCullingPass grid (LIGHT_CULLING_BLOCK_SIZE-pixel tiles), then
// runs the BSDF accumulator for each. Per-light visibility comes from the
// cube-shadow resolver (TASK-148): an INVALID_ATLAS_SLOT in
// PointLight_CB::position.w means the light is non-shadow-casting (or was
// rejected by the per-frame atlas budget) and the resolver returns 0.
//
// Convention parity with sun shadow: PointShadowResolver returns shadow ∈ [0,1]
// where 1 = fully shadowed, the consumer applies Visibility = 1 - shadow. See
// feedback_verify_source_before_chasing.md / TASK-145 hypothesis 2 for why
// the convention must match.
void EvaluateTiledPointLighting(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in Texture2D<uint2> in_LightGrid,
	in StructuredBuffer<uint> in_LightIndexList,
	in Texture2DArray in_PointShadow,
	in SamplerState in_LinearSampler,
	in MaterialAttributes in_Material,
	in float3 in_PositionWS,
	in float3 in_NormalWS,
	in float3 in_V,
	in uint2 in_ScreenCoord,
	inout float3 io_DirectLuminance,
	inout float3 io_IndirectSeedLuminance)
{
	uint2 l_TileIndex = uint2(floor((float2)in_ScreenCoord / LIGHT_CULLING_BLOCK_SIZE));
	uint l_StartOffset = in_LightGrid[l_TileIndex].x;
	uint l_LightCount = in_LightGrid[l_TileIndex].y;

	[loop]
	for (uint i = 0; i < l_LightCount; ++i)
	{
		uint l_LightIndex = in_LightIndexList[l_StartOffset + i];
		PointLight_CB l_PointLight = g_PointLights[l_LightIndex];

		float3 L_unnormalized = l_PointLight.position.xyz - in_PositionWS;
		float l_AttenuationRadius = l_PointLight.luminousFlux.w;
		float3 L = normalize(L_unnormalized);

		float l_InvSquareRadius = 1.0 / max(l_AttenuationRadius * l_AttenuationRadius, EPSILON);
		float l_AttenuationFactor = CalculateDistanceAttenuation(L_unnormalized, l_InvSquareRadius);

		float3 l_LightDirect = float3(0.0, 0.0, 0.0);
		float3 l_LightIndirectSeed = float3(0.0, 0.0, 0.0);
		AccumulateLightContribution(
			in_BRDFLUT, in_BRDFMSLUT, in_PointSampler,
			in_Material, in_V, in_NormalWS, L,
			l_PointLight.luminousFlux.xyz, l_AttenuationFactor,
			l_LightDirect, l_LightIndirectSeed);

		// Shadow term — slot stamped on PointLight_CB::position.w by
		// LightDataService_PointShadow.inl as a uint reinterpreted to float
		// (asuint() recovers the uint). Sentinel == INVALID_ATLAS_SLOT short-
		// circuits the resolver before any atlas sample.
		uint l_ShadowSlot = asuint(l_PointLight.position.w);
		float l_ShadowFactor = 0.0;
		if (l_ShadowSlot != INVALID_ATLAS_SLOT)
		{
			PointShadow_CB l_ShadowCB = g_PointShadows[l_ShadowSlot];
			l_ShadowFactor = PointShadowResolver(in_PositionWS, in_NormalWS, in_PointShadow, in_LinearSampler, l_ShadowSlot, l_ShadowCB, in_ScreenCoord);
		}
		float l_Visibility = 1.0 - l_ShadowFactor;
		io_DirectLuminance += l_LightDirect * l_Visibility;
		io_IndirectSeedLuminance += l_LightIndirectSeed * l_Visibility;
	}
}

#endif // LIGHTPASS_DIRECT_LIGHTING_HLSL
