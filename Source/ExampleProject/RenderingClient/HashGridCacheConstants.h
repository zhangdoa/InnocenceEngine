#pragma once
#include <cstdint>

// C++ mirror of the PT hash-grid radiance cache layout constants defined in
// Source/Shaders/HLSL/common/PTHashGridCache.hlsl. The shader header owns the
// canonical values; this file mirrors them so GPUPathTracerPass can size the
// UAVs and upload the constant buffer without redefining them per pass.
//
// Reference: Capsaicin GI-1.0 hash_grid_cache.hlsl
// (gi1.h:60-65 — gi1_hash_grid_cache_num_buckets / num_tiles_per_bucket /
//  tile_cell_ratio / max_sample_count defaults).
//
// Drift between this header and the HLSL #defines manifests as out-of-bounds
// UAV access or over-allocation. Keep the values in lockstep.

namespace Inno
{
namespace PTHashGridCache
{
    // Master compile-time toggle. When false, GPUPathTracerPass does not
    // allocate or bind cache buffers, and the corresponding #if-gated cache
    // code in GPUPathTracerRayGen.hlsl strips out at compile time. Output is
    // bit-identical to the cache-off baseline at HEAD 10d7b158.
    //
    // The HLSL side owns the #define PT_HASH_GRID_CACHE_ENABLED in
    // GPUPathTracerRayGen.hlsl; both must hold the same value. With the
    // toggle on but reads not yet wired, the cache is written-only — visual
    // output remains parity with toggle-off, validating the plumbing before
    // read sites land in subsequent CLs.
    static constexpr bool ENABLED = false;

    // 2^13 buckets × 2^4 tiles/bucket × 8×8 cells/tile (mip0) ≈ 8.4M cells at
    // mip0; including mips 1-3 (4×4 + 2×2 + 1) the per-tile cell count is 85
    // and the all-mip total is ~11.1M cells. Sized for the balanced middle of
    // the auditor's recommended 5-10M-cell range (D2 deviation): cuts
    // Capsaicin's default ~22M-cell budget in half while still leaving headroom
    // for Sponza-class scenes. Power-of-two num_buckets keeps the bucket-index
    // hash modulo cheap.
    //
    // Buffer footprint at this sizing (mip0 cells = num_buckets * num_tiles_per_bucket * size_tile_mip0^2):
    //   ValueBuffer            (uint2 = 8B/cell)  ≈  88 MB
    //   UpdateCellValueBuffer  (4*uint = 16B/cell)≈ 176 MB
    //   HashBuffer / DecayBuf  (uint per tile, 131072 tiles each) ≈ 1 MB total
    // Total ≈ 265 MB without USE_MULTI_BOUNCE mirrors.
    static constexpr uint32_t NUM_BUCKETS_LOG2          = 13u;
    static constexpr uint32_t NUM_BUCKETS               = 1u << NUM_BUCKETS_LOG2;
    static constexpr uint32_t NUM_TILES_PER_BUCKET_LOG2 = 4u;
    static constexpr uint32_t NUM_TILES_PER_BUCKET      = 1u << NUM_TILES_PER_BUCKET_LOG2;
    static constexpr uint32_t TILE_CELL_RATIO           = 8u;
    static constexpr uint32_t SIZE_TILE_MIP0            = TILE_CELL_RATIO;
    static constexpr uint32_t SIZE_TILE_MIP1            = SIZE_TILE_MIP0 >> 1;     // 4
    static constexpr uint32_t SIZE_TILE_MIP2            = SIZE_TILE_MIP0 >> 2;     // 2
    static constexpr uint32_t SIZE_TILE_MIP3            = SIZE_TILE_MIP0 >> 3;     // 1
    static constexpr uint32_t NUM_CELLS_PER_TILE_MIP0   = SIZE_TILE_MIP0 * SIZE_TILE_MIP0;
    static constexpr uint32_t NUM_CELLS_PER_TILE_MIP1   = SIZE_TILE_MIP1 * SIZE_TILE_MIP1;
    static constexpr uint32_t NUM_CELLS_PER_TILE_MIP2   = SIZE_TILE_MIP2 * SIZE_TILE_MIP2;
    static constexpr uint32_t NUM_CELLS_PER_TILE_MIP3   = SIZE_TILE_MIP3 * SIZE_TILE_MIP3;
    static constexpr uint32_t NUM_CELLS_PER_TILE        = NUM_CELLS_PER_TILE_MIP0 + NUM_CELLS_PER_TILE_MIP1
                                                        + NUM_CELLS_PER_TILE_MIP2 + NUM_CELLS_PER_TILE_MIP3;
    static constexpr uint32_t NUM_TILES                 = NUM_BUCKETS * NUM_TILES_PER_BUCKET;
    static constexpr uint32_t NUM_CELLS                 = NUM_TILES * NUM_CELLS_PER_TILE;

    // First-cell offsets within a tile (mip cells are linearly packed).
    static constexpr uint32_t FIRST_CELL_OFFSET_TILE_MIP0 = 0u;
    static constexpr uint32_t FIRST_CELL_OFFSET_TILE_MIP1 = FIRST_CELL_OFFSET_TILE_MIP0 + NUM_CELLS_PER_TILE_MIP0;
    static constexpr uint32_t FIRST_CELL_OFFSET_TILE_MIP2 = FIRST_CELL_OFFSET_TILE_MIP1 + NUM_CELLS_PER_TILE_MIP1;
    static constexpr uint32_t FIRST_CELL_OFFSET_TILE_MIP3 = FIRST_CELL_OFFSET_TILE_MIP2 + NUM_CELLS_PER_TILE_MIP2;

    // Cap on per-cell sample count for the running-mean update (Capsaicin
    // gi1.h:63 — gi1_hash_grid_cache_max_sample_count default 16). Once a
    // cell saturates, new contributions blend at weight 1/16. The previous
    // attempt used 32; Capsaicin's reference value is 16.
    static constexpr float MAX_SAMPLE_COUNT = 16.0f;

    // Min cell size floor in metres (Capsaicin gi1.h:59 — default 0.1m).
    // Keeps the log2 input strictly positive for sub-millimetre eye-to-hit
    // distances and sets the smallest cell at the closest hits.
    static constexpr float MIN_CELL_SIZE = 0.1f;

    // Knob multiplier on the angular-pixel footprint in the cell-size formula
    // (Capsaicin gi1.h:58 — gi1_hash_grid_cache_cell_size default 32.0; the
    // factor is roughly "32 vertical pixels in radians"). The CPU side uses
    // this to compute the per-frame angular_footprint constant uploaded with
    // the cache CB.
    static constexpr float CELL_SIZE_KNOB = 32.0f;

    // Number of frames before an unused tile is evicted (Capsaicin
    // hash_grid_cache.hlsl:29 — kHashGridCache_TileDecay 50). First-CL
    // tracking-only writes do not yet run a PurgeTiles pass; the constant
    // is mirrored here so the eventual purge pass agrees with the shader.
    static constexpr uint32_t TILE_DECAY_FRAMES = 50u;

    // Constant-buffer layout shared with the shader (CPU-side struct mirrored
    // by cbuffer HashGridCacheCB in PTHashGridCache.hlsl). 16-byte aligned
    // for HLSL constant-buffer packing; pad fields are part of the contract.
    struct HashGridCacheConstants
    {
        uint32_t num_buckets;
        uint32_t num_tiles_per_bucket;
        uint32_t tile_cell_ratio;
        uint32_t num_cells_per_tile;          // 16 B

        uint32_t size_tile_mip0;
        uint32_t size_tile_mip1;
        uint32_t size_tile_mip2;
        uint32_t size_tile_mip3;              // 32 B

        uint32_t first_cell_offset_tile_mip0;
        uint32_t first_cell_offset_tile_mip1;
        uint32_t first_cell_offset_tile_mip2;
        uint32_t first_cell_offset_tile_mip3; // 48 B

        float    cell_size;                   // angular-pixel footprint computed CPU-side per frame
        float    min_cell_size;
        float    max_sample_count;
        float    pad0;                        // 64 B
    };
    static_assert(sizeof(HashGridCacheConstants) == 64,
                  "HashGridCacheConstants must match the cbuffer layout in PTHashGridCache.hlsl");
}
} // namespace Inno
