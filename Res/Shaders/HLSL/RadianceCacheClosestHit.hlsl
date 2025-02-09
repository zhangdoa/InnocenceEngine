// shadertype=hlsl
#include "RayTracingCommon.hlsl"

// In a triangle hit, built-in attributes (e.g., barycentrics) could be used to interpolate vertex data.
// For simplicity we use a fixed normal here.
[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload,
    in BuiltInTriangleIntersectionAttributes attrib)
{
    uint2 index = DispatchRaysIndex().xy;
    float3 position = in_opaquePassRT0.Load(int3(index, 0)).xyz;
    float3 normal = in_opaquePassRT1.Load(int3(index, 0)).xyz;
    float3 albedo = in_opaquePassRT2.Load(int3(index, 0)).rgb;

    float3 lambertianRadiance = CalculateLambertianRadiance(position, normal, albedo);

    payload.radiance = lambertianRadiance;
    payload.sampleCount = 1;
}
