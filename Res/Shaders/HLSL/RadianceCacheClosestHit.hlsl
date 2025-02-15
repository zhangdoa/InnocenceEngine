// shadertype=hlsl
#include "RayTracingBindings.hlsl"
#include "common/BSDF.hlsl"

float2 NDCToScreenSpace(float2 positionNDC)
{
    // Convert to screen space (0 to 1)
    float2 screenCoord = positionNDC * 0.5 + 0.5;

    // Flip Y for screen-space
    screenCoord.y = 1.0 - screenCoord.y;

    // Convert to pixel space (0 to viewportSize)
    screenCoord *= g_Frame.viewportSize.xy;

    return screenCoord;
}

float4 ClipToNDC(float4 positionCS)
{
    float w = max(abs(positionCS.w), EPSILON);
    return positionCS / w;
}

float2 ClipToScreenSpace(float4 positionCS)
{
    float4 positionNDC = ClipToNDC(positionCS);
    return NDCToScreenSpace(positionNDC.xy);
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // Compute hit position in world space
    float3 hitPositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Compute screen space coordinate from hitPositionWS
    float4 positionVS = mul(float4(hitPositionWS, 1.0), g_Frame.v);
    float4 positionCS = mul(positionVS, g_Frame.p_original);
    float2 screenCoord = ClipToScreenSpace(positionCS);

    float2 motionVector = in_opaquePassRT3.Load(int3(screenCoord, 0)).xy;
    float2 prevScreenCoord = screenCoord + motionVector;

    // Fetch previous frame the light pass radiance
    float3 hitRadiance = float3(0, 0, 0);
    bool withinBounds = all(prevScreenCoord >= float2(0, 0)) && all(prevScreenCoord <= g_Frame.viewportSize.xy);
    if (withinBounds)
    {
        float3 hitNormal = normalize(in_opaquePassRT1.Load(int3(prevScreenCoord, 0)).xyz);

        // The outgoing luminance is illuminance times albedo and divided by PI — a simple Lambertian diffuse model
        hitRadiance = in_LightPassOutgoingLuminance.Load(int3(prevScreenCoord, 0)).rgb;
        float3 V = -WorldRayDirection();
        float3 NdotV = saturate(dot(hitNormal, V));
        hitRadiance *= NdotV;

        float l_LightAttenuationRadius = length(RayTCurrent());
        float l_InvertedSquareAttenuationRadius = 1.0 / max(l_LightAttenuationRadius * l_LightAttenuationRadius, EPSILON);
        // The unnormalized light direction is the ray direction since it is about evaluating the radiance at the ray origin, and the hit point acts as the light source
        float attenuation = CalculateDistanceAttenuation(WorldRayDirection(), l_InvertedSquareAttenuationRadius);
        hitRadiance *= attenuation;

        if (isnan(hitRadiance.x) || isnan(hitRadiance.y) || isnan(hitRadiance.z))
        {
            hitRadiance = float3(0, 0, 0);
        }
    }
    else
    {
        uint worldProbeIndex = ComputeProbeHash(hitPositionWS);
        hitRadiance = in_WorldProbeGrid[worldProbeIndex].radiance;
    }

    payload.radiance = hitRadiance;
}
