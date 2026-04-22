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
        // Read the Lambertian diffuse outgoing radiance (albedo * E / PI) from the previous frame.
        // Lambertian radiance is view-independent, so no NdotV factor.
        // Radiance is constant along a ray, so no distance attenuation.
        hitRadiance = in_LightPassOutgoingLuminance.Load(int3(prevScreenCoord, 0)).rgb;

        if (any(isnan(hitRadiance)) || any(isinf(hitRadiance)))
        {
            hitRadiance = float3(0, 0, 0);
        }
    }
    else
    {
        // Off-screen hit: walk the world cache by fingerprint per [W.1].
        // A matching fingerprint within MAX_LINEAR_PROBE slots returns
        // the cached outgoing radiance; a miss leaves the contribution
        // at zero (better than reading another cell's accumulated colour).
        uint bucket = ComputeProbeHash(hitPositionWS);
        uint fingerprint = ComputeProbeFingerprint(hitPositionWS);
        for (uint probe = 0u; probe < MAX_LINEAR_PROBE; probe++)
        {
            uint slot = (bucket + probe) % HASH_TABLE_SIZE;
            if (in_WorldProbeGrid[slot].fingerprint == fingerprint)
            {
                hitRadiance = in_WorldProbeGrid[slot].radiance;
                break;
            }
            if (in_WorldProbeGrid[slot].fingerprint == 0u)
                break; // empty slot — no entry exists for this cell
        }
    }

    payload.radiance = hitRadiance;
    payload.distance = RayTCurrent();
}
