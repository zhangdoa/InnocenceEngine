// shadertype=hlsl
#include "RayTracingBindings.hlsl"
#include "common/SSRCCommon.hlsl"
#include "SSRCRayGen_HemisphereCDF.hlsli"


[shader("raygeneration")]
void RayGenShader()
{
    // GI-1.0 §2.1.1 sparse spawn: dispatch is sized at spawn-tile granularity
    // (viewport / (8·ξ_x, 8·ξ_y)). Each thread owns one spawn tile and spawns
    // exactly one probe inside it per frame, at a Halton-picked pixel. The
    // remaining (ξ_x·ξ_y - 1) probe slots inside the spawn tile inherit
    // whatever Reprojection has for them — reprojected radiance if the
    // temporal reuse succeeded, PROBE_MASK_INVALID if it didn't.
    // The render graph drives this RTPSO with a full-screen DispatchRays (the
    // recorder can't size a ray dispatch at spawn-tile granularity). Gate to the
    // spawn-tile-aligned threads and remap to the paper's viewport/(8*xi) sparse
    // spawn index; the other threads in each spawn tile early-out.
    uint2 fullDispatchIndex = DispatchRaysIndex().xy;
    if ((fullDispatchIndex.x % spawnTileSize.x) != 0u || (fullDispatchIndex.y % spawnTileSize.y) != 0u)
        return;
    uint2 spawnIndex = fullDispatchIndex / spawnTileSize;
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
                float L0 = GetLuma(in_SSRCResults[probeScreen + uint2(0, 0)].rgb);
                float L1 = GetLuma(in_SSRCResults[probeScreen + uint2(7, 0)].rgb);
                float L2 = GetLuma(in_SSRCResults[probeScreen + uint2(0, 7)].rgb);
                float L3 = GetLuma(in_SSRCResults[probeScreen + uint2(7, 7)].rgb);
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
        float3 oldScreenSpaceRadiance = in_SSRCResults[texIndex].xyz;

        if (any(isnan(radiance)) || any(isinf(radiance)))
            radiance = oldScreenSpaceRadiance;

        float t = TemporalBlendAlgo3(GetLuma(radiance), GetLuma(oldScreenSpaceRadiance));
        // Alpha = ray travel distance along sampleDir. Consumed next frame
        // during ray-guiding reconstruction to parallax-correct the
        // reused cell direction (paper §2.1.3).
        in_SSRCResults[texIndex] = float4(lerp(radiance, oldScreenSpaceRadiance, t), tempPayload.distance);
    }
}
