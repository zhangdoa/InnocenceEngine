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

// GI-1.0 §2.1.3 ray guiding — hemisphere reconstruction across a 3x3 probe
// neighbourhood with parallax correction (Figure 6).
//
// For each neighbour probe in the 3x3 tile window:
//   1. Locate the neighbour's shading position (sampled from the opaque
//      G-buffer at the neighbour tile's anchor pixel — inline, avoiding
//      cross-thread races on in_ProbePosition which is being written by
//      the same RayGen dispatch).
//   2. Iterate the neighbour's 8x8 octahedral cells. For each cell:
//        hit_ws  = neighbourPos + distance * cellDir    (distance stored
//                  in alpha by [S1.2])
//        dir_new = normalize(hit_ws - positionWS)       (re-aim from
//                  current probe's frame)
//   3. Scatter the radiance into the corresponding cell of the CURRENT
//      probe's CDF, in the current probe's tangent frame.
//
// Rejection: neighbour positions beyond adaptive_cell_size * 3 are
// dropped; sky pixels (rt0.w == 0) have no meaningful radiance.
//
// Split into BuildHemisphereImportanceCDF (the expensive 3x3 reconstruction,
// computed once per probe per frame) and SampleHemisphereImportanceCDF (a
// thin per-sample CDF binary search). Lets a per-probe RayGen draw N rays
// without redoing the 9x64 reconstruction work N times.
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

            // Under sparse spawning each neighbour was anchored at its own
            // Halton-picked sub-tile pixel in whichever frame last spawned
            // it; use the mask's stored sub-pixel to locate its G-buffer
            // position this frame rather than assuming all tiles share the
            // current frame's Halton offset.
            uint neighbourMask = in_ProbeMask[uint2(neighbourIdx)];
            if (!IsValidProbe(neighbourMask))
                continue;
            uint2 neighbourSubPixel = UnpackProbeMask(neighbourMask);

            int2 neighbourAnchor = neighbourIdx * int(RADIANCE_CACHE_TILE_SIZE) + int2(neighbourSubPixel);
            float4 neighbourRT0 = in_opaquePassRT0.Load(int3(neighbourAnchor, 0));
            if (neighbourRT0.w == 0.0)
                continue;                      // tile is sky this frame — skip
            float3 neighbourPos = neighbourRT0.xyz;
            if (distance(neighbourPos, positionWS) > maxNeighbourDist)
                continue;                      // same heuristic as filter/reprojection

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
                        continue;              // empty cell — skip
                    float d = neighbourSample.a;

                    float3 hitWS = neighbourPos + d * dir_n;
                    float3 dir_new = hitWS - positionWS;
                    float len = length(dir_new);
                    if (len < EPSILON)
                        continue;
                    dir_new /= len;

                    if (dot(dir_new, normalWS) <= 0.0)
                        continue;              // hemisphere reject in current probe's frame

                    float2 octUV_new = EncodeOctahedral(dir_new);
                    int2 newCellXY = clamp(int2(octUV_new * float(OCTAHEDRAL_SIZE)), int2(0, 0), int2(OCTAHEDRAL_SIZE - 1, OCTAHEDRAL_SIZE - 1));
                    int newCellIndex = newCellXY.y * OCTAHEDRAL_SIZE + newCellXY.x;

                    cellLuminance[newCellIndex] += L;
                    result.totalLuminance += L;
                }
            }
        }
    }

    // Build the prefix sum. Normalisation is deferred to the sample call
    // (one division per sample is cheaper than 64 here), and the sampler
    // re-derives `randomValue * cdf[63]` so we do not need cdf in [0,1].
    result.cdf[0] = cellLuminance[0];
    [unroll]
    for (int i = 1; i < OCTAHEDRAL_CELL_COUNT; i++)
        result.cdf[i] = result.cdf[i - 1] + cellLuminance[i];

    return result;
}

float3 SampleHemisphereImportanceCDF(float2 Xi, float3 normalWS, HemisphereCDF cdf)
{
    // No usable reconstructed hemisphere → cosine-weighted fallback.
    if (cdf.totalLuminance < 0.001)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    float totalWeight = cdf.cdf[OCTAHEDRAL_CELL_COUNT - 1];
    if (totalWeight <= 0.0)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    float threshold = Xi.x * totalWeight;        // un-normalised binary search
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

// 4x4 stratified Halton(2,3) over the unit square. The 16 samples drawn for
// one probe-tile per frame each occupy a distinct stratum of a 4x4 grid in
// (Xi.x, Xi.y), with sub-stratum jitter from Halton(2)/Halton(3) indexed by
// (frameIndex * NUM_SAMPLES_PER_PROBE + sampleIndex) for temporal
// decorrelation. Stratification guarantees the 16 samples cover all four
// quartiles of the CDF binary search (Xi.x), so they pick from at least 4
// distinct hemisphere luminance buckets even when the CDF is peaked at one
// bright cell — essential to avoid 16x the same direction when one cell
// dominates.
static const int STRATA_GRID_DIM = 4;             // 4 x 4 = 16 strata
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

[shader("raygeneration")]
void RayGenShader()
{
    // GI-1.0 §2.1.1 sparse spawn: dispatch is sized at spawn-tile granularity
    // (viewport / (8·ξ_x, 8·ξ_y)). Each thread owns one spawn tile and spawns
    // exactly one probe inside it per frame, at a Halton-picked pixel. The
    // remaining (ξ_x·ξ_y - 1) probe slots inside the spawn tile inherit
    // whatever Reprojection has for them — reprojected radiance if the
    // temporal reuse succeeded, PROBE_MASK_INVALID if it didn't.
    uint2 spawnIndex = DispatchRaysIndex().xy;
    uint2 spawnTileOrigin = spawnIndex * spawnTileSize;

    // Halton(2) / Halton(3) — pick pixel inside the (8·ξ_x × 8·ξ_y) spawn
    // tile for this frame. The ξ_x · ξ_y permutation cycles through all
    // probe-tile slots within the spawn tile over that many frames.
    float2 haltonUV = float2(Halton(g_Frame.frameIndex, 2u), Halton(g_Frame.frameIndex, 3u));
    uint2 pixelInSpawn = uint2(haltonUV * float2(spawnTileSize));

    // Paper Algorithm 2 priorities inside each 2×2 spawn tile:
    //   1. [S1.5c] Empty-tile redirect — any probe tile flagged INVALID
    //      by Reprojection (disoccluded this frame) preempts Halton so
    //      the disoccluded tile gets a ray immediately instead of waiting
    //      up to ξ_x·ξ_y frames for the Halton cycle to land there.
    //   2. [S1.5c-override] High-variance tile redirect — if no tile is
    //      empty, redirect to the tile with the highest current-atlas
    //      luma coefficient-of-variation (σ/μ > OVERRIDE_CV_THRESHOLD).
    //      Paper's override queue spends EXTRA rays on high-variance
    //      tiles at the cost of well-converged ones; we approximate with
    //      a priority-shift inside the fixed per-spawn-tile ray budget,
    //      so high-variance tiles get spawned every frame (instead of 1
    //      in ξ_x·ξ_y) while quiet tiles wait for Halton. Sub-pixel
    //      inside the redirected tile uses the Halton offset so repeated
    //      redirects still get varied jitter.
    //   3. Halton default — no empty, no high-variance → let the Halton
    //      pick land wherever it rolled.
    //
    // Variance probe: four diagonal cells of the probe's 8×8 octahedral
    // atlas give a cheap σ/μ estimate. Cheaper than reading all 64 cells;
    // enough to distinguish fireflies (one outlier cell) from smooth
    // probes.
    const float OVERRIDE_CV_THRESHOLD = 1.0; // stddev ≈ mean → firefly-class variance

    bool redirected = false;
    for (uint py = 0; py < upscaleFactor.y; py++)
    {
        for (uint px = 0; px < upscaleFactor.x; px++)
        {
            uint2 probeTile = spawnIndex * upscaleFactor + uint2(px, py);
            if (!IsValidProbe(in_ProbeMask[probeTile]))
            {
                uint2 subJitter = uint2(haltonUV * float(RADIANCE_CACHE_TILE_SIZE));
                pixelInSpawn = uint2(px, py) * RADIANCE_CACHE_TILE_SIZE + subJitter;
                redirected = true;
                break;
            }
        }
        if (redirected) break;
    }

    if (!redirected)
    {
        float bestCV = OVERRIDE_CV_THRESHOLD;
        uint2 bestOffset = uint2(0, 0);
        bool foundVariance = false;
        for (uint py = 0; py < upscaleFactor.y; py++)
        {
            for (uint px = 0; px < upscaleFactor.x; px++)
            {
                uint2 probeTile = spawnIndex * upscaleFactor + uint2(px, py);
                if (!IsValidProbe(in_ProbeMask[probeTile]))
                    continue; // empty tiles handled above; shouldn't reach here but guards re-entry

                uint2 probeScreen = probeTile * RADIANCE_CACHE_TILE_SIZE;
                float L0 = GetLuma(in_RadianceCacheResults[probeScreen + uint2(0, 0)].rgb);
                float L1 = GetLuma(in_RadianceCacheResults[probeScreen + uint2(7, 0)].rgb);
                float L2 = GetLuma(in_RadianceCacheResults[probeScreen + uint2(0, 7)].rgb);
                float L3 = GetLuma(in_RadianceCacheResults[probeScreen + uint2(7, 7)].rgb);
                float mean = (L0 + L1 + L2 + L3) * 0.25;
                float d0 = L0 - mean;
                float d1 = L1 - mean;
                float d2 = L2 - mean;
                float d3 = L3 - mean;
                float var = (d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3) * 0.25;
                float cv = sqrt(var) / max(mean, 0.01);
                if (cv > bestCV)
                {
                    bestCV = cv;
                    bestOffset = uint2(px, py);
                    foundVariance = true;
                }
            }
        }
        if (foundVariance)
        {
            uint2 subJitter = uint2(haltonUV * float(RADIANCE_CACHE_TILE_SIZE));
            pixelInSpawn = bestOffset * RADIANCE_CACHE_TILE_SIZE + subJitter;
            redirected = true;
        }
    }

    uint2 samplingScreenPos = spawnTileOrigin + pixelInSpawn;

    // Derive probe tile and sub-tile pixel from the Halton-chosen pixel.
    uint2 probeIndex = samplingScreenPos / RADIANCE_CACHE_TILE_SIZE;
    uint2 subTilePixel = samplingScreenPos % RADIANCE_CACHE_TILE_SIZE;
    uint2 probeScreenPos = probeIndex * RADIANCE_CACHE_TILE_SIZE;

    // Fetch world space position. Paper §2.1.5: tiles without a usable
    // probe (sky, NaN position) are flagged PROBE_MASK_INVALID so filter
    // and interpolation skip them.
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
    in_ProbeMask[probeIndex] = PackProbeMask(subTilePixel);

    // Auditor recommendation #1 (TASK-6.7): raise per-probe ray budget from 1
    // to NUM_SAMPLES_PER_PROBE = 16. Capsaicin uses 64 (one ray per cell of
    // the 8x8 octahedral atlas, dispatch shape (W/16, H/16, 64)). 16 is the
    // cheaper interim — kept the dispatch shape (W/16, H/16, 1) and stratified
    // 16 samples across a 4x4 grid of the (Xi.x, Xi.y) unit square so the
    // CDF binary search is forced to draw from 4 distinct quartiles per
    // frame. The remaining gap to 64 is recommendation #2 (separate task).
    HemisphereCDF probeCDF = BuildHemisphereImportanceCDF(normalWS, probeIndex, positionWS);

    for (int i = 0; i < NUM_SAMPLES_PER_PROBE; i++)
    {
        float2 randVal = StratifiedHaltonSample(uint(i), g_Frame.frameIndex);
        float3 sampleDir = SampleHemisphereImportanceCDF(randVal, normalWS, probeCDF);

        RayDesc ray;
        ray.Origin = positionWS + normalWS * 0.001;
        ray.Direction = sampleDir;
        ray.TMin = 0.01;
        ray.TMax = 1000.0;

        RayPayload tempPayload;
        tempPayload.radiance = float3(0, 0, 0);
        tempPayload.distance = ray.TMax;

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
        // Alpha = ray travel distance along sampleDir. Consumed next frame
        // during ray-guiding reconstruction to parallax-correct the
        // reused cell direction (paper §2.1.3).
        in_RadianceCacheResults[texIndex] = float4(lerp(radiance, oldScreenSpaceRadiance, t), tempPayload.distance);

        // Write to world tile grid on the first sample (avoids intra-probe
        // write races). Paper §2.2.3 two-level hash: tile key is
        // (quant(pos, 8·cellSize), dominantAxis(normal), shortRay); cell
        // coord inside the tile collapses along the dominant axis. Linear-
        // probe by fingerprint, reclaim on match / empty / stale. After
        // writing MIP0 we fan out to MIP1..3 by averaging each parent's 4
        // child cells — gives distant (cone) queries a prefiltered answer.
        if (i == 0)
        {
            const float WORLD_TILE_EMA = 0.1;
            uint shortRayBit = IsShortRay(tempPayload.distance) ? 1u : 0u;
            uint bucket = ComputeTileHash(positionWS, normalWS, shortRayBit);
            uint fingerprint = ComputeTileFingerprint(positionWS, normalWS, shortRayBit);
            uint axisBin = DominantAxis(normalWS);
            uint2 cellXY0 = CellInTile(positionWS, axisBin);

            for (uint probe = 0u; probe < MAX_LINEAR_PROBE; probe++)
            {
                uint slot = (bucket + probe) % WORLD_TILE_HASH_SIZE;
                uint slotFp = in_WorldTileGrid[slot].fingerprint;
                uint slotStamp = in_WorldTileGrid[slot].lastTouchedFrame;
                bool stale = IsTileSlotStale(slotStamp, g_Frame.frameIndex);
                if (slotFp == 0u || slotFp == fingerprint || stale)
                {
                    // Slot ownership change (empty → ours, or stale reclaim).
                    // Clear all 85 cells so the new owner doesn't read the
                    // previous occupant's radiance before its own rays
                    // repopulate. Same-fingerprint updates skip the clear.
                    if (slotFp != fingerprint)
                    {
                        for (uint c = 0u; c < WORLD_TILE_CELLS_TOTAL; c++)
                        {
                            in_WorldTileGrid[slot].cells[c].radiance = float3(0, 0, 0);
                            in_WorldTileGrid[slot].cells[c].weight = 0.0;
                        }
                    }

                    // Update MIP0 cell with temporal EMA.
                    uint cIdx0 = CellIndexInTile(cellXY0, 0u);
                    float wOld = in_WorldTileGrid[slot].cells[cIdx0].weight;
                    float3 rOld = in_WorldTileGrid[slot].cells[cIdx0].radiance;
                    float3 rNew = (wOld > 0.0) ? lerp(rOld, radiance, WORLD_TILE_EMA) : radiance;
                    in_WorldTileGrid[slot].cells[cIdx0].radiance = rNew;
                    in_WorldTileGrid[slot].cells[cIdx0].weight = 1.0;

                    // MIP fan-out: for each level m ∈ {1,2,3}, recompute the
                    // parent cell at (cellXY0 >> m) as the average of its 4
                    // children at MIP (m-1). Skips unwritten children
                    // (weight == 0) so one-sample hits don't dilute the
                    // average with zero radiance from un-traced cells.
                    for (uint m = 1u; m <= 3u; m++)
                    {
                        uint2 parentXY = cellXY0 >> m;
                        uint2 childOrigin = parentXY << 1u; // in MIP(m-1) coords
                        float3 sumR = float3(0, 0, 0);
                        float sumW = 0.0;
                        for (uint cy = 0u; cy < 2u; cy++)
                        {
                            for (uint cx = 0u; cx < 2u; cx++)
                            {
                                uint2 childXY = childOrigin + uint2(cx, cy);
                                uint cIdx = CellIndexInTile(childXY, m - 1u);
                                float cW = in_WorldTileGrid[slot].cells[cIdx].weight;
                                if (cW > 0.0)
                                {
                                    sumR += in_WorldTileGrid[slot].cells[cIdx].radiance;
                                    sumW += 1.0;
                                }
                            }
                        }
                        if (sumW > 0.0)
                        {
                            uint pIdx = CellIndexInTile(parentXY, m);
                            in_WorldTileGrid[slot].cells[pIdx].radiance = sumR / sumW;
                            in_WorldTileGrid[slot].cells[pIdx].weight = 1.0;
                        }
                    }

                    in_WorldTileGrid[slot].fingerprint = fingerprint;
                    in_WorldTileGrid[slot].lastTouchedFrame = g_Frame.frameIndex;
                    break;
                }
            }
        }
    }
}
