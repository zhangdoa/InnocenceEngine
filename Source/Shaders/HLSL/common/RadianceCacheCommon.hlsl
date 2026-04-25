// shadertype=hlsl
#ifndef RADIANCE_CACHE_COMMON_HLSL
#define RADIANCE_CACHE_COMMON_HLSL

#include "common.hlsl"
#include "../RayTracingTypes.hlsl"

// GI-1.0 (Boissé et al., AMD, 2022) primitives shared across the radiance
// cache passes.

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

// GI-1.0 Algorithm 4 — sparse directional probe search. Returns the target
// tile if it has a valid probe, otherwise expands outward in Chebyshev
// rings looking for the closest valid substitute. The paper's form uses a
// probe-mask MIP chain to cover the same search pattern in O(log r); with
// 2×2 sparse spawning the worst-case hole is ≤ 2 probe-tiles, so a direct
// ring walk to radius PROBE_SEARCH_MAX_RING (2) covers the same cases
// without the MIP chain overhead.
//
// On success, r.tileCoord is the SUBSTITUTE tile's coordinate (which may
// differ from the requested offset) — callers must use r.tileCoord for
// all subsequent world-space reads so the substitute's own position and
// normal drive the caller's rejection tests, not the target's.
struct ProbeLookup
{
    int2 tileCoord;
    uint packed;
};

static const int PROBE_SEARCH_MAX_RING = 2;

ProbeLookup FindClosestProbe(Texture2D<uint> probeMask, int2 pixel, int2 offsetInProbes, int2 gridSize)
{
    ProbeLookup r;
    int2 baseTile = (pixel / int(RADIANCE_CACHE_TILE_SIZE)) + offsetInProbes;
    r.tileCoord = baseTile;
    r.packed = PROBE_MASK_INVALID;

    // Ring 0 — the requested target.
    if (all(baseTile >= int2(0, 0)) && all(baseTile < gridSize))
    {
        uint packed = probeMask.Load(int3(baseTile, 0));
        if (IsValidProbe(packed))
        {
            r.packed = packed;
            return r;
        }
    }

    // Rings 1..PROBE_SEARCH_MAX_RING — Chebyshev ring (outline only, not
    // filled) gives even coverage without duplicating interior tiles we
    // already visited. First-hit wins; the loop order (ring, then dy,
    // then dx) makes "closest substitute" deterministic across invocations.
    for (int ring = 1; ring <= PROBE_SEARCH_MAX_RING; ring++)
    {
        for (int dy = -ring; dy <= ring; dy++)
        {
            for (int dx = -ring; dx <= ring; dx++)
            {
                if (abs(dx) != ring && abs(dy) != ring) continue;
                int2 q = baseTile + int2(dx, dy);
                if (any(q < int2(0, 0)) || any(q >= gridSize)) continue;
                uint packed = probeMask.Load(int3(q, 0));
                if (IsValidProbe(packed))
                {
                    r.tileCoord = q;
                    r.packed = packed;
                    return r;
                }
            }
        }
    }

    return r;
}

// GI-1.0 Algorithm 5 angular-error threshold for parallax-corrected filter
// taps. Capsaicin uses cos(2e-2 * PI) ≈ 0.998 — reject a tap if the
// parallax-reprojected direction diverges by > ~3.6° from the original
// cell direction. Preserves small-scale occlusion (paper Figure 9): a
// neighbour whose stored hit distance, re-aimed from the current probe,
// points at a noticeably different world direction is likely seeing
// different geometry and would contaminate shadow edges if blended in.
static const float PROBE_FILTER_ANGLE_THRESHOLD = 0.998;

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

// Per-pixel cache evaluation helpers. Compiled only when the consumer has
// declared the four cache textures (in_RadianceCache, in_ProbePosition,
// in_ProbeNormal, in_ProbeMask) and the PerFrame_CB instance `g_Frame`
// before the #include. Two callers exist today: GIDenoise.comp consumes the
// full bilinear blend; RadianceCacheFilter{Horizontal,Vertical}.comp use
// only FindClosestProbe (above) and don't need these, so they leave the
// flag undefined and skip the bodies.
#ifdef RADIANCE_CACHE_HAS_BINDINGS

// GI-1.0 §2.4.2 irradiance evaluation via Ramamoorthi–Hanrahan 2001
// cosine-lobe convolution. 9 coefficients (bands 0–2) packed row-major
// in a 3×3 SH tile per probe:
//   (0,0)=Y00   (1,0)=Y11   (2,0)=Y1_1
//   (0,1)=Y10   (1,1)=Y2_2  (2,1)=Y2_1
//   (0,2)=Y20   (1,2)=Y21   (2,2)=Y22
float3 LoadIrradiance(uint2 shReadCoord, float3 n)
{
	float3 c00 = in_RadianceCache[shReadCoord + uint2(0, 0)].rgb;
	float3 c11 = in_RadianceCache[shReadCoord + uint2(1, 0)].rgb;
	float3 c1_1 = in_RadianceCache[shReadCoord + uint2(2, 0)].rgb;
	float3 c10 = in_RadianceCache[shReadCoord + uint2(0, 1)].rgb;
	float3 c2_2 = in_RadianceCache[shReadCoord + uint2(1, 1)].rgb;
	float3 c2_1 = in_RadianceCache[shReadCoord + uint2(2, 1)].rgb;
	float3 c20 = in_RadianceCache[shReadCoord + uint2(0, 2)].rgb;
	float3 c21 = in_RadianceCache[shReadCoord + uint2(1, 2)].rgb;
	float3 c22 = in_RadianceCache[shReadCoord + uint2(2, 2)].rgb;

	const float A0 = 1.0;
	const float A1 = 2.0 / 3.0;
	const float A2 = 1.0 / 4.0;

	float3 band0 = A0 * c00  * Y_00();
	float3 band1 = A1 * (c11  * Y_11 (n) + c1_1 * Y_1_1(n) + c10  * Y_10 (n));
	float3 band2 = A2 * (c2_2 * Y_2_2(n) + c2_1 * Y_2_1(n) + c20  * Y_20 (n) +
	                     c21  * Y_21 (n) + c22  * Y_22 (n));

	return max(band0 + band1 + band2, 0.0);
}

// GI-1.0 §2.4 edge-aware probe weight. Returns 0 when the probe is sky /
// off-surface or crosses a plane / normal discontinuity.
float ComputeProbeWeight(uint2 probeCoord, uint2 maxProbeIdx,
                         float3 pixelPos, float3 pixelNormal, float cellSize)
{
	probeCoord = min(probeCoord, maxProbeIdx);
	uint mask = in_ProbeMask[probeCoord];
	if (!IsValidProbe(mask))
		return 0.0;

	float3 probePos = in_ProbePosition[probeCoord].xyz;
	float3 probeNormal = normalize(in_ProbeNormal[probeCoord].xyz);

	float planeDist = abs(dot(probePos - pixelPos, pixelNormal));
	if (planeDist > cellSize)
		return 0.0;

	float normalDot = dot(pixelNormal, probeNormal);
	if (normalDot <= 0.0)
		return 0.0;

	return normalDot * saturate(1.0 - planeDist / cellSize);
}

// GI-1.0 §2.4.1 per-pixel interpolation: weighted average of the 4 probes
// surrounding the pixel. Each corner combines the bilinear weight from
// screen-space position with an edge-aware weight (probe validity + plane
// + normal). Relaxed-interpolation fallback (paper §2.4.1): equal-weight
// screen-bilinear blend so edge pixels don't go black when the local
// neighbourhood has no good probe.
float3 SampleRadianceCache(float2 screenCoord, float3 pixelPos, float3 pixelNormal)
{
	// Probe (i, j) is anchored at tile centre TILE_SIZE * (i + 0.5). Shift the
	// screen coord into probe-grid space (subtract half a tile) so `probeFloor`
	// is the top-left probe of the 4-probe quad that surrounds the pixel, not
	// the tile the pixel happens to land in.
	float2 probeUV = float2(screenCoord) / float2(RADIANCE_CACHE_TILE_SIZE, RADIANCE_CACHE_TILE_SIZE) - 0.5;
	int2 probeFloor = int2(floor(probeUV));
	uint2 maxProbeIndex = uint2(g_Frame.viewportSize.xy) / RADIANCE_CACHE_TILE_SIZE - 1;
	int2 maxProbeIndexI = int2(maxProbeIndex);

	float depth = length(pixelPos - g_Frame.camera_posWS.xyz);
	float cellSize = max(AdaptiveCellSize(depth, g_Frame.viewportSize.xy, g_Frame.p_original), 0.1);

	int2 targetTL = clamp(probeFloor,              int2(0, 0), maxProbeIndexI);
	int2 targetTR = clamp(probeFloor + int2(1, 0), int2(0, 0), maxProbeIndexI);
	int2 targetBL = clamp(probeFloor + int2(0, 1), int2(0, 0), maxProbeIndexI);
	int2 targetBR = clamp(probeFloor + int2(1, 1), int2(0, 0), maxProbeIndexI);

	int2 gridSize = int2(g_Frame.viewportSize.xy) / int(RADIANCE_CACHE_TILE_SIZE);
	ProbeLookup lookupTL = FindClosestProbe(in_ProbeMask, int2(0, 0), targetTL, gridSize);
	ProbeLookup lookupTR = FindClosestProbe(in_ProbeMask, int2(0, 0), targetTR, gridSize);
	ProbeLookup lookupBL = FindClosestProbe(in_ProbeMask, int2(0, 0), targetBL, gridSize);
	ProbeLookup lookupBR = FindClosestProbe(in_ProbeMask, int2(0, 0), targetBR, gridSize);

	uint2 tl = uint2(lookupTL.tileCoord);
	uint2 tr = uint2(lookupTR.tileCoord);
	uint2 bl = uint2(lookupBL.tileCoord);
	uint2 br = uint2(lookupBR.tileCoord);

	float2 bilinear = probeUV - float2(probeFloor);
	float wTL = (1.0 - bilinear.x) * (1.0 - bilinear.y);
	float wTR = bilinear.x * (1.0 - bilinear.y);
	float wBL = (1.0 - bilinear.x) * bilinear.y;
	float wBR = bilinear.x * bilinear.y;

	float eTL = ComputeProbeWeight(tl, maxProbeIndex, pixelPos, pixelNormal, cellSize);
	float eTR = ComputeProbeWeight(tr, maxProbeIndex, pixelPos, pixelNormal, cellSize);
	float eBL = ComputeProbeWeight(bl, maxProbeIndex, pixelPos, pixelNormal, cellSize);
	float eBR = ComputeProbeWeight(br, maxProbeIndex, pixelPos, pixelNormal, cellSize);

	float fTL = wTL * eTL;
	float fTR = wTR * eTR;
	float fBL = wBL * eBL;
	float fBR = wBR * eBR;

	float totalWeight = fTL + fTR + fBL + fBR;

	float3 result;
	if (totalWeight > 0.0)
	{
		float3 ITL = LoadIrradiance(tl * SH_TILE_SIZE, pixelNormal);
		float3 ITR = LoadIrradiance(tr * SH_TILE_SIZE, pixelNormal);
		float3 IBL = LoadIrradiance(bl * SH_TILE_SIZE, pixelNormal);
		float3 IBR = LoadIrradiance(br * SH_TILE_SIZE, pixelNormal);
		result = (fTL * ITL + fTR * ITR + fBL * IBL + fBR * IBR) / totalWeight;
	}
	else
	{
		float3 ITL = LoadIrradiance(tl * SH_TILE_SIZE, pixelNormal);
		float3 ITR = LoadIrradiance(tr * SH_TILE_SIZE, pixelNormal);
		float3 IBL = LoadIrradiance(bl * SH_TILE_SIZE, pixelNormal);
		float3 IBR = LoadIrradiance(br * SH_TILE_SIZE, pixelNormal);
		result = wTL * ITL + wTR * ITR + wBL * IBL + wBR * IBR;
	}

	return max(result, float3(0.0, 0.0, 0.0));
}

#endif // RADIANCE_CACHE_HAS_BINDINGS

#endif // RADIANCE_CACHE_COMMON_HLSL
