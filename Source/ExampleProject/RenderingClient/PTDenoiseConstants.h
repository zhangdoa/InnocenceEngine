#pragma once

// C++ mirror of the per-lobe GBuffer-equivalent UAV write toggle defined in
// Source/Shaders/HLSL/common/PTDenoiseShared.hlsl. The shader header owns the
// canonical channel-layout enumeration; this header carries the C++ side's
// compile-time toggle so GPUPathTracerPass can branch on
// `if constexpr (Inno::PTDenoise::ENABLED)` for resource allocation, binding,
// and dispatch without redefining the channel layout per pass.
//
// Drift between this header's ENABLED flag and the HLSL PT_DENOISE_ENABLED
// #define manifests as a root-signature / DXIL mismatch — keep them in
// lockstep. As of TASK-77.4 CL-2 both are pinned to true: NRD ReBLUR consumes
// the per-lobe radiance UAVs + the GBuffer-equivalent textures via the new
// PTNRDFormatConvertPass, so this toggle is no longer optional.
//
// The PTDenoise namespace name survives CL-2 because flipping it to NRD
// would touch every if-constexpr site in GPUPathTracerPass; a future CL can
// rename the namespace (or merge with Inno::NRD) once that mass-rename has
// its own diff isolated.

namespace Inno
{
namespace PTDenoise
{
    // Master compile-time toggle. When true, GPUPathTracerPass allocates and
    // binds the GBuffer-equivalent UAVs, and the corresponding #if-gated
    // raygen code in GPUPathTracerRayGen.hlsl emits the per-lobe radiance
    // writes. The HLSL side mirrors this in GPUPathTracerRayGen.hlsl
    // (`#define PT_DENOISE_ENABLED`); both must hold the same value.
    static constexpr bool ENABLED = true;
}
} // namespace Inno
