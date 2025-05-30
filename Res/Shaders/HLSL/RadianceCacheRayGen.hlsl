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

// Generate stable random values without temporal rotation
float2 StableHash2D(uint2 pixelID, uint sampleIndex)
{
    // Use stable hash without frame dependency
    uint n = pixelID.x * 73856093u ^ pixelID.y * 19349663u ^ sampleIndex * 83492791u;
    n = (n << 13u) ^ n;
    return float2(
        (n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffff,
        (n * (n * n * 12347u + 45679u) + 987654321u) & 0x7fffffff
    ) / float(0x7fffffff);
}

// Sample from cumulative distribution function of reprojected radiance
float3 ImportanceSampleFromCDF(float2 Xi, float3 normalWS, uint2 probeIndex)
{
    // Try to find valid reprojected radiance data
    float totalLuminance = 0.0;
    float maxLuminance = 0.0;
    
    // Calculate luminance for each octahedral cell from previous frame
    const int OCTAHEDRAL_SIZE = 8;
    float cellLuminance[64]; // 8x8 octahedral grid
    
    for (int y = 0; y < OCTAHEDRAL_SIZE; y++)
    {
        for (int x = 0; x < OCTAHEDRAL_SIZE; x++)
        {
            int cellIndex = y * OCTAHEDRAL_SIZE + x;
            
            // Convert cell to direction
            float2 octUV = (float2(x, y) + 0.5) / OCTAHEDRAL_SIZE;
            float3 cellDirection = DecodeOctahedral(octUV);
            
            // Only consider hemisphere above surface
            if (dot(cellDirection, normalWS) > 0.0)
            {
                uint2 atlasCoord = GetAtlasTextureCoordinates(float2(probeIndex * TILE_SIZE), cellDirection);
                float3 reprojectedRadiance = in_RadianceCacheResults_Prev[atlasCoord].rgb;
                
                float luminance = GetLuma(reprojectedRadiance);
                cellLuminance[cellIndex] = luminance;
                totalLuminance += luminance;
                maxLuminance = max(maxLuminance, luminance);
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
    
    // Sample from CDF
    float randomValue = Xi.x;
    int selectedCell = 0;
    
    // Find cell using binary search
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

    // Stable temporal sampling without rotation - key fix for flickering
    const int NUM_SAMPLES = 4; // Reduced for better temporal distribution
    float3 totalRadiance = 0;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        // CRITICAL FIX: Use stable sampling without frame rotation
        // This prevents the temporal instability that caused flickering
        float2 randVal = StableHash2D(samplingScreenPos, i);
        
        // Use importance sampling from reprojected radiance when available
        float3 sampleDir = ImportanceSampleFromCDF(randVal, normalWS, probeIndex);

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

        // Store in radiance cache with improved temporal accumulation
        uint2 texIndex = GetAtlasTextureCoordinates(float2(probeScreenPos), sampleDir);
        float3 oldScreenSpaceRadiance = in_RadianceCacheResults[texIndex].xyz;

        // Adaptive temporal blending based on luminance
        float currentLuminance = GetLuma(radiance);
        float oldLuminance = GetLuma(oldScreenSpaceRadiance);
        
        // Aggressive denoising for dark areas
        float baseTemporal = 0.15;
        if (currentLuminance < 0.1) // Dark areas need more aggressive filtering
        {
            baseTemporal = 0.05; // Much slower convergence but smoother
        }
        else if (currentLuminance > 2.0) // Bright areas can converge faster
        {
            baseTemporal = 0.25;
        }
        
        float temporalWeight = baseTemporal;
        
        // Variance-based adjustment
        float3 radianceDiff = abs(radiance - oldScreenSpaceRadiance);
        float variance = (radianceDiff.r + radianceDiff.g + radianceDiff.b) / 3.0;
        
        // Enhanced firefly suppression with luminance-based clamping
        if (variance > 0.5)
        {
            temporalWeight *= 0.2; // Strong suppression for high variance
            
            // Adaptive clamping based on old luminance
            float maxAllowedChange = max(oldLuminance * 1.5, 0.5);
            radiance = min(radiance, float3(maxAllowedChange, maxAllowedChange, maxAllowedChange));
        }
        
        // NaN protection
        if (any(isnan(radiance)) || any(isinf(radiance)))
        {
            radiance = oldScreenSpaceRadiance;
            temporalWeight = 0.0;
        }
        
        in_RadianceCacheResults[texIndex] = float4(lerp(oldScreenSpaceRadiance, radiance, temporalWeight), 1);

        uint worldProbeIndex = ComputeProbeHash(positionWS);
        float3 oldWorldProbeRadiance = in_WorldProbeGrid[worldProbeIndex].radiance;
        in_WorldProbeGrid[worldProbeIndex].positionWS = positionWS;

        // Apply same conservative blending to world probe grid
        in_WorldProbeGrid[worldProbeIndex].radiance = lerp(oldWorldProbeRadiance, radiance, temporalWeight);
        in_WorldProbeGrid[worldProbeIndex].weight = 1.0;
    }
}
