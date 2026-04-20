// shadertype=hlsl
#include "RayTracingBindings.hlsl"
#include "common/RadianceCacheCommon.hlsl"

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

// R2 quasi-random sequence (generalized golden ratio, Martin Roberts 2018)
// Produces well-stratified 2D samples with minimal clumping
float2 R2Sequence(uint index)
{
    static const float g = 1.32471795724; // Plastic constant
    static const float a1 = 1.0 / g;
    static const float a2 = 1.0 / (g * g);
    return frac(float2(a1 * index, a2 * index) + 0.5);
}

float2 Hash2D(uint2 pixelID, uint sampleIndex, uint frameIndex)
{
    uint baseIndex = pixelID.x * 73u + pixelID.y * 157u + frameIndex * 13u;
    return R2Sequence(baseIndex * 4u + sampleIndex);
}

// Sample from cumulative distribution function of reprojected radiance
float3 ImportanceSampleFromCDF(float2 Xi, float3 normalWS, uint2 probeIndex)
{
    float totalLuminance = 0.0;
    const int OCTAHEDRAL_SIZE = 8;
    float cellLuminance[64];

    for (int y = 0; y < OCTAHEDRAL_SIZE; y++)
    {
        for (int x = 0; x < OCTAHEDRAL_SIZE; x++)
        {
            int cellIndex = y * OCTAHEDRAL_SIZE + x;
            float2 octUV = (float2(x, y) + 0.5) / OCTAHEDRAL_SIZE;
            float3 cellDirection = DecodeOctahedral(octUV);

            if (dot(cellDirection, normalWS) > 0.0)
            {
                uint2 atlasCoord = GetAtlasTextureCoordinates(float2(probeIndex * TILE_SIZE), cellDirection);
                float luminance = GetLuma(in_RadianceCacheResults_Prev[atlasCoord].rgb);
                cellLuminance[cellIndex] = luminance;
                totalLuminance += luminance;
            }
            else
            {
                cellLuminance[cellIndex] = 0.0;
            }
        }
    }
    
    // If no valid reprojected data, fall back to cosine-weighted sampling
    if (totalLuminance < 0.001)
    {
        return CosineWeightedHemisphereSample(Xi, normalWS);
    }
    
    // Build cumulative distribution function
    float cdf[64];
    cdf[0] = cellLuminance[0];
    for (int i = 1; i < 64; i++)
    {
        cdf[i] = cdf[i-1] + cellLuminance[i];
    }
    
    // Normalize CDF
    float totalWeight = cdf[63];
    if (totalWeight > 0.0)
    {
        for (int i = 0; i < 64; i++)
        {
            cdf[i] /= totalWeight;
        }
    }
    
    float randomValue = Xi.x;
    int selectedCell = 0;

    for (int i = 0; i < 64; i++)
    {
        if (randomValue <= cdf[i])
        {
            selectedCell = i;
            break;
        }
    }
    
    // Convert cell back to direction with jittering within cell
    int cellX = selectedCell % OCTAHEDRAL_SIZE;
    int cellY = selectedCell / OCTAHEDRAL_SIZE;
    
    float2 cellCenter = (float2(cellX, cellY) + 0.5) / OCTAHEDRAL_SIZE;
    float2 jitteredUV = cellCenter + (Xi - 0.5) / OCTAHEDRAL_SIZE; // Jitter within cell
    jitteredUV = clamp(jitteredUV, 0.0, 1.0);
    
    float3 sampledDirection = DecodeOctahedral(jitteredUV);
    
    // Ensure we're in the correct hemisphere
    if (dot(sampledDirection, normalWS) <= 0.0)
    {
        return CosineWeightedHemisphereSample(Xi, normalWS);
    }
    
    return normalize(sampledDirection);
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

    // Fetch world space position. Paper §2.1.5: tiles without a usable
    // probe (sky, NaN position) are flagged with PROBE_MASK_INVALID so the
    // filter and downstream interpolation can skip them instead of
    // inheriting stale data from the previous frame.
    bool valid = in_opaquePassRT0.Load(int3(samplingScreenPos, 0)).w == 1.0;
    if (!valid)
    {
        in_ProbeMask[probeIndex] = PROBE_MASK_INVALID;
        return;
    }

    float3 positionWS = in_opaquePassRT0.Load(int3(samplingScreenPos, 0)).xyz;
    if (isnan(positionWS.x) || isnan(positionWS.y) || isnan(positionWS.z))
    {
        in_ProbeMask[probeIndex] = PROBE_MASK_INVALID;
        return;
    }

    // Fetch normal
    float3 normalWS = normalize(in_opaquePassRT1.Load(int3(samplingScreenPos, 0)).xyz);

    // Store probe position and normal for the next frame's reprojection pass
    in_ProbePosition[probeIndex] = float4(positionWS, 1);
    in_ProbeNormal[probeIndex] = float4(normalWS, 1);
    in_ProbeMask[probeIndex] = PackProbeMask(jitterOffset);

    const int NUM_SAMPLES = 1;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 randVal = Hash2D(samplingScreenPos, i, g_Frame.frameIndex);
        float3 sampleDir = ImportanceSampleFromCDF(randVal, normalWS, probeIndex);

        RayDesc ray;
        ray.Origin = positionWS + normalWS * 0.001;
        ray.Direction = sampleDir;
        ray.TMin = 0.01;
        ray.TMax = 1000.0;

        RayPayload tempPayload;
        tempPayload.radiance = float3(0, 0, 0);

        TraceRay(SceneAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, tempPayload);
        float NdotL = saturate(dot(normalWS, sampleDir));
        float3 radiance = tempPayload.radiance * NdotL;

        // Algorithm 3 hysteresis: firefly-reject + shadow-preserving blend.
        // The history bias (high t) only kicks in when L_new > 2*L_old,
        // otherwise the new sample is adopted fully; the paper's asymmetry
        // is intentional — favours dark (shadow) transitions while filtering
        // bright outliers from rarely-hit emissives or BRDF tails.
        uint2 texIndex = GetAtlasTextureCoordinates(float2(probeScreenPos), sampleDir);
        float3 oldScreenSpaceRadiance = in_RadianceCacheResults[texIndex].xyz;

        if (any(isnan(radiance)) || any(isinf(radiance)))
            radiance = oldScreenSpaceRadiance;

        float t = TemporalBlendAlgo3(GetLuma(radiance), GetLuma(oldScreenSpaceRadiance));
        in_RadianceCacheResults[texIndex] = float4(lerp(radiance, oldScreenSpaceRadiance, t), 1);

        // Only write to world probe grid on the first sample to avoid intra-probe write races.
        // Cross-probe hash collisions on the same cell remain a known limitation of the hash-grid approach.
        // Simple EMA here — paper §2.2.3 prescribes Karis-style exponential
        // moving average for the world cache; full two-level tiled layout
        // with proper decay is deferred to [W].
        if (i == 0)
        {
            const float WORLD_PROBE_EMA = 0.1;
            uint worldProbeIndex = ComputeProbeHash(positionWS);
            float3 oldWorldProbeRadiance = in_WorldProbeGrid[worldProbeIndex].radiance;
            in_WorldProbeGrid[worldProbeIndex].positionWS = positionWS;
            in_WorldProbeGrid[worldProbeIndex].radiance = lerp(oldWorldProbeRadiance, radiance, WORLD_PROBE_EMA);
            in_WorldProbeGrid[worldProbeIndex].weight = 1.0;
        }
    }
}
