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
//   - 20 B per-cell payload (radiance + sampleCount + frameLastTouched) + 4 B
//     key. No mip-level filtering, no separate indirect/multibounce buffers,
//     no decay buffer — those pay off at secondary-vertex usage which is
//     phase-2 work.
//   - Write site: PT primary hit only (Capsaicin writes at secondary).
//
// The consumer compiles this header after declaring the buffer bindings
// (g_HashGridKeys, g_HashGridCells) and the per-frame view of g_FrameCount.
// Both buffers must be RWStructuredBuffer<...> bound at the same descriptor
// set so atomics and stores resolve to the same physical resource.

// Per-cell payload. Layout matches HashGridCacheConstants::CELL_BYTES (20 B).
struct HashGridCell
{
    float3 radiance;
    uint   sampleCount;
    uint   frameLastTouched;
};

// Mirror of HashGridCacheConstants.h on the C++ side. Keep in sync.
static const uint HASHGRID_CELL_COUNT   = (1u << 20);
static const uint HASHGRID_PROBE_LENGTH = 4u;
static const uint HASHGRID_SAMPLE_CAP   = 256u;

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

// Online running-mean update with sample-cap. Past the cap the update
// becomes a sliding mean — fresh samples carry weight 1/SAMPLE_CAP
// indefinitely, so a relit surface drifts toward the new value without
// explicit invalidation.
float3 HashGridCache_OnlineMeanUpdate(float3 oldRadiance, uint oldSampleCount, float3 newSample)
{
    float weight = 1.0 / float(min(oldSampleCount + 1u, HASHGRID_SAMPLE_CAP));
    return lerp(oldRadiance, newSample, weight);
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
    // unconditionally — at this point the slot is a different cell with
    // staler radiance than ours, and serving its old radiance to the
    // caller's key would be wrong. The new sample becomes the seed.
    g_HashGridKeys[oldestSlot] = key;
    isNewInsert = true;
    return oldestSlot;
}

void HashGridCache_Insert(float3 posWS, float3 normal, float3 radiance, float cellSize, uint frameIndex)
{
    uint key = HashGridCache_BuildKey(posWS, normal, cellSize);
    bool isNewInsert;
    uint slot = HashGridCache_InsertOrFind(key, frameIndex, isNewInsert);

    if (isNewInsert)
    {
        g_HashGridCells[slot].radiance         = radiance;
        g_HashGridCells[slot].sampleCount      = 1u;
        g_HashGridCells[slot].frameLastTouched = frameIndex;
    }
    else
    {
        // Race-tolerant update: a concurrent inserter may also be reading
        // and writing this cell. Without atomics on the float3 we accept
        // a benign last-writer-wins race; a single dropped update is
        // invisible against the running mean.
        HashGridCell cell = g_HashGridCells[slot];
        float3 updated = HashGridCache_OnlineMeanUpdate(cell.radiance, cell.sampleCount, radiance);
        g_HashGridCells[slot].radiance         = updated;
        g_HashGridCells[slot].sampleCount      = min(cell.sampleCount + 1u, HASHGRID_SAMPLE_CAP);
        g_HashGridCells[slot].frameLastTouched = frameIndex;
    }
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
            radiance    = cell.radiance;
            sampleCount = cell.sampleCount;
            return true;
        }
    }
    return false;
}

#endif // HASHGRIDCACHE_HAS_BINDINGS && !HASH_GRID_CACHE_HLSL_BINDINGS
