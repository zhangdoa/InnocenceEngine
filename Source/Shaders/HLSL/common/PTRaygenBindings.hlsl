// shadertype=hlsl
#ifndef PT_RAYGEN_BINDINGS_HLSL
#define PT_RAYGEN_BINDINGS_HLSL

// Resource bindings for GPUPathTracerRayGen.hlsl. The matching descriptor
// table is populated in
// Source/ExampleProject/RenderingClient/GPUPathTracerPass_BindingLayout.cpp;
// the layout count + register order in this header and that .cpp must move
// in lockstep — count drift is a silent root-signature / DXIL mismatch
// surface (b9a103cc PSO-failure precedent).
//
// Toggle gates: the PT_HASH_GRID_CACHE_ENABLED + PT_DENOISE_ENABLED #define
// values must already be set by the consumer before this header is
// included. When OFF, the corresponding cbuffer / UAV declarations strip
// at compile time and no descriptor entries are allocated on the C++ side
// (the cache + denoise binding-count consts are 0 under their respective
// `if constexpr` short-circuits in ConfigureRaytracingBindings).

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(1, 0)]]
cbuffer FrameCountCB : register(b1) { uint g_FrameCount; }

[[vk::binding(2, 0)]]
cbuffer LightCountCB : register(b2) { uint g_PointLightCount; uint g_SphereLightCount; uint g_LightCountPad0; uint g_LightCountPad1; }

#if PT_HASH_GRID_CACHE_ENABLED
[[vk::binding(3, 0)]]
cbuffer HashGridCacheCB : register(b3) { PTHashGridCacheCB_t g_HashGridCacheConstants; }
#endif

#if PT_DENOISE_ENABLED
// Previous-frame per-frame CB (engine ping-pong, populated by
// PerFrameDataService each frame). Reused unchanged from OpaquePass.frag
// (b2 there); we get b4 here because b2 is LightCountCB in this raygen's
// existing layout.
[[vk::binding(4, 0)]]
cbuffer PerFrameConstantBufferPrev : register(b4) { PerFrame_CB g_FramePrev; }
#endif

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(5, 1)]]
StructuredBuffer<PointLight_CB> g_PointLights : register(t5);

[[vk::binding(6, 1)]]
StructuredBuffer<SphereLight_CB> g_SphereLights : register(t6);

[[vk::binding(0, 2)]]
RWTexture2D<float4> AccumBuffer : register(u0);

#if PT_HASH_GRID_CACHE_ENABLED
[[vk::binding(1, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_HashBuffer            : register(u1);

[[vk::binding(2, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_DecayTileBuffer       : register(u2);

[[vk::binding(3, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_UpdateCellValueBuffer : register(u3);

[[vk::binding(4, 2)]]
RWStructuredBuffer<uint2> g_HashGridCache_ValueBuffer           : register(u4);

// Indirect-mirror UAVs (Capsaicin gi1.comp:1948-1989 UpdateMultibounceCells).
// u5 carries the (b) secondary-bounce write target — the BRDF/pdf-modulated
// next-vertex mean atomic-added into the previous vertex's indirect scratch.
// u6 is the resolved indirect-lobe estimator read at Site-3 alongside u4.
[[vk::binding(5, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_UpdateCellValueIndirectBuffer : register(u5);

[[vk::binding(6, 2)]]
RWStructuredBuffer<uint2> g_HashGridCache_ValueIndirectBuffer           : register(u6);
#endif

#if PT_DENOISE_ENABLED
// GBuffer-equivalent UAVs written at bounce == 0. Channel layout per
// common/PTDenoiseShared.hlsl — mirrors the rasterized GBuffer
// produced by opaqueGeometryProcessPass.frag so DecodeGBuffer
// (common/lightPassCommon.hlsl) is reusable on PT-mode denoiser inputs.
[[vk::binding(7, 2)]]
RWTexture2D<float4> u_PTDenoise_PositionInstanceID : register(u7);

[[vk::binding(8, 2)]]
RWTexture2D<float4> u_PTDenoise_NormalMetalness    : register(u8);

[[vk::binding(9, 2)]]
RWTexture2D<float4> u_PTDenoise_AlbedoRoughness    : register(u9);

[[vk::binding(10, 2)]]
RWTexture2D<float4> u_PTDenoise_MotionHitDist      : register(u10);

// Per-lobe primary-hit radiance, written at the AccumBuffer-composition
// site. SVGF demodulated diffuse / specular channels (CL-2 input). Both
// RGBA16F: rgb carries the lobe's unfiltered radiance for the current
// frame, alpha is reserved (zero on write) — alpha will carry per-lobe
// hit-distance in CL-3 when ReBLUR-shape specular blur radius needs it,
// kept zero here to avoid signaling intent we don't yet enforce.
[[vk::binding(11, 2)]]
RWTexture2D<float4> u_PTDenoise_RadianceDiffuse    : register(u11);

[[vk::binding(12, 2)]]
RWTexture2D<float4> u_PTDenoise_RadianceSpecular   : register(u12);
#endif

#endif // PT_RAYGEN_BINDINGS_HLSL
