// shadertype=hlsl
//
// LightPass shared structures and BSDF accumulator helper.
// Used by lightPassDirectLighting.hlsl and lightPass.comp.
//
// The accumulator is named LightPass-specific because it returns the
// (direct, indirect-seed) pair LightPass needs for its two RTs. The
// generic single-return BSDF helpers live in common/BSDF.hlsl.

#ifndef LIGHTPASS_COMMON_HLSL
#define LIGHTPASS_COMMON_HLSL

// TASK-183 runtime debug-view sentinel values. Values greater than the max
// signal the lit-composite path should still be the LightPass output even
// when GI / direct contributions are individually inspected — DirectOnly
// and IndirectOnly are *also* lit composites, just with one term zeroed.
// DEBUG_VIEW_PIXEL_NEEDS_RAW_OUTPUT applies to the GBuffer / shadow / heatmap
// modes that must override RT0 with display-referred raw data.
//
// Returns true when the picked debug mode replaces RT0 with raw channel data
// (bypasses the BSDF accumulators) rather than modulating direct vs indirect
// terms inside the lit composite.
bool DebugViewModeReplacesRT0(uint in_DebugViewMode)
{
	return in_DebugViewMode >= DEBUG_VIEW_GBUFFER_ALBEDO;
}

struct MaterialAttributes
{
	float3 m_Albedo;
	float m_Metallic;
	float m_Roughness;
};

// Decode the four GBuffer attachments into world-space position/normal +
// material attributes. Returns false when the pixel has no geometry
// (RT0.a == 0), in which case the caller must skip lighting.
bool DecodeGBuffer(
	in Texture2D in_GBufferRT0,
	in Texture2D in_GBufferRT1,
	in Texture2D in_GBufferRT2,
	in uint2 in_ScreenCoord,
	out float3 out_PositionWS,
	out float3 out_NormalWS,
	out MaterialAttributes out_Material)
{
	float4 l_RT0 = in_GBufferRT0[in_ScreenCoord];

	out_PositionWS = float3(0.0, 0.0, 0.0);
	out_NormalWS = float3(0.0, 1.0, 0.0);
	out_Material.m_Albedo = float3(0.0, 0.0, 0.0);
	out_Material.m_Metallic = 0.0;
	out_Material.m_Roughness = 1.0;

	if (l_RT0.a == 0.0)
		return false;

	float4 l_RT1 = in_GBufferRT1[in_ScreenCoord];
	float4 l_RT2 = in_GBufferRT2[in_ScreenCoord];

	out_PositionWS = l_RT0.xyz;
	out_NormalWS = normalize(l_RT1.xyz);
	out_Material.m_Metallic = l_RT1.w;
	out_Material.m_Roughness = l_RT2.w;
	out_Material.m_Albedo = l_RT2.xyz;
	return true;
}

// Per-light BSDF accumulator. Returns the BSDF * illuminance for this
// light, split into the visual contribution (direct, full BSDF) and the
// indirect-seed contribution (Lambertian only, fed to next-frame ray
// hits via the illuminance RT).
//
// The two outputs share the same NdotL * luminousFlux * attenuation
// illuminance; only the BSDF differs (full vs. Lambertian).
void AccumulateLightContribution(
	in Texture2D in_BRDFLUT,
	in Texture2D in_BRDFMSLUT,
	in SamplerState in_PointSampler,
	in MaterialAttributes in_Material,
	in float3 in_V,
	in float3 in_N,
	in float3 in_L,
	in float3 in_LuminousFlux,
	in float in_AttenuationFactor,
	out float3 out_DirectLuminance,
	out float3 out_IndirectSeedLuminance)
{
	float NdotV = max(dot(in_N, in_V), 0.0);
	float NdotL = max(dot(in_N, in_L), 0.0);
	float3 HL = normalize(in_V + in_L);
	float LdotHL = max(dot(in_L, HL), 0.0);
	float NdotHL = max(dot(in_N, HL), 0.0);

	// Schlick Fresnel with F0 lerped from dielectric to albedo by metallic.
	float3 F0 = float3(F0_DIELECTRIC, F0_DIELECTRIC, F0_DIELECTRIC);
	F0 = lerp(F0, in_Material.m_Albedo, in_Material.m_Metallic);
	float F90 = 1.0;
	float3 l_FresnelTerm = Fresnel_Schlick(F0, F90, LdotHL);

	float3 l_DiffuseBSDF = ComputeDiffuseBRDF(NdotV, NdotL, LdotHL, in_Material.m_Roughness, in_Material.m_Metallic, l_FresnelTerm, in_Material.m_Albedo);
	float3 l_SpecularBSDF = ComputeSpecularBRDF(in_BRDFLUT, in_BRDFMSLUT, in_PointSampler, NdotV, NdotL, NdotHL, LdotHL, in_Material.m_Roughness, F0, l_FresnelTerm);

	float3 l_Illuminance = in_LuminousFlux * in_AttenuationFactor * NdotL;

	out_DirectLuminance = (l_DiffuseBSDF + l_SpecularBSDF) * l_Illuminance;
	out_IndirectSeedLuminance = in_Material.m_Albedo * l_Illuminance / PI; // Simple Lambertian for next-frame indirect seed.
}

#endif // LIGHTPASS_COMMON_HLSL
