#pragma once

// C++ mirror of the screen-space PT denoiser toggle and shared layout
// constants defined in Source/Shaders/HLSL/common/PTDenoiseShared.hlsl.
// The shader header owns the canonical channel-layout enumeration; this
// header carries the C++ side's compile-time toggle so GPUPathTracerPass
// can branch on `if constexpr (Inno::PTDenoise::ENABLED)` for resource
// allocation, binding, and dispatch without redefining the channel
// layout per pass.
//
// Reference: SVGF (Schied et al. 2017) — temporal accumulation +
// edge-aware à-trous spatial filter. Architectural pattern; CL-1 lands
// only the primary-hit signal split + GBuffer-equivalent UAV writes.
//
// Drift between this header's ENABLED flag and the HLSL
// PT_DENOISE_ENABLED #define manifests as a root-signature / DXIL
// mismatch — keep them in lockstep.

namespace Inno
{
namespace PTDenoise
{
    // Master compile-time toggle. When false, GPUPathTracerPass does not
    // allocate or bind the GBuffer-equivalent UAVs, and the corresponding
    // #if-gated raygen code in GPUPathTracerRayGen.hlsl strips out at
    // compile time. AccumBuffer write is bit-identical to the toggle-off
    // path tracer.
    //
    // The HLSL side mirrors this in GPUPathTracerRayGen.hlsl
    // (`#define PT_DENOISE_ENABLED`); both must hold the same value.
    static constexpr bool ENABLED = false;

    // Temporal-accumulator constants (CL-2). Mirrored on the HLSL side in
    // common/PTDenoiseShared.hlsl so the rejection thresholds and history
    // cap stay in lockstep across the C++ pass scheduling and the
    // PTDenoiseTemporal.comp kernel body.
    //
    // SVGF default — reaches `α = 1/32` blend weight at full convergence,
    // an empirically common balance between residual noise and lag under
    // motion (Schied 2017 §3). 16 trades convergence depth for faster
    // motion response; 32 keeps more samples but takes longer to evict
    // stale history when reprojection just barely passes the gate. Match
    // SVGF reference; revisit if rejection turns out to leak ghosting.
    static constexpr uint32_t MaxHistoryFrames = 32u;

    // History rejection — mesh-id strict equality plus geometry tests.
    // Matches GIDenoise.comp:245 (`dot(N, prevN) > 0.95`); the depth gate
    // is relative because absolute thresholds break across scene scales
    // (Capsaicin gi1.comp:4039 and the SVGF reference both go relative).
    // 0.1 = 10% linear-depth tolerance; tuned in CL-2 capture (revisit
    // alongside specular blur radius in CL-3 if disocclusion flicker
    // shows up at the threshold boundary).
    static constexpr float HistoryNormalDotThreshold = 0.95f;
    static constexpr float HistoryDepthRelativeThreshold = 0.1f;
}
} // namespace Inno
