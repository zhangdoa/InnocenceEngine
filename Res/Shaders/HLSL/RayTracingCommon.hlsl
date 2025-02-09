// shadertype=hlsl
#include "common/common.hlsl"

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

// RW texture to store computed radiance for each cache entry.
RWTexture2D<float4> RadianceCacheResults : register(u0);

// Payload structure passed between TraceRay calls.
struct RayPayload
{
    float3 radiance;
    uint sampleCount;
};

// Function to calculate Lambertian radiance using opaque pass input.
float3 CalculateLambertianRadiance(float3 position, float3 normal, float3 albedo)
{
    float3 lightDir = normalize(float3(0, 1, 0)); // Light coming from above
    float NdotL = saturate(dot(normal, lightDir));
    return albedo * NdotL;
}
