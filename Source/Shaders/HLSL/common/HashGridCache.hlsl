// shadertype=hlsl
#ifndef HASH_GRID_CACHE_HLSL_TYPES
#define HASH_GRID_CACHE_HLSL_TYPES

#include "common.hlsl"

// World-space hash-grid radiance cache. Cell-only port of the Capsaicin
// GI-1.0 hash_grid_cache structure (paper §2.2). Divergences from the
// reference (intentional, phase-1 scope reduction; tracked in
// .alignments/TASK-77.1.3-hash-grid-cache-paper-port.md):
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
//
// Storage shape — two-buffer EMA (mirrors Capsaicin gi1.comp:2160-2217):
//
//   - g_HashGridScratch (PT writes here via HashGridCache_Insert):
//     per-frame contributions, accumulated atomically. Drained + zeroed by
//     the filter pass each frame.
//   - g_HashGridValue (filter pass writes; denoise reads): persistent
//     EMA-blended history. Read site recovers the running mean as
//     (quantizedSum / (FloatQuantize * sampleCount)).
//   - g_HashGridKeys (shared identity): one slot per (key, scratch entry,
//     value entry). Eviction zeros both payload buffers at the slot.
//
// The consumer compiles this header after declaring the buffer bindings it
// needs (g_HashGridKeys + at least one of g_HashGridScratch / g_HashGridValue)
// and the per-frame view of g_FrameCount. Buffers must be
// RWStructuredBuffer<...> bound at the same descriptor set so atomics and
// stores resolve to the same physical resource.

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

// Persistent-buffer sample-count clamp. The filter pass clamps the EMA
// blend's effective sample count to this value (Capsaicin's max_sample_count
// at gi1.comp:2170, hash_grid_cache.hlsl). Once the persistent cell has
// accumulated this many contributions, the EMA weight floors at
// 1/HASHGRID_MAX_SAMPLE_COUNT — keeping the cache responsive to relighting
// and bounding the impact of bright single-frame outliers (sun-NEE bursts
// on small-working-set cells). Doubles as the uint32-wrap bound on the
// stored quantized sum: with HASHGRID_FLOAT_QUANTIZE=1e3 and the upstream
// luma-preserving firefly clamp at 100 luma (GPUPathTracerRayGen.hlsl
// cacheRadiance), worst-case stored sum is
//   MAX_SAMPLE_COUNT × 100 × 1e3 = 3.2e6 ≪ 4.29e9 (uint32 max);
// ~1300x head-room.
static const uint  HASHGRID_MAX_SAMPLE_COUNT = 32u;

// Per-frame scratch sampleCount cap. Bounds the in-frame contribution count
// per cell so a contention burst (full 8x8 wavefront colliding into one
// near-camera cell) cannot wrap the uint32 quantized sum on the scratch
// side before the filter pass drains it. Worst-case per-frame quantized
// sum on scratch: PER_FRAME_CAP × 100 × 1e3 = 3.2e6 at 32 samples — same
// 1300x head-room as the persistent buffer. The cap is enforced via a
// non-atomic pre-check; race past the cap by a wavefront's worth (~32-64
// contributions) is harmless both for wrap (still well under uint32) and
// for quality (the EMA filter clamps the blended count to
// HASHGRID_MAX_SAMPLE_COUNT).
static const uint  HASHGRID_PER_FRAME_CAP    = 32u;

// Quantization scale for the integer sum. Matches Capsaicin's
// kHashGridCache_FloatQuantize (gi1/hash_grid_cache.hlsl:32). With
// HASHGRID_MAX_SAMPLE_COUNT=32 and the upstream luma-preserving clamp
// at 100 luma, worst-case sum per channel is 32 × 100 × 1e3 = 3.2e6 —
// well under uint32 max (4.29e9). Precision floor on the recovered mean
// is 1/1e3 = 0.001 luma.
static const float HASHGRID_FLOAT_QUANTIZE   = 1e3f;

// Empty-slot sentinel for the key buffer. Zero-initialised memory reads as
// "empty"; we OR a high bit into every valid hash so a legitimate hash of
// zero still resolves to a valid key.
static const uint HASHGRID_KEY_EMPTY     = 0u;
static const uint HASHGRID_KEY_VALID_BIT = 0x80000000u;

// Adaptive cell size in world units, quantised to a power of two. Mirrors
// Capsaicin's HashGridCache_GetCellSize (hash_grid_cache.hlsl:96-102):
//   cell_size_step = max(distance(eye, hit) * fovScale, min_cell_size)
//   cell_size      = SIZE_FACTOR * exp2(floor(log2(STEP_FACTOR * step)))
// The exp2(floor(log2(...))) step is load-bearing — it locks neighbouring
// frames to the same cell binning despite Halton-jittered camera samples
// (GPUPathTracerRayGen.hlsl jitter), which a continuous-valued cell size
// would defeat at depths where the world-space cell size approaches a
// single screen-space pixel.
//
// fovScale = tan(fovY * cellSizePx / maxDim) / sqrt(2) keeps the underlying
// "cellSizePx pixels at the hit point" intent of the prior formulation;
// only the final quantisation step changes.
static const float HASHGRID_STEP_FACTOR = 1e3f;
static const float HASHGRID_SIZE_FACTOR = 1e-3f;

float HashGridCache_CellSize(float depth, float2 viewportSize, float4x4 proj)
{
    float tanHalfFovY = 1.0 / max(proj[1][1], EPSILON);
    float fovY        = 2.0 * atan(tanHalfFovY);
    // Cell footprint at the hit point, expressed in source-pixel units.
    // Larger values aggregate more rays per cell (denser sampling per
    // bucket) at the cost of spatial resolution; smaller values reduce
    // light-leak across surface boundaries and visible cell-boundary
    // tiling on near surfaces. TASK-208 fix-up (2026-05-01): dropped from
    // 64 to 16 because at fixed camera the cache reaches saturation and
    // the per-cell mean projected onto adjacent pixels surfaced as a
    // visible checkerboard — 16 px cells make each tile small enough that
    // the boundary is below the per-pixel noise scale once the noisy
    // lerp residual (HashGridCache_DenoiseSampleCap cap < 1) is composed
    // back in. ~0.5x Capsaicin's default 32 (gi1.h:58
    // gi1_hash_grid_cache_cell_size = 32.0F combined with the per-pixel-
    // density factor at gi1.cpp:1856-1859); see D11 in
    // .alignments/TASK-77.1.3-hash-grid-cache-paper-port.md.
    float cellSizePx  = 16.0;
    float maxDim      = max(viewportSize.x, viewportSize.y);
    float fovScale    = tan(fovY * cellSizePx / maxDim) / SQRT2;

    float step = max(depth * fovScale, 0.01);
    float lod  = floor(log2(HASHGRID_STEP_FACTOR * step));
    return HASHGRID_SIZE_FACTOR * exp2(lod);
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

// Coarse octahedral encoding of a unit normal into a uint, quantised at
// 3 bits per axis (8 levels per axis = ~45 deg max bin width). The
// coarseness is deliberate — fine normal binning fragments the cell
// working set across smoothly-interpolated normals on high-poly meshes
// (curtains, foliage), leaving most cells with too few samples for the
// EMA filter to stabilise. Compare to Capsaicin's ray-direction binning
// at 5x5x5 = 125 buckets (hash_grid_cache.hlsl direction coding); the
// principle is the same: surface orientation discriminates light-leak
// between back-to-back surfaces, but does not need to track per-pixel
// micro-normals — radiance varies across smooth normal bands more slowly
// than the noise floor of the per-frame estimator does.
uint HashGridCache_PackOctahedral(float3 n)
{
    n /= max(abs(n.x) + abs(n.y) + abs(n.z), EPSILON);
    float2 oct = (n.z >= 0.0)
        ? n.xy
        : (1.0 - abs(n.yx)) * float2(n.x >= 0.0 ? 1.0 : -1.0,
                                     n.y >= 0.0 ? 1.0 : -1.0);
    oct = oct * 0.5 + 0.5;
    uint2 q = uint2(saturate(oct) * 7.0 + 0.5);
    return (q.y << 3) | (q.x & 0x7u);
}

// Cell key from world-space position + normal. Position is quantised by
// the adaptive cell size (RadianceCacheCommon::AdaptiveCellSize), normal
// is octahedral-packed at 6 bits per axis (~5.6 deg max error).
//
// Coarse normal binning is load-bearing: the read site (denoise pass)
// looks up by primary-hit normal which varies across texture-mapped
// pixels even on a planar surface. Fine normal binning fragments the
// per-cell sample working set across micro-normals, leaving most cells
// too sparse for the EMA filter to stabilise. Compare to Capsaicin's
// ray-direction binning at ~125 total buckets across the unit sphere
// (hash_grid_cache.hlsl direction coding).
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
// declared the buffers it needs and #defined HASHGRIDCACHE_HAS_BINDINGS.
//
// Shared bindings:
//   RWStructuredBuffer<uint>          g_HashGridKeys
//   uint                              g_FrameCount      (mirrored cb)
// Write-site bindings (defined when HASHGRIDCACHE_HAS_SCRATCH):
//   RWStructuredBuffer<HashGridCell>  g_HashGridScratch
// Read-site bindings (defined when HASHGRIDCACHE_HAS_VALUE):
//   RWStructuredBuffer<HashGridCell>  g_HashGridValue
// The filter pass declares both and #defines both flags.
#if defined(HASHGRIDCACHE_HAS_BINDINGS) && !defined(HASH_GRID_CACHE_HLSL_BINDINGS)
#define HASH_GRID_CACHE_HLSL_BINDINGS

#if defined(HASHGRIDCACHE_HAS_SCRATCH)

// Open-addressing probe: walk PROBE_LENGTH consecutive slots starting at
// the bucket index. On a hit (existing key match), return the slot index.
// On an empty slot, claim it via InterlockedCompareExchange. On a probe
// overflow, evict the oldest cell in the chain (lowest frameLastTouched)
// and overwrite. Returns the cell slot to write into. Eviction zeros only
// the scratch payload — the persistent g_HashGridValue at the same slot is
// drained + zeroed by the filter pass on the same frame the eviction is
// observed there (the filter pass sees scratch.sampleCount==0 unless this
// frame's writer landed; otherwise treats persistent as still owned by the
// new key and lets the next frame's contributions seed it).
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
        uint slotFrame = g_HashGridScratch[slot].frameLastTouched;
        if (slotFrame < oldestFrame)
        {
            oldestFrame = slotFrame;
            oldestSlot  = slot;
        }
    }

    // Probe overflow: evict the oldest cell in the chain. Stomp the key
    // and zero both payload buffers at the slot — without the zero on
    // value, the new key would inherit the evicted occupant's EMA-blended
    // history on first read; without the zero on scratch, the filter pass
    // would blend the prior tenant's per-frame contributions into the new
    // key's first persistent record.
    //
    // Two race shapes survive at this point, both bounded:
    //  (a) New-key-inherits-old-value: a reader on a different thread
    //      probes slot S after the key flip but before the value zero
    //      lands; reads K_new with K_old's EMA history. The zero closes
    //      this within one writer's atomic store — single-frame artifact.
    //  (b) Wrong-cell pollution: a writer that already resolved its slot
    //      to S for K_old (between probe-resolve and InterlockedAdd) races
    //      the eviction; its InterlockedAdd lands in K_new's slot. The
    //      late writer pollutes K_new with K_old's per-frame radiance
    //      until the EMA filter or another eviction averages it out.
    // Both are bounded — eviction frequency is gated by probe-chain
    // pressure (4 slots) and HASHGRID_CELL_COUNT (2^20). Acceptable for
    // phase-1.5; resolving requires the descriptor + tile-fingerprint
    // shape (D2/D4 in the alignment artifact) which is phase-2 work.
#if defined(HASHGRIDCACHE_HAS_VALUE)
    g_HashGridValue[oldestSlot].quantizedRadianceSum   = uint3(0u, 0u, 0u);
    g_HashGridValue[oldestSlot].sampleCount            = 0u;
    g_HashGridValue[oldestSlot].frameLastTouched       = frameIndex;
#endif
    g_HashGridScratch[oldestSlot].quantizedRadianceSum = uint3(0u, 0u, 0u);
    g_HashGridScratch[oldestSlot].sampleCount          = 0u;
    g_HashGridScratch[oldestSlot].frameLastTouched     = frameIndex;
    g_HashGridKeys[oldestSlot]                         = key;
    isNewInsert = true;
    return oldestSlot;
}

// Atomic per-frame contribution accumulator. Matches Capsaicin's
// gi1.comp:1971-1975 + hash_grid_cache.hlsl scratch-buffer write — every
// thread's quantised radiance is summed atomically into the scratch slot;
// the filter pass drains scratch into the persistent value buffer on the
// same frame.
void HashGridCache_Insert(float3 posWS, float3 normal, float3 radiance, float cellSize, uint frameIndex)
{
    uint key = HashGridCache_BuildKey(posWS, normal, cellSize);
    bool isNewInsert;
    uint slot = HashGridCache_InsertOrFind(key, frameIndex, isNewInsert);

    // Per-frame cap — bounds in-frame contention so a near-camera cell
    // hit by an entire wavefront cannot wrap the uint32 quantized sum
    // before the filter pass drains scratch. Race past the cap is
    // harmless: the filter pass clamps the EMA blend's effective sample
    // count to HASHGRID_MAX_SAMPLE_COUNT, which doubles as the persistent
    // wrap bound. A non-atomic pre-check is sufficient — a wavefront-sized
    // overshoot stays well below uint32 max.
    if (g_HashGridScratch[slot].sampleCount >= HASHGRID_PER_FRAME_CAP)
    {
        g_HashGridScratch[slot].frameLastTouched = frameIndex;
        return;
    }

    uint3 q = HashGridCache_QuantizeRadiance(radiance);

    uint dummy;
    InterlockedAdd(g_HashGridScratch[slot].quantizedRadianceSum.x, q.x, dummy);
    InterlockedAdd(g_HashGridScratch[slot].quantizedRadianceSum.y, q.y, dummy);
    InterlockedAdd(g_HashGridScratch[slot].quantizedRadianceSum.z, q.z, dummy);
    InterlockedAdd(g_HashGridScratch[slot].sampleCount,            1u,  dummy);

    // frameLastTouched feeds the eviction tiebreaker; concurrent writers
    // in the same frame all write the same value, so non-atomic last-
    // writer-wins is acceptable.
    g_HashGridScratch[slot].frameLastTouched = frameIndex;
}

#endif // HASHGRIDCACHE_HAS_SCRATCH

#if defined(HASHGRIDCACHE_HAS_VALUE) && defined(HASHGRIDCACHE_HAS_SCRATCH)

// EMA filter — drains one scratch slot into the persistent value buffer.
// Mirrors Capsaicin's UpdateTilesMain (gi1.comp:2160-2217): recover the
// running mean on both buffers, blend with EMA weight 1/totalSampleCount
// where totalSampleCount is clamped to HASHGRID_MAX_SAMPLE_COUNT, then
// re-multiply the mean by the clamped sample count to maintain the
// (sum, sampleCount) storage shape. Returns true if the slot was active
// this frame (caller may want to clear the scratch entry afterwards).
//
// EMA-weight choice (1/total rather than scratchCount/total): once the
// persistent count saturates at MAX, the blend changes the mean by at
// most 1/MAX per frame regardless of how many contributions arrived
// in scratch. That is the load-bearing variance-reduction property the
// outer denoiser depends on; using scratchCount/total would collapse to
// full replacement when scratch is also saturated, defeating the EMA.
//
// Storage shape note: the persistent buffer's quantizedRadianceSum stores
// (mean × sampleCount) post-blend so the read path's
// HashGridCache_DequantizeRadiance reconstruction stays the same.
bool HashGridCache_FilterCell(uint slot, uint frameIndex)
{
    uint storedKey = g_HashGridKeys[slot];
    if (storedKey == HASHGRID_KEY_EMPTY)
        return false;

    HashGridCell scratchCell = g_HashGridScratch[slot];
    if (scratchCell.sampleCount == 0u)
        return false;

    HashGridCell valueCell = g_HashGridValue[slot];

    float3 oldMean = HashGridCache_DequantizeRadiance(valueCell.quantizedRadianceSum, valueCell.sampleCount);
    float3 newMean = HashGridCache_DequantizeRadiance(scratchCell.quantizedRadianceSum, scratchCell.sampleCount);

    uint  totalCount     = min(valueCell.sampleCount + scratchCell.sampleCount, HASHGRID_MAX_SAMPLE_COUNT);
    float totalCountF    = float(max(totalCount, 1u));

    float3 blendedMean   = (valueCell.sampleCount == 0u)
        ? newMean
        : lerp(oldMean, newMean, 1.0f / totalCountF);

    uint3  blendedSum    = HashGridCache_QuantizeRadiance(blendedMean * totalCountF);

    g_HashGridValue[slot].quantizedRadianceSum = blendedSum;
    g_HashGridValue[slot].sampleCount          = totalCount;
    g_HashGridValue[slot].frameLastTouched     = frameIndex;
    return true;
}

#endif // HASHGRIDCACHE_HAS_VALUE && HASHGRIDCACHE_HAS_SCRATCH

#if defined(HASHGRIDCACHE_HAS_VALUE)

// Read site — looks up the persistent EMA-blended cell value.
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
            HashGridCell cell = g_HashGridValue[slot];
            sampleCount = cell.sampleCount;
            radiance = HashGridCache_DequantizeRadiance(cell.quantizedRadianceSum, sampleCount);
            return true;
        }
    }
    return false;
}

#endif // HASHGRIDCACHE_HAS_VALUE

#endif // HASHGRIDCACHE_HAS_BINDINGS && !HASH_GRID_CACHE_HLSL_BINDINGS
