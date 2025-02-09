// shadertype=hlsl
#include "RayTracingCommon.hlsl"

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    // Return a fallback environment radiance.
    payload.radiance = float3(0.1, 0.1, 0.1);
    payload.sampleCount = 1;
}
