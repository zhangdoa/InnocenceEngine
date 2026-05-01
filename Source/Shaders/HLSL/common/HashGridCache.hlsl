// shadertype=hlsl
#ifndef HASH_GRID_CACHE_HLSL_TYPES
#define HASH_GRID_CACHE_HLSL_TYPES

#include "common.hlsl"

// World-space hash-grid radiance cache. TASK-77.1 phase 1 — cell-only port
// of the Capsaicin GI-1.0 hash_grid_cache structure (paper §2.2). Divergences
// from the reference (intentional, phase-1 scope reduction):
//
//   - Flat single-buffer hash table (no per-tile bucket / mip chain).
//   - Cell key = (quantize(posWS, AdaptiveCellSize), packOctahedral(N)).
//   - Open-addressing linear-probe with probe length 4 (Capsaicin convention).
//   - Eviction on collision: replace the oldest cell in the probe chain by
//     frameLastTouched.
//   - 20 B per-cell payload (uint3 quantizedRadianceSum + uint sampleCount +
//     uint frameLastTouched) + 4 B key. No mip-level filtering, no separate
//     indirect/multibounce buffers, no decay buffer — those pay off at
//     secondary-vertex usage which is phase-2 work.
//   - Write site: PT primary hit only (Capsaicin writes at secondary).
//   - TASK-77.1.3 fix (D9 in .alignments/TASK-77.1.3-hash-grid-cache-paper-port.md):
//     payload migrated from non-atomic float3 running-mean to integer
//     quantized-sum + sample-count, accumulated via InterlockedAdd. Matches
//     Capsaicin's gi1.comp:1971-1975 / hash_grid_cache.hlsl:300-315 shape so
//     contributions from concurrent threads compose correctly under cell
//     contention. Read recovers the running mean as
//     (quantizedSum / (FloatQuantize * sampleCount)).
//
// The consumer compiles this header after declaring the buffer bindings
// (g_HashGridKeys, g_HashGridCells) and the per-frame view of g_FrameCount.
// Both buffers must be RWStructuredBuffer<...> bound at the same descriptor
// set so atomics and stores resolve to the same physical resource.

// Per-cell payload. Layout matches HashGridCacheConstants::CELL_BYTES (20 B).
//
// quantizedRadianceSum holds the per-channel sum of contributions multiplied
// by HASHGRID_FLOAT_QUANTIZE (1e3, mirroring Capsaicin's
// kHashGridCache_FloatQuantize at hash_grid_cache.hlsl:32). Each channel is
// accumulated independently via InterlockedAdd so concurrent inserts at the
// same cell compose correctly.
struct HashGridCell
{
    uint3 quantizedRadianceSum;
    uint  sampleCount;
    uint  frameLastTouched;
};

// Mirror of HashGridCacheConstants.h on the C++ side. Keep in sync.
static const uint  HASHGRID_CELL_COUNT       = (1u << 20);
static const uint  HASHGRID_PROBE_LENGTH     = 4u;

// Per-cell sample cap (write-side). Past the cap, new contributions are
// dropped — the denoiser's lerp weight (saturate(sampleCount/32) at
// GPUPathTracerDenoise.comp:73) is already saturated at 1.0, so further
// writes would only stress the uint32 sum without changing the displayed
// value. Bounded growth is also necessary to avoid uint32 wraparound in
// quantizedRadianceSum: with HASHGRID_FLOAT_QUANTIZE=1e3 and the upstream
// radiance clamp at 1e5 (GPUPathTracerRayGen.hlsl:502), worst-case
// per-sample quantized value is 1e8 — wraps after ~42 unbounded samples.
// Capsaicin's reference impl avoids this by using a per-frame scratch
// buffer + an EMA filter pass that drains and bounds the historical mean
// (gi1.comp:2160-2217); phase 1 takes the simpler shape with a hard cap.
static const uint  HASHGRID_SAMPLE_CAP       = 32u;

// Quantization scale for the integer sum. Matches Capsaicin's
// kHashGridCache_FloatQuantize (gi1/hash_grid_cache.hlsl:32). With
// HASHGRID_SAMPLE_CAP=32 and the upstream radiance clamp at 1e5, worst
// case sum per channel is 32 × 1e5 × 1e3 = 3.2e9 — under uint32 max
// (4.29e9), with single-frame thread contention pushing it close to
// the limit only for very-bright sun-NEE sample bursts. Precision
// floor on the recovered mean is 1/1e3 = 0.001 luma.
static const float HASHGRID_FLOAT_QUANTIZE   = 1e3f;

// Empty-slot sentinel for the key buffer. Zero-initialised memory reads as
// "empty"; we OR a high bit into every valid hash so a legitimate hash of
// zero still resolves to a valid key.
static const uint HASHGRID_KEY_EMPTY     = 0u;
static const uint HASHGRID_KEY_VALID_BIT = 0x80000000u;

// Adaptive cell size in world units. Reuses the GI-1.0 §2.1.7 / Algorithm 6
// formula already deployed for the screen-probe tier in
// RadianceCacheCommon::AdaptiveCellSize. For PT primary hits with no
// rasterizer-derived depth buffer, depth is recovered from
// length(hitPos - camera_posWS). The Capsaicin reference uses
// distance(eye, position) * cell_size for the same purpose
// (HashGridCache_GetCellSize, hash_grid_cache.hlsl) — same shape, slightly
// different parameterisation; ours threads through the projection's fovY
// instead of a free constant so cell-size scales with FOV.
float HashGridCache_CellSize(float depth, float2 viewportSize, float4x4 proj)
{
    float tanHalfFovY = 1.0 / max(proj[1][1], EPSILON);
    float fovY = 2.0 * atan(tanHalfFovY);
    float cellSizePx = 8.0;
    float maxDim = max(viewportSize.x, viewportSize.y);
    return max(depth * tan(fovY * cellSizePx / maxDim) / SQRT2, 0.01);
}

// PCG hash. Same 32-bit variant used by the path tracer's RNG, suitable
// here for reducing a multi-component cell key to a single bucket index.
uint HashGridCache_PCG(uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint HashGridCache_HashCombine(uint h, uint x)
{
    return HashGridCache_PCG(h ^ (x + 0x9E3779B9u + (h << 6) + (h >> 2)));
}

// Octahedral encoding of a unit normal into a uint (16 bits per axis).
// Cheap mapping; ~3.6 deg max error at 16 bits per axis is comfortably
// finer than the cell granularity bands we'd care about here.
uint HashGridCache_PackOctahedral(float3 n)
{
    n /= max(abs(n.x) + abs(n.y) + abs(n.z), EPSILON);
    float2 oct = (n.z >= 0.0)
        ? n.xy
        : (1.0 - abs(n.yx)) * float2(n.x >= 0.0 ? 1.0 : -1.0,
                                     n.y >= 0.0 ? 1.0 : -1.0);
    oct = oct * 0.5 + 0.5;
    uint2 q = uint2(saturate(oct) * 65535.0 + 0.5);
    return (q.y << 16) | (q.x & 0xFFFFu);
}

// Cell key from world-space position + normal. Position is quantised by
// the adaptive cell size (RadianceCacheCommon::AdaptiveCellSize), normal
// is octahedral-packed.
uint HashGridCache_BuildKey(float3 posWS, float3 normal, float cellSize)
{
    float invCellSize = 1.0 / max(cellSize, EPSILON);
    int3  cellCoord  = int3(floor(posWS * invCellSize));
    uint3 c          = asuint(cellCoord);

    uint h = HashGridCache_PCG(c.x);
    h = HashGridCache_HashCombine(h, c.y);
    h = HashGridCache_HashCombine(h, c.z);
    h = HashGridCache_HashCombine(h, HashGridCache_PackOctahedral(normalize(normal)));

    // Force a non-zero hash so zero-initialised memory remains an empty
    // slot. Bit 31 is the "valid" flag — separated from the bucket index
    // by HASHGRID_CELL_COUNT being a power of two ≤ 2^31.
    return (h | HASHGRID_KEY_VALID_BIT);
}

uint HashGridCache_BucketIndex(uint key)
{
    return (key & ~HASHGRID_KEY_VALID_BIT) % HASHGRID_CELL_COUNT;
}

// Quantize a float3 radiance to the integer sum representation. Matches
// Capsaicin's HashGridCache_QuantizeRadiance (hash_grid_cache.hlsl:300-306).
uint3 HashGridCache_QuantizeRadiance(float3 radiance)
{
    // Negative radiance is not expected at a primary-hit accumulator but
    // a clamp here prevents underflow into wrap-around uint values.
    float3 q = max(radiance, 0.0f) * HASHGRID_FLOAT_QUANTIZE;
    return uint3(uint(round(q.x)), uint(round(q.y)), uint(round(q.z)));
}

// Recover the running mean from the integer sum. Matches Capsaicin's
// HashGridCache_RecoverRadiance (hash_grid_cache.hlsl:308-315), but with
// the per-cell sample count stored separately rather than in the .w
// component of the radiance vector. Returns 0 for an unwritten cell.
float3 HashGridCache_DequantizeRadiance(uint3 quantizedSum, uint sampleCount)
{
    float invDivisor = 1.0f / max(HASHGRID_FLOAT_QUANTIZE * float(sampleCount), HASHGRID_FLOAT_QUANTIZE);
    return float3(float(quantizedSum.x), float(quantizedSum.y), float(quantizedSum.z)) * invDivisor;
}

#endif // HASH_GRID_CACHE_HLSL_TYPES

// Binding-using helpers — second-pass include after the consumer has
// declared g_HashGridKeys / g_HashGridCells and #defined
// HASHGRIDCACHE_HAS_BINDINGS.
#if defined(HASHGRIDCACHE_HAS_BINDINGS) && !defined(HASH_GRID_CACHE_HLSL_BINDINGS)
#define HASH_GRID_CACHE_HLSL_BINDINGS

// HASHGRIDCACHE_HAS_BINDINGS is defined by the consumer immediately before
// the include, after the consumer has bound:
//   RWStructuredBuffer<uint>          g_HashGridKeys
//   RWStructuredBuffer<HashGridCell>  g_HashGridCells
//   uint                              g_FrameCount  (mirrored cb)

// Open-addressing probe: walk PROBE_LENGTH consecutive slots starting at
// the bucket index. On a hit (existing key match), return the slot index.
// On an empty slot, claim it via InterlockedCompareExchange. On a probe
// overflow, evict the oldest cell in the chain (lowest frameLastTouched)
// and overwrite. Returns the cell slot to write into.
uint HashGridCache_InsertOrFind(uint key, uint frameIndex, out bool isNewInsert)
{
    uint bucket = HashGridCache_BucketIndex(key);
    isNewInsert = false;

    uint oldestSlot = bucket;
    uint oldestFrame = 0xFFFFFFFFu;

    [loop]
    for (uint i = 0u; i < HASHGRID_PROBE_LENGTH; ++i)
    {
        uint slot = (bucket + i) % HASHGRID_CELL_COUNT;
        uint expected = HASHGRID_KEY_EMPTY;
        uint previous;
        InterlockedCompareExchange(g_HashGridKeys[slot], expected, key, previous);
        if (previous == HASHGRID_KEY_EMPTY)
        {
            isNewInsert = true;
            return slot;
        }
        if (previous == key)
        {
            return slot;
        }
        // Track the oldest slot in the probe chain for eviction fallback.
        uint slotFrame = g_HashGridCells[slot].frameLastTouched;
        if (slotFrame < oldestFrame)
        {
            oldestFrame = slotFrame;
            oldestSlot  = slot;
        }
    }

    // Probe overflow: evict the oldest cell in the chain. Stomp the key
    // and zero the stale payload — without the zero, the new key would
    // inherit the previous occupant's quantizedSum/sampleCount and the
    // first read after eviction would return the old cell's mean. The
    // four uint stores below are not atomic with respect to a concurrent
    // InterlockedAdd at this slot from another thread that observed the
    // OLD key one instruction earlier, but that race is a frame-bounded
    // single-sample drop — invisible against the running mean.
    g_HashGridCells[oldestSlot].quantizedRadianceSum = uint3(0u, 0u, 0u);
    g_HashGridCells[oldestSlot].sampleCount          = 0u;
    g_HashGridCells[oldestSlot].frameLastTouched     = frameIndex;
    g_HashGridKeys[oldestSlot]                       = key;
    isNewInsert = true;
    return oldestSlot;
}

// Atomic radiance accumulator. Matches Capsaicin's gi1.comp:1971-1975
// (UpdateMultibounceCells) — InterlockedAdd on each quantized channel +
// the sample-counter. Concurrent inserts at the same cell compose
// correctly because each contribution is summed independently.
//
// Divergence vs Capsaicin (gi1.comp:2087-2095, hash_grid_cache.hlsl:300-315):
//   - They store the .w sample count inline in a 4-uint per-cell record;
//     we keep sampleCount as a named field at byte offset 12 in
//     HashGridCell. Same atomic semantics.
//   - They use a per-frame "scratch" buffer (UpdateCellValueBuffer)
//     cleared every frame and EMA-blended into a persistent ValueBuffer
//     in a separate kernel (gi1.comp:2160-2217). We fold both into a
//     single buffer with a write-side sample cap. The deferred filter
//     pass — paper §2.2.3's "exponential moving average" — is a phase 2
//     follow-up; without it, the cell's recovered mean is an unweighted
//     average of the first HASHGRID_SAMPLE_CAP contributions and does
//     not adapt to relighting.
void HashGridCache_Insert(float3 posWS, float3 normal, float3 radiance, float cellSize, uint frameIndex)
{
    uint key = HashGridCache_BuildKey(posWS, normal, cellSize);
    bool isNewInsert;
    uint slot = HashGridCache_InsertOrFind(key, frameIndex, isNewInsert);

    // Drop the contribution once the cell is full. The denoiser is
    // already saturated at sampleCount/32, so further writes would not
    // change the displayed value — and would risk wrap-around on the
    // sum. Race window: two threads can both observe sampleCount = CAP-1
    // and both proceed; the InterlockedAdd below still composes their
    // contributions correctly, just allowing sampleCount to exceed CAP
    // by the per-frame thread count touching this cell. With the upstream
    // 1e5 radiance clamp and HASHGRID_FLOAT_QUANTIZE=1e3, the resulting
    // quantizedSum stays under uint32 max for typical contention.
    if (g_HashGridCells[slot].sampleCount >= HASHGRID_SAMPLE_CAP)
    {
        g_HashGridCells[slot].frameLastTouched = frameIndex;
        return;
    }

    uint3 q = HashGridCache_QuantizeRadiance(radiance);

    uint dummy;
    InterlockedAdd(g_HashGridCells[slot].quantizedRadianceSum.x, q.x, dummy);
    InterlockedAdd(g_HashGridCells[slot].quantizedRadianceSum.y, q.y, dummy);
    InterlockedAdd(g_HashGridCells[slot].quantizedRadianceSum.z, q.z, dummy);
    InterlockedAdd(g_HashGridCells[slot].sampleCount,            1u,  dummy);

    // frameLastTouched feeds the eviction tiebreaker; an exact non-atomic
    // last-writer-wins on a uint is acceptable — concurrent writers in
    // the same frame all write the same value.
    g_HashGridCells[slot].frameLastTouched = frameIndex;
}

bool HashGridCache_Read(float3 posWS, float3 normal, float cellSize, out float3 radiance, out uint sampleCount)
{
    uint key = HashGridCache_BuildKey(posWS, normal, cellSize);
    uint bucket = HashGridCache_BucketIndex(key);
    radiance = float3(0.0, 0.0, 0.0);
    sampleCount = 0u;

    [loop]
    for (uint i = 0u; i < HASHGRID_PROBE_LENGTH; ++i)
    {
        uint slot = (bucket + i) % HASHGRID_CELL_COUNT;
        uint stored = g_HashGridKeys[slot];
        if (stored == HASHGRID_KEY_EMPTY)
            return false;
        if (stored == key)
        {
            HashGridCell cell = g_HashGridCells[slot];
            sampleCount = cell.sampleCount;
            radiance = HashGridCache_DequantizeRadiance(cell.quantizedRadianceSum, sampleCount);
            return true;
        }
    }
    return false;
}

#endif // HASHGRIDCACHE_HAS_BINDINGS && !HASH_GRID_CACHE_HLSL_BINDINGS
