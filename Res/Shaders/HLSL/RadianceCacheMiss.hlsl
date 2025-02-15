// shadertype=hlsl
#include "RayTracingBindings.hlsl"
#include "common/skyResolver.hlsl"

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    // Get the world-space ray origin (where the ray starts)
    float3 rayOrigin = WorldRayOrigin();
    float3 rayDirection = WorldRayDirection();

    // Light and atmosphere properties
    float3 lightDir = -g_Frame.sun_direction.xyz;
    float planetRadius = 6371e3;
    float atmosphereHeight = 100e3;

    float3 eyeDir = rayDirection;
	float3 eyePosition = rayOrigin + float3(0.0, planetRadius, 0.0);

    float3 skyRadiance = getSkyColor(
        eyeDir,
        eyePosition,
        lightDir,
        g_Frame.sun_illuminance.xyz,
        planetRadius,
        atmosphereHeight
    );

    payload.radiance = skyRadiance;
}
