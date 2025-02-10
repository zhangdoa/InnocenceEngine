// shadertype=hlsl
#include "RayTracingCommon.hlsl"

// Sample previous frame’s radiance for temporal reprojection
float3 SamplePreviousRadiance(int2 screenPos, float2 motionVector)
{
    int2 prevScreenPos = screenPos - int2(motionVector * g_Frame.viewportSize.xy);
    prevScreenPos = clamp(prevScreenPos, int2(0, 0), int2(g_Frame.viewportSize.xy - 1));
    return RadianceCacheResults.Load(int3(prevScreenPos.x, prevScreenPos.y, 0)).rgb;
}

// Create tangent space for importance sampling
float3x3 CreateTangentSpace(float3 normal)
{
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    return float3x3(tangent, bitangent, normal);
}

// Importance sampling GGX for indirect reflection rays
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float alpha = roughness * roughness;
    float phi = 2.0 * 3.14159265 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    float3x3 basis = CreateTangentSpace(N);
    return normalize(mul(H, basis));
}

// Random hash function for ray variation
float2 Hash2D(uint2 pixelID, uint seed)
{
    uint n = pixelID.x * 73856093u ^ pixelID.y * 19349663u ^ seed * 83492791u;
    n = (n << 13u) ^ n;
    return float2(
        (n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffff,
        (n * (n * n * 12347u + 45679u) + 987654321u) & 0x7fffffff
    ) / float(0x7fffffff);
}

[shader("raygeneration")]
void RayGenShader()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 screenSize = DispatchRaysDimensions().xy;

    float3 positionWS = in_opaquePassRT0.Load(int3(launchIndex, 0)).xyz;
    float3 normalWS = normalize(in_opaquePassRT1.Load(int3(launchIndex, 0)).xyz);
    float roughness = in_opaquePassRT2.Load(int3(launchIndex, 0)).a;
    float metallic = in_opaquePassRT1.Load(int3(launchIndex, 0)).a;
    float2 motionVector = in_opaquePassRT3.Load(int3(launchIndex, 0)).xy;

    if (all(positionWS == float3(0, 0, 0)) || isnan(positionWS.x))
    {
        RadianceCacheResults[launchIndex] = float4(0, 0, 0, 1);
        return;
    }

    RayPayload payload;
    payload.radiance = float3(0, 0, 0);
    payload.sampleCount = 0;

    const int NUM_SAMPLES = 8;
    float3 totalRadiance = 0;

    float3 rayDirection = normalize(positionWS - g_Frame.camera_posWS.xyz); // Replace WorldRayDirection()

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 randVal = Hash2D(launchIndex, i);
        float3 sampleDir = ImportanceSampleGGX(randVal, normalWS, roughness);

        float3 mixedDir = lerp(sampleDir, reflect(-rayDirection, normalWS), metallic);

        RayDesc ray;
        ray.Origin = positionWS + normalWS * 0.001;
        ray.Direction = normalize(mixedDir);
        ray.TMin = 0.01;
        ray.TMax = 1000.0;

        RayPayload tempPayload;
        tempPayload.radiance = float3(0, 0, 0);
        tempPayload.sampleCount = 0;

        TraceRay(SceneAS, RAY_FLAG_NONE, 0xFF, 0, 1, 2, ray, tempPayload);

        totalRadiance += tempPayload.radiance;
    }

    float confidence = saturate(dot(normalWS, rayDirection));
    float blendFactor = lerp(0.9, 0.2, confidence);
    float3 newRadiance = totalRadiance / NUM_SAMPLES;
    float3 prevRadiance = SamplePreviousRadiance(launchIndex, motionVector);
    float3 finalRadiance = lerp(prevRadiance, newRadiance, blendFactor);

    RadianceCacheResults[launchIndex] = float4(finalRadiance, 1);
}
