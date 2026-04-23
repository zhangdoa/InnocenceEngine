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
float3 ImportanceSampleFromCDF(float2 Xi, float3 normalWS, uint2 probeIndex, float3 positionWS)
{
    const int OCTAHEDRAL_SIZE = 8;
    float cellLuminance[64];
    [unroll]
    for (int init = 0; init < 64; init++)
        cellLuminance[init] = 0.0;
    float totalLuminance = 0.0;

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
                    totalLuminance += L;
                }
            }
        }
    }

    // No usable reconstructed hemisphere → cosine-weighted fallback.
    if (totalLuminance < 0.001)
        return CosineWeightedHemisphereSample(Xi, normalWS);

    // Build CDF over current probe's cells, sample via Xi.x.
    float cdf[64];
    cdf[0] = cellLuminance[0];
    for (int i = 1; i < 64; i++)
        cdf[i] = cdf[i - 1] + cellLuminance[i];

    float totalWeight = cdf[63];
    if (totalWeight > 0.0)
    {
        [unroll]
        for (int j = 0; j < 64; j++)
            cdf[j] /= totalWeight;
    }

    float randomValue = Xi.x;
    int selectedCell = 0;
    for (int k = 0; k < 64; k++)
    {
        if (randomValue <= cdf[k])
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

    // [S1.5c] Paper Algorithm 2 — empty-tile priority. Scan the ξ_x·ξ_y
    // probe tiles inside our spawn tile; if any is flagged INVALID by
    // Reprojection (disoccluded this frame), redirect the spawn there
    // instead of letting the Halton pick land on an already-valid tile.
    // Without this, disoccluded tiles have to wait for the Halton cycle
    // to come around — up to ξ_x·ξ_y frames of visible dark patches
    // under fast motion. Sub-pixel inside the redirected tile uses the
    // Halton offset so repeated disocclusions still get varied jitter.
    //
    // Paper's full Algorithm 2 also routes EXTRA rays to high-variance
    // tiles via an override-queue (ray stealing from well-reprojected
    // neighbours). Out of scope here; ray budget stays constant at one
    // spawn per spawn tile.
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

    const int NUM_SAMPLES = 1;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 randVal = Hash2D(samplingScreenPos, i, g_Frame.frameIndex);
        float3 sampleDir = ImportanceSampleFromCDF(randVal, normalWS, probeIndex, positionWS);

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
