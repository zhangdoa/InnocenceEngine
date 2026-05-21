// shadertype=hlsl
#ifndef SSRC_SPATIAL_COMMON_HLSL
#define SSRC_SPATIAL_COMMON_HLSL

#include "common.hlsl"

// GI-1.0 §2.4.3 / Figure 19 spatial filter — separable variable-radius
// bilateral blur driven by the per-pixel blur mask written by
// SSRCTemporal.comp. Capsaicin reference: gi1.comp:4104–4154 (FilterGI) and
// gi_denoiser.hlsl:43–54 (SSRCTemporalr_GetBlurRadius). Two compilation
// units (`SSRCSpatialHorizontal.comp`, `SSRCSpatialVertical.comp`) define
// `BLUR_DIRECTION` and include this header — the horizontal pass blurs
// along ±x, the vertical pass blurs along ±y, matching Capsaicin's
// 2-pass dispatch (gi1.cpp:2882–2927).
//
// Storage convention (paper-port aligned with Capsaicin gi1.comp:4100):
//   - in_GIInput.rgb   = sample-count-weighted radiance Σ Lᵢ
//   - in_GIInput.a     = sample count N (denoiser_hint sentinel `-1` reserved
//                        for non-sample taps but unused in our pipeline)
// The filter operates on (rgb / max(N, 1)) per tap, accumulates weighted,
// then re-multiplies by `max(lighting.w, 1.0)` so the (rgb·N, N) shape
// flows through the horizontal pass unchanged. The vertical pass divides
// out the final N and writes the ready-to-shade irradiance.

#ifndef BLUR_DIRECTION
#error "Define BLUR_DIRECTION (int2(1,0) for H, int2(0,1) for V) before including SSRCSpatialCommon.hlsl"
#endif

// Capsaicin gi_denoiser.hlsl:26. Caps both the cap clamp in SSRCTemporal.comp
// (`kSSRCTemporalr_MaxBlurMask` there) and the spatial filter radius here.
// Per pixel: `blur_radius = round(blur_mask * MAX_BLUR_MASK)` with
// `blur_mask` already normalised to [0, 1] in the texture. Range is
// 0–8 pixels per axis.
static const float kSSRCTemporalr_MaxBlurMask = 16.0;

// Capsaicin gi1.comp:4134 — bilateral depth weight scale. Tighter
// (larger) when the centre pixel has any sample (`lighting.w > 0`); a
// fresh disocclusion (`lighting.w == 0`) gets a looser depth gate so the
// filter can pull in usable irradiance from the surrounding converged
// neighbourhood.
static const float kDepthFactorConverged = 200.0; // 2e2
static const float kDepthFactorDisocclusion = 20.0; // 2e1

struct ComputeInputType { uint3 dispatchThreadID : SV_DispatchThreadID; };

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0)
{
	PerFrame_CB g_Frame;
}

// G-buffer world position (xyz) + sky flag (w==0 marks no geometry).
// Used both for the early-out and to derive a depth surrogate
// (`length(world - camera_posWS)`) for the bilateral depth weight.
[[vk::binding(0, 1)]] Texture2D<float4> in_opaquePassRT0 : register(t0);
// G-buffer world normal (xyz). Bilateral normal weight references this
// at both the centre and tap pixels; mismatch on the seam between two
// surface orientations rejects the tap.
[[vk::binding(1, 1)]] Texture2D<float4> in_opaquePassRT1 : register(t1);
// Sample-count-weighted GI irradiance. H pass reads GIHistory written
// this frame by SSRCTemporal; V pass reads the H pass's scratch output
// (same shape).
[[vk::binding(2, 1)]] Texture2D<float4> in_GIInput : register(t2);
// Per-pixel blur mask written by SSRCTemporal.comp. Stored in [0, 1] for
// real surfaces (1 = max radius, 0 = no blur), with -1 as the sky
// sentinel. Multiply by `kSSRCTemporalr_MaxBlurMask` to recover Capsaicin's
// raw `blur_mask` value.
[[vk::binding(3, 1)]] Texture2D<float> in_BlurMask : register(t3);

// H pass: writes the same (rgb·N, N) shape so the vertical pass can keep
// blurring in sample-count-weighted space.
// V pass: writes the final divided irradiance (rgb / max(N, 1), 1).
[[vk::binding(0, 2)]] RWTexture2D<float4> out_GIOutput : register(u0);

// Capsaicin gi_denoiser.hlsl:43–54. blur_mask is stored normalised on
// disk; multiply back to recover the raw value, then round to the nearest
// integer pixel count. Range is [0, 8] per axis.
int GetBlurRadius(in uint2 pos)
{
	int blur_radius = 0;
	float blur_mask = in_BlurMask.Load(int3(pos, 0)) * kSSRCTemporalr_MaxBlurMask;

	if (blur_mask > 0.0)
	{
		blur_radius = int(max(blur_mask, 1.0) + 0.5);
	}

	return blur_radius;
}

// Capsaicin gi1.comp:4104–4154 (FilterGI), ported verbatim modulo
// binding names and the use of world-space position in place of a depth
// buffer. Sample-count-weighted accumulator in/out.
[numthreads(8, 8, 1)]
void main(ComputeInputType input)
{
	int2 viewport = int2(g_Frame.viewportSize.xy);
	int2 did = int2(input.dispatchThreadID.xy);
	if (any(did >= viewport))
		return;

	float4 lighting = in_GIInput.Load(int3(did, 0));
	int blur_radius = GetBlurRadius(uint2(did));

	if (blur_radius > 0)
	{
		float weight = 1.0;
		float3 color = lighting.xyz / max(lighting.w, 1.0);

		float4 centerPos4 = in_opaquePassRT0.Load(int3(did, 0));
		float3 center_normal = normalize(in_opaquePassRT1.Load(int3(did, 0)).xyz);
		float center_depth = length(centerPos4.xyz - g_Frame.camera_posWS.xyz);

		// Branch the depth-tightness once outside the loop. A converged
		// pixel (`lighting.w > 0`) wants the tighter Capsaicin K=200; a
		// fresh disocclusion uses K=20 so the filter can blend against
		// neighbours that differ in depth more than would be admissible
		// in steady state.
		float depth_factor_k = (lighting.w > 0.0) ? kDepthFactorConverged : kDepthFactorDisocclusion;

		for (int r = -blur_radius; r <= blur_radius; ++r)
		{
			int2 pos = clamp(did + r * BLUR_DIRECTION, int2(0, 0), viewport - int2(1, 1));
			float4 c = in_GIInput.Load(int3(pos, 0));

			if (c.w > 0.0)
			{
				float4 tapPos4 = in_opaquePassRT0.Load(int3(pos, 0));
				// Tap on sky / no geometry — its world position is the
				// zero sentinel and the depth surrogate would clamp to
				// the camera position. Skip explicitly to match
				// Capsaicin's `c.w > 0` branch in spirit (their
				// equivalent gate is the depth-buffer read, which we
				// don't have on this path).
				if (tapPos4.a == 0.0)
					continue;

				float3 normal = normalize(in_opaquePassRT1.Load(int3(pos, 0)).xyz);
				float depth = length(tapPos4.xyz - g_Frame.camera_posWS.xyz);

				float depth_diff = 1.0 - (center_depth / max(depth, EPSILON));
				float depth_factor = exp2(-depth_factor_k * abs(depth_diff));
				float normal_factor = max(dot(normal, center_normal), 0.0);
				normal_factor *= normal_factor; normal_factor *= normal_factor; // ^4

				// `lighting.w == 0` (no centre history) skips the normal
				// gate so the disocclusion can grab any plausibly-lit
				// neighbour. Capsaicin gi1.comp:4138.
				float w = depth_factor * (lighting.w > 0.0 ? normal_factor : 1.0);

				color += w * (c.xyz / max(c.w, 1.0));
				weight += w;
			}
		}

		// Re-multiply by the centre's sample count so the (rgb·N, N)
		// invariant survives this pass for the next reader. The vertical
		// pass undoes this once at output (gi1.comp:4150).
		lighting.xyz = (color / weight) * max(lighting.w, 1.0);
	}

	// gi1.comp:4148–4153. The vertical pass owns the final divide; the
	// horizontal pass re-emits the sample-count-weighted form so the
	// vertical pass keeps using it.
	if (BLUR_DIRECTION.y > 0)
	{
		out_GIOutput[did] = float4(lighting.xyz / max(lighting.w, 1.0), 1.0);
	}
	else
	{
		out_GIOutput[did] = lighting;
	}
}

#endif // SSRC_SPATIAL_COMMON_HLSL
