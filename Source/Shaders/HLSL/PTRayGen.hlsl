// shadertype=hlsl
#include "common/common.hlsl"
#include "common/skyResolver.hlsl"
#include "common/pathTracerPayload.hlsli"
#include "common/sunSampling.hlsl"

// Master toggle for the secondary-vertex hash-grid radiance cache. When 0,
// the cache code below strips at compile time and this shader produces the
// same DXIL/SPIR-V as the cache-off path tracer (the bypass invariant —
// visual-validation.md §3b). The C++ side mirrors this in
// Source/ExampleProject/RenderingClient/HashGridCacheConstants.h::ENABLED;
// both must agree.
//
// Reference: Capsaicin GI-1.0 hash_grid_cache.hlsl + gi1.comp secondary-
// vertex sites. Full audit at .alignments/TASK-77.1-rework-paper-port-audit.md.
#define PT_HASH_GRID_CACHE_ENABLED 0

#if PT_HASH_GRID_CACHE_ENABLED
#include "common/PTHashGridCache.hlsl"
#endif

// Master toggle for the screen-space PT denoiser's primary-hit signal
// split + GBuffer-equivalent UAV writes. Mirrors Inno::PTDenoise::ENABLED
// in Source/ExampleProject/RenderingClient/PTDenoiseConstants.h. When 0,
// no UAVs allocated, no shader bytes emitted, AccumBuffer write is bit-
// identical to the denoiser-off path tracer (the bypass invariant —
// visual-validation.md §3b).
//
// Reference: SVGF (Schied 2017) — temporal accumulation + edge-aware
// à-trous spatial filter. Architectural pattern; CL-1 lands the signal-
// split + GBuffer-equivalent write only.
#define PT_DENOISE_ENABLED 1

#if PT_DENOISE_ENABLED
#include "common/PTDenoiseShared.hlsl"
#endif

// Resource bindings (cbuffers, TLAS, light SRVs, AccumBuffer UAV plus the
// toggle-gated cache + denoiser UAV blocks). Ordering: bindings include
// before helpers so the helper functions see g_Frame at file scope.
#include "common/PTRaygenBindings.hlsl"
#include "common/PTRaygenHelpers.hlsl"

// Path-tracer integrator body — bounce loop, NEE, BSDF importance sample,
// optional cache substitution, optional denoiser GBuffer-equivalent write,
// AccumBuffer composition. Behaviour-preserving extraction; toggle=0 +
// toggle=1 DXIL byte-identity is the gate (see TASK-77.2 CL-1 implementation
// note).
#include "common/PTRaygenIntegrator.hlsl"

[shader("raygeneration")]
void RayGenShader()
{
    uint2 pixel      = DispatchRaysIndex().xy;
    uint2 resolution = DispatchRaysDimensions().xy;
    RunPathIntegrator(pixel, resolution);
}
