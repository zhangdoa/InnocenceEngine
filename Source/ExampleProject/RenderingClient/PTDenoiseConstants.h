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
}
} // namespace Inno
