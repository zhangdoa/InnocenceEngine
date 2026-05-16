// shadertype=hlsli
// GI-1.0 §2.1.3 ray-guiding helpers — extracted from RadianceCacheRayGen.hlsl
// to keep the file under the 300-line gate while TASK-226.6 (Capsaicin-port
// 64-rays-per-probe + workgroup-parallel CDF scan) is pending. TASK-226.6
// rewrites the kernel's CDF construction; these helpers either get folded
// back into the rewritten kernel or removed entirely. Treat as transient.
#ifndef RADIANCE_CACHE_RAYGEN_HEMISPHERE_CDF_HLSLI
#define RADIANCE_CACHE_RAYGEN_HEMISPHERE_CDF_HLSLI

float3 CosineWeightedHemisphereSample(float2 Xi, float3 N)
{
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt(Xi.y);
    float sinTheta = sqrt(1.0 - Xi.y);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    float3x3 basis = CreateTangentSpace(N);
    return normalize(mul(H, basis));
}

// GI-1.0 §2.1.3 ray guiding — 3×3 probe-neighbourhood hemisphere
// reconstruction with parallax correction (Figure 6). For each neighbour
// probe: locate its shading position from G-buffer at the neighbour tile's
// stored sub-pixel anchor; iterate its 8×8 octahedral cells; reproject each
// non-empty cell's hit into the current probe's frame via hit_ws =
// neighbourPos + distance · cellDir, dir_new = normalize(hit_ws - posWS);
// scatter luminance into the current probe's CDF cell. Rejects neighbours
// beyond adaptive_cell_size × 3 and sky pixels.
// Build once per probe per frame; SampleHemisphereImportanceCDF does the
// thin per-sample binary search.
static const int OCTAHEDRAL_SIZE = 8;
static const int OCTAHEDRAL_CELL_COUNT = OCTAHEDRAL_SIZE * OCTAHEDRAL_SIZE;

struct HemisphereCDF
{
    float cdf[OCTAHEDRAL_CELL_COUNT];
    float totalLuminance;       // 0 ⇒ no usable reconstruction; sample falls back to cosine-weighted
};

HemisphereCDF BuildHemisphereImportanceCDF(float3 normalWS, uint2 probeIndex, float3 positionWS)
{
    HemisphereCDF result;
    float cellLuminance[OCTAHEDRAL_CELL_COUNT];
    [unroll]
    for (int init = 0; init < OCTAHEDRAL_CELL_COUNT; init++)
        cellLuminance[init] = 0.0;
    result.totalLuminance = 0.0;

    int2 gridSize = int2(uint2(g_Frame.viewportSize.xy) / RADIANCE_CACHE_TILE_SIZE);
    float currentDepth = length(positionWS - g_Frame.camera_posWS.xyz);
    float cellSize = max(AdaptiveCellSize(currentDepth, g_Frame.viewportSize.xy, g_Frame.p_original), 0.1);
    float maxNeighbourDist = cellSize * 3.0;

    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int2 neighbourIdx = int2(probeIndex) + int2(dx, dy);
            if (any(neighbourIdx < int2(0, 0)) || any(neighbourIdx >= gridSize))
                continue;

            // Each neighbour was anchored at its own Halton sub-tile pixel
            // in whichever frame last spawned it; use the mask's stored
            // sub-pixel to locate its G-buffer position this frame.
            uint neighbourMask = in_ProbeMask[uint2(neighbourIdx)];
            if (!IsValidProbe(neighbourMask))
                continue;
            uint2 neighbourSubPixel = UnpackProbeMask(neighbourMask);

            int2 neighbourAnchor = neighbourIdx * int(RADIANCE_CACHE_TILE_SIZE) + int2(neighbourSubPixel);
            float4 neighbourRT0 = in_opaquePassRT0.Load(int3(neighbourAnchor, 0));
            if (neighbourRT0.w == 0.0)
                continue;
            float3 neighbourPos = neighbourRT0.xyz;
            if (distance(neighbourPos, positionWS) > maxNeighbourDist)
                continue;

            int2 neighbourScreenPos = neighbourIdx * int(RADIANCE_CACHE_TILE_SIZE);

            for (int cy = 0; cy < OCTAHEDRAL_SIZE; cy++)
            {
                for (int cx = 0; cx < OCTAHEDRAL_SIZE; cx++)
                {
                    float2 octUV_n = (float2(cx, cy) + 0.5) / float(OCTAHEDRAL_SIZE);
                    float3 dir_n = DecodeOctahedral(octUV_n);

                    int2 atlasCoord = neighbourScreenPos + int2(cx, cy);
                    float4 neighbourSample = in_RadianceCacheResults_Prev.Load(int3(atlasCoord, 0));
                    float L = GetLuma(neighbourSample.rgb);
                    if (L < 0.001)
                        continue;
                    float d = neighbourSample.a;

                    float3 hitWS = neighbourPos + d * dir_n;
                    float3 dir_new = hitWS - positionWS;
                    float len = length(dir_new);
                    if (len < EPSILON)
                        continue;
                    dir_new /= len;

                    if (dot(dir_new, normalWS) <= 0.0)
                        continue;

                    float2 octUV_new = EncodeOctahedral(dir_new);
                    int2 newCellXY = clamp(int2(octUV_new * float(OCTAHEDRAL_SIZE)), int2(0, 0), int2(OCTAHEDRAL_SIZE - 1, OCTAHEDRAL_SIZE - 1));
                    int newCellIndex = newCellXY.y * OCTAHEDRAL_SIZE + newCellXY.x;

                    cellLuminance[newCellIndex] += L;
                    result.totalLuminance += L;
                }
            }
        }
    }

    // Prefix sum; normalisation deferred to sample call (one div / sample
    // is cheaper than 64 here; sampler uses `Xi.x * cdf[63]` directly).
    result.cdf[0] = cellLuminance[0];
    [unroll]
    for (int i = 1; i < OCTAHEDRAL_CELL_COUNT; i++)
        result.cdf[i] = result.cdf[i - 1] + cellLuminance[i];

    return result;
}

float3 SampleHemisphereImportanceCDF(float2 Xi, float3 normalWS, HemisphereCDF cdf)
{
    if (cdf.totalLuminance < 0.001)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    float totalWeight = cdf.cdf[OCTAHEDRAL_CELL_COUNT - 1];
    if (totalWeight <= 0.0)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    float threshold = Xi.x * totalWeight;
    int selectedCell = OCTAHEDRAL_CELL_COUNT - 1;
    for (int k = 0; k < OCTAHEDRAL_CELL_COUNT; k++)
    {
        if (threshold <= cdf.cdf[k])
        {
            selectedCell = k;
            break;
        }
    }

    int cellX = selectedCell % OCTAHEDRAL_SIZE;
    int cellY = selectedCell / OCTAHEDRAL_SIZE;
    float2 cellCenter = (float2(cellX, cellY) + 0.5) / float(OCTAHEDRAL_SIZE);
    float2 jitteredUV = cellCenter + (Xi - 0.5) / float(OCTAHEDRAL_SIZE);
    jitteredUV = clamp(jitteredUV, 0.0, 1.0);

    float3 sampledDirection = DecodeOctahedral(jitteredUV);
    if (dot(sampledDirection, normalWS) <= 0.0)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    return normalize(sampledDirection);
}

// 4×4 stratified Halton(2,3): 16 per-probe samples each in a distinct
// stratum, sub-stratum jitter via Halton indexed by
// (frameIndex * NUM_SAMPLES_PER_PROBE + sampleIndex). Stratification
// guarantees ≥ 4 distinct CDF buckets per frame even when one cell
// dominates — prevents 16× the same direction.
static const int STRATA_GRID_DIM = 4;
static const int NUM_SAMPLES_PER_PROBE = STRATA_GRID_DIM * STRATA_GRID_DIM;

float2 StratifiedHaltonSample(uint sampleIndex, uint frameIndex)
{
    uint stratumX = sampleIndex % uint(STRATA_GRID_DIM);
    uint stratumY = sampleIndex / uint(STRATA_GRID_DIM);
    uint haltonIndex = frameIndex * uint(NUM_SAMPLES_PER_PROBE) + sampleIndex + 1u;
    float jitterX = Halton(haltonIndex, 2u);
    float jitterY = Halton(haltonIndex, 3u);
    float invDim = 1.0 / float(STRATA_GRID_DIM);
    return float2((float(stratumX) + jitterX) * invDim,
                  (float(stratumY) + jitterY) * invDim);
}

#endif // RADIANCE_CACHE_RAYGEN_HEMISPHERE_CDF_HLSLI
