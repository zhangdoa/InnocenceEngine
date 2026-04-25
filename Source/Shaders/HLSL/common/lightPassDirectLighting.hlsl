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
// runs the BSDF accumulator for each. Point lights have no shadow
// implementation today (visibility = 1).
void EvaluateTiledPointLighting(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in Texture2D<uint2> in_LightGrid,
	in StructuredBuffer<uint> in_LightIndexList,
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

		// @TODO: implement shadow mapping for point lights — visibility = 1 today.
		io_DirectLuminance += l_LightDirect;
		io_IndirectSeedLuminance += l_LightIndirectSeed;
	}
}

#endif // LIGHTPASS_DIRECT_LIGHTING_HLSL
