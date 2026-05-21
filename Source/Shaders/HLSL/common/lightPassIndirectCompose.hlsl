// shadertype=hlsl
//
// LightPass indirect-compose stage. Reads the denoised per-pixel
// irradiance from SSRCTemporalPass and converts SH irradiance to outgoing
// Lambertian radiance via
//   L_o = albedo * (1 - metallic) * E / PI.
//
// Per-pixel under-sampling (paper §2.4.1 relaxed interpolation) is
// resolved upstream: SampleSSRC packs a denoiser_hint into the
// irradiance alpha; SSRCTemporal.comp turns that into the spatial filter's
// blur-mask widening (paper §2.4.3) so under-sampled pixels are
// absorbed by their well-sampled neighbours. Nothing to clamp here.

#ifndef LIGHTPASS_INDIRECT_COMPOSE_HLSL
#define LIGHTPASS_INDIRECT_COMPOSE_HLSL

// Reads the denoised per-pixel irradiance and returns the outgoing
// Lambertian radiance for compose into the visual RT (RT0). The
// caller is responsible for adding the result to its accumulator —
// this function returns a pure contribution, not an inout update.
//
// The illuminance RT (RT1) is intentionally not touched here: it
// carries direct lighting only so next-frame ray hits do not re-
// accumulate already-accumulated indirect energy.
float3 ComposeIndirectLighting(
	in Texture2D<float4> in_GIIrradiance,
	in uint2 in_ScreenCoord,
	in MaterialAttributes in_Material)
{
	float3 l_IrradianceFromCache = in_GIIrradiance[in_ScreenCoord].rgb;

	return in_Material.m_Albedo * (1.0 - in_Material.m_Metallic) * l_IrradianceFromCache / PI;
}

#endif // LIGHTPASS_INDIRECT_COMPOSE_HLSL
