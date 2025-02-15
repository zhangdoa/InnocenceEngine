// shadertype=hlsl
#include "RayTracingBindings.hlsl"

float3 CosineWeightedHemisphereSample(float2 Xi, float3 N)
{
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt(Xi.y);
    float sinTheta = sqrt(1.0 - Xi.y);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    // Convert tangent space sample to world space
    float3x3 basis = CreateTangentSpace(N);
    return normalize(mul(H, basis));
}

[shader("raygeneration")]
void RayGenShader()
{
    uint2 probeIndex = DispatchRaysIndex().xy;
    uint2 upscaledProbeSize = probeAtlasSize * upscaleFactor;
    uint2 probeScreenPos = probeIndex * upscaledProbeSize;

    // Apply jittering to select a random pixel within the probe tile
    float2 jitter = float2(g_Frame.radianceCacheJitter_x, g_Frame.radianceCacheJitter_y);
    uint2 jitterOffset = uint2(jitter);
    uint2 samplingScreenPos = probeScreenPos + jitterOffset;

    // Fetch world space position
    bool valid = in_opaquePassRT0.Load(int3(samplingScreenPos, 0)).w == 1.0;
    if (!valid)
    {
        return;
    }

    float3 positionWS = in_opaquePassRT0.Load(int3(samplingScreenPos, 0)).xyz;
    if (isnan(positionWS.x) || isnan(positionWS.y) || isnan(positionWS.z))
    {
        return;
    }

    // Fetch normal
    float3 normalWS = normalize(in_opaquePassRT1.Load(int3(samplingScreenPos, 0)).xyz);

    // Store probe position and normal for the next frame's reprojection pass
    in_ProbePosition[probeIndex] = float4(positionWS, 1);
    in_ProbeNormal[probeIndex] = float4(normalWS, 1);

    // Fixed number of rays per probe (@TODO: this is a basic version, replace it with temporal upscaling + single ray per frame)
    const int NUM_SAMPLES = 8;
    float3 totalRadiance = 0;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        // Generate cosine-weighted hemisphere sample
        float2 randVal = Hash2D(samplingScreenPos, i + g_Frame.frameIndex);
        float3 sampleDir = CosineWeightedHemisphereSample(randVal, normalWS);

        RayDesc ray;
        ray.Origin = positionWS + normalWS * 0.001;
        ray.Direction = sampleDir;
        ray.TMin = 0.01;
        ray.TMax = 1000.0;

        RayPayload tempPayload;
        tempPayload.radiance = float3(0, 0, 0);

        TraceRay(SceneAS, RAY_FLAG_NONE, 0xFF, 0, 1, 2, ray, tempPayload);
        float3 NdotL = saturate(dot(normalWS, sampleDir));
        float3 radiance = tempPayload.radiance * NdotL;
        // The attenuation factor is applied in the closest hit shader already

        // Store in radiance cache
        uint2 texIndex = GetAtlasTextureCoordinates(float2(probeScreenPos), sampleDir);
        float3 oldScreenSpaceRadiance = in_RadianceCacheResults[texIndex].xyz;

        // @TODO: Use a more advanced blending function
        in_RadianceCacheResults[texIndex] = float4(lerp(oldScreenSpaceRadiance, radiance, 0.5), 1);

        uint worldProbeIndex = ComputeProbeHash(positionWS);
        float3 oldWorldProbeRadiance = in_WorldProbeGrid[worldProbeIndex].radiance;
        in_WorldProbeGrid[worldProbeIndex].positionWS = positionWS;

        // @TODO: Use a more advanced blending function
        in_WorldProbeGrid[worldProbeIndex].radiance = lerp(oldWorldProbeRadiance, radiance, 0.5);
        in_WorldProbeGrid[worldProbeIndex].weight = 1.0;
    }
}
