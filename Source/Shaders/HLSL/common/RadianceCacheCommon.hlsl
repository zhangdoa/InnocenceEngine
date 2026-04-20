// shadertype=hlsl
#ifndef RADIANCE_CACHE_COMMON_HLSL
#define RADIANCE_CACHE_COMMON_HLSL

#include "common.hlsl"

// GI-1.0 (Boissé et al., AMD, 2022) primitives shared across the radiance
// cache passes. See Documents/radiance-cache-roadmap.md for the sub-project
// ordering; this header covers the [F] Foundation slice.

// Paper §2.1: screen probes tile the framebuffer in 8x8 pixel blocks, one
// probe per tile. The same constant is the pixel spacing between probe
// anchors that feeds Algorithm 6 (adaptive cell size).
static const uint RADIANCE_CACHE_TILE_SIZE = 8;

// Probe-mask encoding. Per paper §2.1.5 each tile stores the 32-bit encoded
// sub-tile pixel coordinate of its spawned probe. Bit 31 is the VALID flag
// so all-zero (uninitialised / cleared) memory correctly reads as INVALID,
// removing the need for a per-frame mask clear under sparse spawning where
// most tiles inherit their mask from a previous frame's spawn. Consumers
// must not infer validity from the probe position texture alone — that
// texture can carry stale data from a frame where the tile was valid but
// is now sky or disoccluded.
static const uint PROBE_MASK_INVALID = 0u;
static const uint PROBE_MASK_VALID_BIT = 0x80000000u;

uint PackProbeMask(uint2 subTilePixel)
{
    return PROBE_MASK_VALID_BIT | (subTilePixel.y << 16) | (subTilePixel.x & 0xFFFFu);
}

uint2 UnpackProbeMask(uint packed)
{
    return uint2(packed & 0xFFFFu, (packed >> 16) & 0x7FFFu);
}

bool IsValidProbe(uint packed)
{
    return (packed & PROBE_MASK_VALID_BIT) != 0u;
}

// Halton low-discrepancy sequence. GI-1.0 §2.1.1 uses Halton(2) / Halton(3)
// to pick the anchor pixel inside each spawn tile; over ξ_x * ξ_y frames the
// sequence visits every (8-pixel-aligned) sub-region of the spawn tile once.
float Halton(uint i, uint base)
{
    float result = 0.0;
    float f = 1.0;
    while (i > 0u)
    {
        f /= float(base);
        result += f * float(i % base);
        i /= base;
    }
    return result;
}

// GI-1.0 Algorithm 6 — adaptive cell size in world units, a single
// heuristic threading reprojection, sampling, and filter. The paper's form:
//   cell_size = depth * tan(fovY * cell_size_px / max(W, H)) / sqrt(2) * delta
// with cell_size_px = 8 (target neighbor-probe pixel spacing) and delta = 1.
// fovY is recovered from the projection matrix at runtime — p[1][1] equals
// 1 / tan(fovY / 2) under the engine's row-vector convention.
float AdaptiveCellSize(float depth, float2 viewportSize, float4x4 proj)
{
    float tanHalfFovY = 1.0 / max(proj[1][1], EPSILON);
    float fovY = 2.0 * atan(tanHalfFovY);
    float cellSizePx = float(RADIANCE_CACHE_TILE_SIZE);
    float maxDim = max(viewportSize.x, viewportSize.y);
    return depth * tan(fovY * cellSizePx / maxDim) / SQRT2;
}

// GI-1.0 Algorithm 4 — sparse directional probe search. [F] lands a
// single-level mask sufficient for dense spawning; [S1] will extend this
// to a full MIP-chain walk once sparse spawning introduces real holes.
struct ProbeLookup
{
    int2 tileCoord;
    uint packed;
};

ProbeLookup FindClosestProbe(Texture2D<uint> probeMask, int2 pixel, int2 offsetInProbes, int2 gridSize)
{
    ProbeLookup r;
    r.tileCoord = (pixel / int(RADIANCE_CACHE_TILE_SIZE)) + offsetInProbes;
    r.packed = PROBE_MASK_INVALID;
    if (any(r.tileCoord < int2(0, 0)) || any(r.tileCoord >= gridSize))
        return r;
    r.packed = probeMask.Load(int3(r.tileCoord, 0));
    return r;
}

// GI-1.0 Algorithm 3 — biased shadow-preserving temporal hysteresis.
// Returns the blend factor t for `lerp(radiance_new, radiance_old, t)`.
// Matching the Capsaicin reference impl (GPUOpen-LibrariesAndSDKs/Capsaicin):
//   t = squared(clamp(max(L_new - L_old - min(L_new, L_old), 0) /
//                     max(max(L_new, L_old), 1e-4), 0.0, 0.95))
// Behaviour:
//   - Numerator is positive only when L_new > 2 * L_old (bright outlier),
//     so fireflies get heavy history weight (t near 0.95 squared = 0.9025).
//   - All other cases (agreement, darkening into shadow) produce t = 0 and
//     the new radiance is adopted directly. The intentional bias: shadows
//     propagate in one frame at the cost of some overall darkening, and
//     probe cells dominated by a rarely-hit emissive get filtered.
float TemporalBlendAlgo3(float lumaNew, float lumaOld)
{
    float num = max(lumaNew - lumaOld - min(lumaNew, lumaOld), 0.0);
    float den = max(max(lumaNew, lumaOld), 1e-4);
    float t = saturate(num / den);
    t = clamp(t, 0.0, 0.95);
    return t * t;
}

#endif // RADIANCE_CACHE_COMMON_HLSL
