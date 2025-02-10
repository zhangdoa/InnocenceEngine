// shadertype=hlsl
#include "RayTracingCommon.hlsl"

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // Compute hit position in world space
    float3 hitPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Compute screen space coordinate from hitPosition
    float4 positionVS = mul(float4(hitPosition, 1.0), g_Frame.v);
    float4 positionCS = mul(positionVS, g_Frame.p_original);
    float w = max(abs(positionCS.w), 0.0001);
    positionCS.xyz /= w;
    float2 screenCoord = positionCS.xy * 0.5 + 0.5;
    screenCoord.y = 1.0 - screenCoord.y; // Flip Y for screen-space

    // Fetch current frame lighting
    float3 hitRadiance = float3(0, 0, 0);
    if (all(screenCoord >= float2(0, 0)) && all(screenCoord <= float2(1, 1))) // Check if within screen bounds
    {
        int2 screenPos = int2(screenCoord * g_Frame.viewportSize.xy);
        hitRadiance = in_lightPassRT0.Load(int3(screenPos, 0)).rgb;
        if (isnan(hitRadiance.x) || isnan(hitRadiance.y) || isnan(hitRadiance.z))
        {
            hitRadiance = float3(0, 0, 0);
        }
    }

    // Store in payload
    payload.radiance = hitRadiance;
    payload.sampleCount = 1;
}
