// shadertype=hlsl
#ifndef PT_HASH_GRID_CACHE_HLSL
#define PT_HASH_GRID_CACHE_HLSL

// PT secondary-vertex hash-grid radiance cache primitives.
//
// Reference: Capsaicin GI-1.0 hash_grid_cache.hlsl
//   https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/hash_grid_cache.hlsl
// Auditor artifact: .alignments/TASK-77.1-rework-paper-port-audit.md
//
// Divergences from the Capsaicin reference, all flagged in the audit:
//   * Direct UAV bindings (engine convention) instead of Capsaicin's
//     space93/96/97/98 unbounded RWStructuredBuffer[] indexed by enum slot.
//   * pcgHash/xxHash bodies inlined here; Capsaicin pulls them from
//     math/pack.hlsl (not fetched, but standard PCG / xxHash32 — chained
//     by addition like Capsaicin).
//   * No ValueIndirectBuffer / MultibounceInfoBuffer; first CL is direct-
//     lobe tracking only. Multi-bounce cell feedback (Capsaicin Site 2,
//     UpdateMultibounceCells) does not fit a loop-per-bounce raygen
//     architecture (D1 deviation) and is deferred.
//   * No screen-probe / VisibilityRayBuffer plumbing — read at secondary
//     vertex inside the raygen loop directly (D1: Site-3 read pattern).
//   * Capacity halved (D2): NUM_BUCKETS_LOG2=13 vs. Capsaicin default 14.
//   * Cache key is direction-keyed (D3, paper-faithful) — surface normal
//     is NOT part of the key.

#include "common.hlsl"

// Capsaicin gi1.h defaults (D2 deviation: capacity halved). Mirrored in
// Source/ExampleProject/RenderingClient/HashGridCacheConstants.h; both must
// hold identical values.
#define PT_HASHGRIDCACHE_NUM_BUCKETS_LOG2          13u
#define PT_HASHGRIDCACHE_NUM_BUCKETS               (1u << PT_HASHGRIDCACHE_NUM_BUCKETS_LOG2)
#define PT_HASHGRIDCACHE_NUM_TILES_PER_BUCKET_LOG2 4u
#define PT_HASHGRIDCACHE_NUM_TILES_PER_BUCKET      (1u << PT_HASHGRIDCACHE_NUM_TILES_PER_BUCKET_LOG2)
#define PT_HASHGRIDCACHE_TILE_CELL_RATIO           8u
#define PT_HASHGRIDCACHE_NUM_TILES                 (PT_HASHGRIDCACHE_NUM_BUCKETS * PT_HASHGRIDCACHE_NUM_TILES_PER_BUCKET)

// Sentinel returned when a probe-budget exhaustion or miss makes the cell
// index meaningless. Mirrors Capsaicin gi1_shared.h's kGI1_InvalidId.
static const uint kPTHashGridCache_InvalidId = 0xFFFFFFFFu;

// Cell-size formula constants — see Capsaicin hash_grid_cache.hlsl:92-93.
// The 1e-3 / 1e3 pair quantizes cell_size_step to the next-power-of-two below
// it; the factors cancel exactly so that hit_cell_size has metres units.
#define PT_HASHGRIDCACHE_STEP_FACTOR 1e3f
#define PT_HASHGRIDCACHE_SIZE_FACTOR 1e-3f

// Atomic-update quantization factor (Capsaicin hash_grid_cache.hlsl:32).
// Radiance is multiplied by this and rounded to uint before InterlockedAdd;
// the reverse converts back to float on read.
#define PT_HASHGRIDCACHE_FLOAT_QUANTIZE 1e3f

// Tile-decay timeout in frames. Capsaicin hash_grid_cache.hlsl:29 —
// kHashGridCache_TileDecay defaults to 50. Mirrored in
// Source/ExampleProject/RenderingClient/HashGridCacheConstants.h::TILE_DECAY_FRAMES.
#define PT_HASHGRIDCACHE_TILE_DECAY 50u

// Per-frame constant buffer mirroring Source/ExampleProject/RenderingClient/
// HashGridCacheConstants.h::HashGridCacheConstants. Values are uploaded by
// GPUPathTracerPass; the size knobs here are kept as compile-time defines so
// the cbuffer can be bound only when PT_HASH_GRID_CACHE_ENABLED is set.
struct PTHashGridCacheCB_t
{
    uint  num_buckets;
    uint  num_tiles_per_bucket;
    uint  tile_cell_ratio;
    uint  num_cells_per_tile;

    uint  size_tile_mip0;
    uint  size_tile_mip1;
    uint  size_tile_mip2;
    uint  size_tile_mip3;

    uint  first_cell_offset_tile_mip0;
    uint  first_cell_offset_tile_mip1;
    uint  first_cell_offset_tile_mip2;
    uint  first_cell_offset_tile_mip3;

    float cell_size;        // angular-pixel footprint per unit distance, CPU-computed per frame
    float min_cell_size;
    float max_sample_count;
    float pad0;
};

// Inline per-axis hashes. Capsaicin sources these from math/pack.hlsl which
// was not fetched in the audit; the chains-by-addition shape (pcgHash(a +
// pcgHash(b + ...))) is replicated here so the bucket and tile-hash keys
// distribute as Capsaicin does, even though the per-call hash bodies are not
// provably identical without the upstream file.
uint PTHashGridCache_pcg(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint PTHashGridCache_xxhash(uint v)
{
    const uint PRIME32_2 = 2246822519u;
    const uint PRIME32_3 = 3266489917u;
    const uint PRIME32_4 = 668265263u;
    const uint PRIME32_5 = 374761393u;
    uint h = PRIME32_5 + 4u;
    h += v * PRIME32_3;
    h = ((h << 17) | (h >> (32 - 17))) * PRIME32_4;
    h ^= h >> 15;
    h *= PRIME32_2;
    h ^= h >> 13;
    h *= PRIME32_3;
    h ^= h >> 16;
    return h;
}

// Power-of-two cell size at a given world-space hit. Capsaicin
// hash_grid_cache.hlsl:96-102. Uses floor(log2(...)) — the integer-cast form
// at line 99 and the floor form at line 141 are equivalent for positive
// arguments; this header uses floor to match GetDesc exactly.
float PTHashGridCache_GetCellSize(in PTHashGridCacheCB_t cb, in float3 eye_position, in float3 hit_position)
{
    float cell_size_step = max(distance(eye_position, hit_position) * cb.cell_size, cb.min_cell_size);
    float log_step_multiplier = floor(log2(PT_HASHGRIDCACHE_STEP_FACTOR * cell_size_step));
    return PT_HASHGRIDCACHE_SIZE_FACTOR * exp2(log_step_multiplier);
}

float PTHashGridCache_GetTileSize(in PTHashGridCacheCB_t cb, in float3 eye_position, in float3 hit_position)
{
    return PTHashGridCache_GetCellSize(cb, eye_position, hit_position) * float(cb.tile_cell_ratio);
}

struct PTHashGridCache_Data
{
    float3 eye_position;
    float3 hit_position;
    float3 direction;
    float  hit_distance;
};

struct PTHashGridCache_Desc
{
    uint  bucket_index;
    uint  tile_hash;
    uint2 cell_offset;
};

// Bucket index + tile hash + cell offset within tile. Capsaicin
// hash_grid_cache.hlsl:133-192. Inputs to the hash chain: mip-level (l),
// quantized tile position (c), 5-bucket-per-axis quantized ray direction (d),
// and a near/far flag (t). Surface normal is intentionally absent — the cache
// stores outgoing radiance toward the ray-direction bucket (D3 audit note).
PTHashGridCache_Desc PTHashGridCache_GetDesc(in PTHashGridCacheCB_t cb, in PTHashGridCache_Data data)
{
    float cell_size_step = max(distance(data.eye_position, data.hit_position) * cb.cell_size, cb.min_cell_size);
    float log_step_multiplier = floor(log2(PT_HASHGRIDCACHE_STEP_FACTOR * cell_size_step));
    float hit_cell_size = PT_HASHGRIDCACHE_SIZE_FACTOR * exp2(log_step_multiplier);
    float hit_tile_size = hit_cell_size * float(cb.tile_cell_ratio);

    float3 signed_c = floor(data.hit_position / hit_tile_size);
    float3 signed_d = floor(0.5f + (0.5f * data.direction + 0.5f) * 4.0f);

    uint  l = uint(log_step_multiplier);
    uint3 c = asuint(int3(signed_c));
    uint3 d = asuint(int3(signed_d));
    uint  t = uint(data.hit_distance < hit_tile_size);

    uint bucket_index = PTHashGridCache_pcg(l +
                        PTHashGridCache_pcg(c.x + PTHashGridCache_pcg(c.y + PTHashGridCache_pcg(c.z +
                        PTHashGridCache_pcg(d.x + PTHashGridCache_pcg(d.y + PTHashGridCache_pcg(d.z +
                        PTHashGridCache_pcg(t)))))))) % cb.num_buckets;

    uint tile_hash  = max(1u,
                       PTHashGridCache_xxhash(l +
                       PTHashGridCache_xxhash(c.x + PTHashGridCache_xxhash(c.y + PTHashGridCache_xxhash(c.z +
                       PTHashGridCache_xxhash(d.x + PTHashGridCache_xxhash(d.y + PTHashGridCache_xxhash(d.z +
                       PTHashGridCache_xxhash(t)))))))));

    float3 e = floor(data.hit_position / hit_cell_size) - floor(data.hit_position / hit_tile_size) * float(cb.tile_cell_ratio);
    float3 abs_direction = abs(data.direction);
    float  max_direction = max(max(abs_direction.x, abs_direction.y), abs_direction.z);

    uint2 cell_offset;
    if (abs_direction.x == max_direction)
        cell_offset = uint2(uint(e.y), uint(e.z));
    else if (abs_direction.y == max_direction)
        cell_offset = uint2(uint(e.x), uint(e.z));
    else
        cell_offset = uint2(uint(e.x), uint(e.y));

    PTHashGridCache_Desc desc;
    desc.bucket_index = bucket_index;
    desc.tile_hash    = tile_hash;
    desc.cell_offset  = cell_offset;
    return desc;
}

// Resolve a tile-local 2D cell offset to the linear cell index inside the
// owning tile. Capsaicin hash_grid_cache.hlsl:207-221, mip 0 only — first CL
// does not write to mips 1-3 yet.
uint PTHashGridCache_CellIndexMip0(in PTHashGridCacheCB_t cb, in uint2 cell_offset_mip0, in uint tile_index)
{
    return tile_index * cb.num_cells_per_tile + cb.first_cell_offset_tile_mip0
         + cell_offset_mip0.x + cell_offset_mip0.y * cb.size_tile_mip0;
}

// Insert (or claim) a cell for the given hit. Capsaicin
// hash_grid_cache.hlsl:249-278. Open-addressing probe over num_tiles_per_bucket
// slots; on probe-budget exhaustion returns kPTHashGridCache_InvalidId and
// the caller silently drops the contribution.
uint PTHashGridCache_InsertCell(in PTHashGridCacheCB_t cb,
                                in PTHashGridCache_Data data,
                                RWStructuredBuffer<uint> hash_buffer,
                                out uint tile_index,
                                out bool is_new_tile)
{
    is_new_tile = false;
    tile_index = kPTHashGridCache_InvalidId;
    PTHashGridCache_Desc desc = PTHashGridCache_GetDesc(cb, data);

    uint bucket_offset;
    for (bucket_offset = 0u; bucket_offset < cb.num_tiles_per_bucket; ++bucket_offset)
    {
        uint previous_hash;
        tile_index = bucket_offset + desc.bucket_index * cb.num_tiles_per_bucket;
        InterlockedCompareExchange(hash_buffer[tile_index], 0u, desc.tile_hash, previous_hash);
        if (previous_hash == 0u)
        {
            is_new_tile = true;
            break;
        }
        if (previous_hash == desc.tile_hash)
            break;
    }

    if (bucket_offset >= cb.num_tiles_per_bucket)
        return kPTHashGridCache_InvalidId;

    return PTHashGridCache_CellIndexMip0(cb, desc.cell_offset, tile_index);
}

// Read-only lookup. Capsaicin hash_grid_cache.hlsl:281-298. Returns
// kPTHashGridCache_InvalidId when the cell is not present in the bucket.
// Reserved for the read-site CL that follows; first CL is write-only.
uint PTHashGridCache_FindCell(in PTHashGridCacheCB_t cb,
                              in PTHashGridCache_Data data,
                              StructuredBuffer<uint> hash_buffer,
                              out uint tile_index)
{
    tile_index = kPTHashGridCache_InvalidId;
    PTHashGridCache_Desc desc = PTHashGridCache_GetDesc(cb, data);

    uint bucket_offset;
    for (bucket_offset = 0u; bucket_offset < cb.num_tiles_per_bucket; ++bucket_offset)
    {
        tile_index = bucket_offset + desc.bucket_index * cb.num_tiles_per_bucket;
        uint previous_hash = hash_buffer[tile_index];
        if (previous_hash == 0u)
            return kPTHashGridCache_InvalidId;
        if (previous_hash == desc.tile_hash)
            break;
    }

    if (bucket_offset >= cb.num_tiles_per_bucket)
        return kPTHashGridCache_InvalidId;

    return PTHashGridCache_CellIndexMip0(cb, desc.cell_offset, tile_index);
}

// Quantize float radiance to uint for atomic accumulation. Capsaicin
// hash_grid_cache.hlsl:301-306; the .w component is set to 1 so each ray's
// contribution increments the per-cell sample count by exactly one.
uint4 PTHashGridCache_QuantizeRadiance(in float3 radiance)
{
    return uint4(uint(round(PT_HASHGRIDCACHE_FLOAT_QUANTIZE * max(radiance.x, 0.0f))),
                 uint(round(PT_HASHGRIDCACHE_FLOAT_QUANTIZE * max(radiance.y, 0.0f))),
                 uint(round(PT_HASHGRIDCACHE_FLOAT_QUANTIZE * max(radiance.z, 0.0f))), 1u);
}

// Inverse of QuantizeRadiance; recovers the float-space sum and sample count
// from the atomic scratch buffer. Capsaicin hash_grid_cache.hlsl:309-315.
float4 PTHashGridCache_RecoverRadiance(in uint4 quantized_radiance)
{
    return float4(float(quantized_radiance.x) / PT_HASHGRIDCACHE_FLOAT_QUANTIZE,
                  float(quantized_radiance.y) / PT_HASHGRIDCACHE_FLOAT_QUANTIZE,
                  float(quantized_radiance.z) / PT_HASHGRIDCACHE_FLOAT_QUANTIZE,
                  float(quantized_radiance.w));
}

// fp16-pack a float4 into uint2 for ValueBuffer storage. Capsaicin
// hash_grid_cache.hlsl:355-365 calls packHalf4 / unpackHalf4 from
// math/pack.hlsl (not fetched in the audit); the 4 × f32tof16 / f16tof32
// shape is the standard implementation and the only round-trip-exact way to
// fit (rgb mean × sample_count, sample_count) into an 8-byte cell. The
// upcast to float widens to f32 for arithmetic; the down-cast on store
// truncates back to f16 — the same loss of precision Capsaicin accepts.
uint2 PTHashGridCache_PackRadiance(in float4 radiance)
{
    uint2 packed;
    packed.x = (f32tof16(radiance.x) & 0xFFFFu) | (f32tof16(radiance.y) << 16u);
    packed.y = (f32tof16(radiance.z) & 0xFFFFu) | (f32tof16(radiance.w) << 16u);
    return packed;
}

float4 PTHashGridCache_UnpackRadiance(in uint2 packed_radiance)
{
    return float4(f16tof32(packed_radiance.x & 0xFFFFu),
                  f16tof32(packed_radiance.x >> 16u),
                  f16tof32(packed_radiance.y & 0xFFFFu),
                  f16tof32(packed_radiance.y >> 16u));
}

#endif // PT_HASH_GRID_CACHE_HLSL
