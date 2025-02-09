// shadertype=hlsl
#include "RayTracingCommon.hlsl"

[shader("raygeneration")]
void RayGenShader()
{
    uint2 index = DispatchRaysIndex().xy;
    float3 position = in_opaquePassRT0.Load(int3(index, 0)).xyz;
    float3 normal = in_opaquePassRT1.Load(int3(index, 0)).xyz;

    RayDesc ray;
    ray.Origin = position;
    ray.Direction = normalize(normal + float3(0.1, 0.1, 0.1) * (float)index.x);
    ray.TMin = 0.0;
    ray.TMax = 50.0;

    RayPayload payload;
    payload.radiance = float3(0, 0, 0);
    payload.sampleCount = 0;

    TraceRay(
        SceneAS,          // Acceleration structure
        RAY_FLAG_NONE,  // Ray flags
        0xFF,           // Instance mask
        0,              // Ray contribution to hit group index
        1,              // Multiplier for hit group index
        0,              // Miss shader index
        ray,            // Ray description
        payload         // Payload struct
    );

    RadianceCacheResults[index] = float4(payload.radiance, 1.0);
}
