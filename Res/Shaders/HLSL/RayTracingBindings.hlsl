// shadertype=hlsl
#include "common/common.hlsl"
#include "RayTracingTypes.hlsl"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0)
{
    PerFrame_CB g_Frame;
}

// Global acceleration structure (your TLAS), bound in the global RS (e.g., at t0).
[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(1, 1)]]
Texture2D in_opaquePassRT0 : register(t1); // World Space Position (RGB)
[[vk::binding(2, 1)]]
Texture2D in_opaquePassRT1 : register(t2); // World Space Normal (RGB) and Metallic (A)
[[vk::binding(3, 1)]]
Texture2D in_opaquePassRT2 : register(t3); // Albedo (RGB) and Roughness (A)
[[vk::binding(4, 1)]]
Texture2D in_opaquePassRT3 : register(t4); // Motion Vector (RG), AO (B), Transparency (A)
[[vk::binding(5, 1)]]
Texture2D in_LightPassOutgoingLuminance : register(t5);

// RW texture to store computed radiance for each cache entry.
[[vk::binding(0, 2)]]
RWTexture2D<float4> in_RadianceCacheResults : register(u0);

[[vk::binding(1, 2)]]
RWStructuredBuffer<WorldProbe> in_WorldProbeGrid : register(u1);

[[vk::binding(2, 2)]]
RWTexture2D<float4> in_ProbePosition : register(u2);

[[vk::binding(3, 2)]]
RWTexture2D<float4> in_ProbeNormal : register(u3);