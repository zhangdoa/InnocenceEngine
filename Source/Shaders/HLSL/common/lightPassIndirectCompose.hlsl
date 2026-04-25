// shadertype=hlsl
//
// LightPass indirect-compose stage. Reads the denoised per-pixel
// irradiance from GIDenoisePass, applies a constant ambient floor
// (TASK-123 placeholder pending world-cache fallback), and converts
// SH irradiance to outgoing Lambertian radiance via
//   L_o = albedo * (1 - metallic) * E / PI.
//
// Intentionally isolated: the SRV / sampler / extent surface this
// reads through is the surface TASK-127 (GI 2/3-frame coordinate bug)
// is investigating. Land that fix here, not in the kernel.

#ifndef LIGHTPASS_INDIRECT_COMPOSE_HLSL
#define LIGHTPASS_INDIRECT_COMPOSE_HLSL

// Ambient floor for shadowed pixels with zero radiance-cache return.
// Single-bounce GI returns near-zero where probes mostly trace into
// other shadowed surfaces; combined with sun-shadow zeroing direct,
// those pixels would render pure black. max() (not additive) preserves
// bright regions exactly. Replaced by world-cache fallback / multi-
// bounce RayGen (see TASK-123) when those land.
static const float3 LIGHTPASS_AMBIENT_FLOOR = float3(0.02, 0.025, 0.03);

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
	l_IrradianceFromCache = max(l_IrradianceFromCache, LIGHTPASS_AMBIENT_FLOOR);

	return in_Material.m_Albedo * (1.0 - in_Material.m_Metallic) * l_IrradianceFromCache / PI;
}

#endif // LIGHTPASS_INDIRECT_COMPOSE_HLSL
