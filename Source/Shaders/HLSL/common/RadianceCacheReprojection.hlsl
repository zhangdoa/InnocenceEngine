// shadertype=hlsl
#ifndef RADIANCE_CACHE_REPROJECTION_HLSL
#define RADIANCE_CACHE_REPROJECTION_HLSL

#include "RadianceCacheCommon.hlsl"
#include "../RayTracingTypes.hlsl"

// Per-probe LDS arrays plus accumulator / reduction body for the per-cell
// octahedral-remap radiance accumulator (Capsaicin gi1.comp:316-428).
// Included by RadianceCacheReprojection.comp; not standalone-compileable.
// The orchestrating kernel must:
//   1. Bind `in_RadianceCacheResults_Prev`, `in_ProbePosition`, `in_ProbeNormal`
//      before including this header (used inside RemapAccumulate*).
//   2. Run a numthreads(8, 8, 1) dispatch — the reduction is hard-coded to 64
//      cells per group.

// GI-1.0 §2.1.1 octahedral-remap radiance accumulator. Capsaicin
// (gi1.comp:393-403, ScreenProbes_QuantizeRadiance/RecoverRadiance) packs
// radiance + hit distance as fixed-point uints so InterlockedAdd composes
// contributions from many threads writing the same cell. HLSL has no float
// atomic; fixed-point quantize-then-add is the canonical workaround.
//
// Scales picked for the engine's value ranges:
//   radiance: HDR ~[0, 100]; QUANT=65536 → ~6.5e6 per cell, fits uint32 with
//     >600 accumulations before overflow. Per-cell contributor bound here is
//     ≤9 (Row #9 fallback: 9 neighbour probes can remap to one cell);
//     winner-path is 1 per cell. Well within safe range.
//   hitDist: ray.TMax = 1000.0 (RadianceCacheRayGen); QUANT_DIST=65536 →
//     6.5e7 per cell, also safe.
static const float RADIANCE_QUANT_SCALE = 65536.0;
static const float HIT_DIST_QUANT_SCALE = 65536.0;
static const float MAX_HIT_DISTANCE = 1000.0;

uint4 QuantizeRadiance(float4 radiance)
{
    return uint4(uint(max(radiance.x, 0.0) * RADIANCE_QUANT_SCALE),
                 uint(max(radiance.y, 0.0) * RADIANCE_QUANT_SCALE),
                 uint(max(radiance.z, 0.0) * RADIANCE_QUANT_SCALE),
                 uint(max(radiance.w, 0.0) * HIT_DIST_QUANT_SCALE));
}

float3 RecoverRadianceRGB(uint3 quantized)
{
    return float3(quantized) / RADIANCE_QUANT_SCALE;
}

float RecoverHitDist(uint quantized)
{
    return float(quantized) / HIT_DIST_QUANT_SCALE;
}

// GI-1.0 §2.1.1 per-cell history-reuse remap. Given a previous-frame probe
// at probePosPrev with a per-cell radiance + hit distance, compute where in
// the CURRENT probe's atlas the same hit point projects. The engine atlas
// is full-sphere octahedral of world-space directions (RayTracingTypes.hlsl
// EncodeOctahedral), so probe_direction needs no TBN — it is the
// world-space direction Capsaicin recovers via TBN(probe_normal).
bool RemapHistoryCell(float3 probePosPrev, float4 cellRadiancePrev,
                      float3 cellDirWS, float3 worldPos, float3 normal,
                      out uint outCellIndex, out float outHitDist)
{
    float3 hitPoint = probePosPrev + cellDirWS * cellRadiancePrev.w;
    float3 reprojDir = hitPoint - worldPos;
    float reprojLen = length(reprojDir);
    if (reprojLen < EPSILON)
    {
        outCellIndex = 0;
        outHitDist = 0.0;
        return false;
    }
    reprojDir /= reprojLen;
    if (dot(normal, reprojDir) <= 0.0)
    {
        outCellIndex = 0;
        outHitDist = 0.0;
        return false;
    }
    float2 remapUV = EncodeOctahedral(reprojDir);
    uint2 remapCell = uint2(clamp(remapUV * float(RADIANCE_CACHE_TILE_SIZE),
                                  0.0,
                                  float(RADIANCE_CACHE_TILE_SIZE - 1)));
    outCellIndex = remapCell.x + remapCell.y * RADIANCE_CACHE_TILE_SIZE;
    outHitDist = reprojLen;
    return true;
}

groupshared uint sharedReprojectionScore;
groupshared float sharedConfidence;

groupshared uint sharedRadianceValues[64 * 4];
groupshared uint sharedRadianceSampleCounts[64];

groupshared float4 sharedRadianceBackup[64];

// Winner broadcast slots. The single thread matching the InterlockedMin
// winner writes its own state here; all 64 threads then read for the
// per-cell remap step.
groupshared float3 sharedWinnerPositionWS;
groupshared float3 sharedWinnerNormalWS;
groupshared float3 sharedWinnerPrevProbePos;
groupshared int2 sharedWinnerPrevProbeTileCoord;

// NORMAL_THRESHOLD = 0.95 matches Capsaicin gi1.comp:4051.
static const float NORMAL_THRESHOLD = 0.95;
static const float DEPTH_THRESHOLD = 0.1;
static const float MIN_CONFIDENCE = 0.1;
static const float MAX_CONFIDENCE = 0.95;

float CalculateReprojectionConfidence(
    float3 currentPos, float3 currentNormal,
    float3 prevPos, float3 prevNormal,
    float2 motionVector, float depthDiff,
    float cellSize, float viewportWidth)
{
    float confidence = 1.0;
    float distancePenalty = distance(currentPos, prevPos) / max(cellSize, EPSILON);
    confidence *= exp(-distancePenalty * distancePenalty);
    confidence *= max(0.0, dot(currentNormal, prevNormal));
    float motionMagnitude = length(motionVector) / viewportWidth;
    confidence *= exp(-motionMagnitude * 10.0);
    confidence *= exp(-depthDiff * depthDiff / (DEPTH_THRESHOLD * DEPTH_THRESHOLD));
    return clamp(confidence, MIN_CONFIDENCE, MAX_CONFIDENCE);
}

// Grazing-angle boost (Capsaicin gi1.comp:4026): cellSize scales up to 5×
// as view·normal → 0; matches denoiser-pass reprojection.
float ComputeReprojectionCellSize(float3 positionWS, float3 normalWS,
                                  float3 cameraPosWS, float2 viewportSize,
                                  float4x4 projection)
{
    float depth = length(positionWS - cameraPosWS);
    float baseCellSize = max(AdaptiveCellSize(depth, viewportSize, projection), 0.1);
    float3 viewDir = normalize(cameraPosWS - positionWS);
    return baseCellSize * lerp(1.0, 5.0, pow(1.0 - max(dot(viewDir, normalWS), 0.0), 6.0));
}

void ResetReprojectionLDS(uint cellIndex)
{
    if (cellIndex == 0)
    {
        sharedReprojectionScore = 0xFFFFFFFF;
        sharedConfidence = 0.0;
    }
    sharedRadianceValues[(cellIndex << 2) + 0] = 0;
    sharedRadianceValues[(cellIndex << 2) + 1] = 0;
    sharedRadianceValues[(cellIndex << 2) + 2] = 0;
    sharedRadianceValues[(cellIndex << 2) + 3] = 0;
    sharedRadianceSampleCounts[cellIndex] = 0;
}

void AccumulateRemappedRadiance(uint cellIndex, float3 radianceRGB, float hitDist)
{
    uint4 quantized = QuantizeRadiance(float4(radianceRGB, hitDist));
    InterlockedAdd(sharedRadianceValues[(cellIndex << 2) + 0], quantized.x);
    InterlockedAdd(sharedRadianceValues[(cellIndex << 2) + 1], quantized.y);
    InterlockedAdd(sharedRadianceValues[(cellIndex << 2) + 2], quantized.z);
    InterlockedAdd(sharedRadianceValues[(cellIndex << 2) + 3], quantized.w);
    InterlockedAdd(sharedRadianceSampleCounts[cellIndex], 1);
}

void SeedRadianceBackup(uint cellIndex)
{
    uint sampleCount = sharedRadianceSampleCounts[cellIndex];
    float3 recoveredRGB = RecoverRadianceRGB(uint3(
        sharedRadianceValues[(cellIndex << 2) + 0],
        sharedRadianceValues[(cellIndex << 2) + 1],
        sharedRadianceValues[(cellIndex << 2) + 2]));
    sharedRadianceBackup[cellIndex] = float4(recoveredRGB, sampleCount > 0 ? 1.0 : 0.0);
}

// Hillis-Steele up-sweep over the 64 cells (Capsaicin gi1.comp:414-419).
// All 64 threads participate in every barrier even though only a shrinking
// subset writes — shader-standards skill.
void ReduceRadianceBackup(uint cellIndex)
{
    [unroll]
    for (uint stride = 1; stride < 64; stride <<= 1)
    {
        bool active = cellIndex < (64 / (2 * stride));
        float4 a = sharedRadianceBackup[(2 * cellIndex + 1) * stride - 1];
        float4 b = sharedRadianceBackup[2 * (cellIndex + 1) * stride - 1];
        if (active)
        {
            sharedRadianceBackup[2 * (cellIndex + 1) * stride - 1] = a + b;
        }
        GroupMemoryBarrierWithGroupSync();
    }
}

void FinalizeRadianceBackup(uint cellIndex)
{
    if (cellIndex == 0)
    {
        float4 total = sharedRadianceBackup[63];
        float3 avgRadiance = total.xyz / max(total.w, 1.0);
        float emptyCount = 64.0 - total.w;
        sharedRadianceBackup[0] = float4(avgRadiance / max(emptyCount, 1.0), MAX_HIT_DISTANCE);
    }
}

#endif // RADIANCE_CACHE_REPROJECTION_HLSL
