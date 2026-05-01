# TASK-77.1 rework — Capsaicin GI-1.0 hash-grid radiance cache: paper-port audit

| Field | Value |
|---|---|
| Audit date | 2026-05-01 |
| Auditor | paper-auditor agent |
| Output venue | `.alignments/TASK-77.1-rework-paper-port-audit.md` |
| Reference repo | `https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin` (default branch `master`, fetched via GitHub API at audit time) |
| Reference paths | `src/core/src/render_techniques/gi1/{hash_grid_cache.hlsl, gi1.comp, gi1.h, gi1_shared.h, gi1.cpp}` |
| Local snapshot | `.alignments/_audit_refs/{hash_grid_cache.hlsl, gi1.comp, gi1.h, gi1_shared.h, gi1.cpp}` (fetched 2026-05-01; line numbers in this artifact reference these snapshots) |
| In-house implementation | **DOES NOT EXIST YET.** This is a *paper-port pre-pass* per the design doc `.alignments/TASK-77.1-pt-hash-grid-cache-rework-design.md`, dispatched before the implementer authors `Source/Shaders/HLSL/common/PTHashGridCache.hlsl`. The 9 questions below resolve unknowns the implementer cannot answer alone. |
| Counted summary | **9 / 9 questions answered from explicit Capsaicin source.** No questions left UNRESOLVED. **3 deviations identified** that the implementer + dispatcher must adjudicate (see "Deviations the rework should consider"). |

---

## 1. Cell-size formula constants (Q1)

**Status: ANSWERED — explicit in source.**

The formula in `hash_grid_cache.hlsl:96-102` (function `HashGridCache_GetCellSize`) is:

```hlsl
#define HASHGRIDCACHE_STEP_FACTOR 1e3f                                  // line 92
#define HASHGRIDCACHE_SIZE_FACTOR 1e-3f                                 // line 93

float HashGridCache_GetCellSize(in float3 position) {                   // line 96
    float cell_size_step = max(distance(g_Eye, position) * g_HashGridCacheConstants.cell_size,
                               g_HashGridCacheConstants.min_cell_size);  // line 98
    uint log_step_multiplier = uint(log2(HASHGRIDCACHE_STEP_FACTOR * cell_size_step));  // line 99
    float hit_cell_size = HASHGRIDCACHE_SIZE_FACTOR * exp2(log_step_multiplier);         // line 100
    return hit_cell_size;
}
```

The variant in `HashGridCache_GetDesc` (`hash_grid_cache.hlsl:140-142`) uses `floor(log2(...))` instead of `uint(log2(...))` — these are equivalent for positive arguments, but the floor-form is the exact spec and what `GetDesc` uses for the bucket-key hash:

```hlsl
float cell_size_step = max(distance(eye_position, hit_position) * g_HashGridCacheConstants.cell_size,
                           g_HashGridCacheConstants.min_cell_size);                       // line 140
float log_step_multiplier = floor(log2(HASHGRIDCACHE_STEP_FACTOR * cell_size_step));     // line 141
float hit_cell_size = HASHGRIDCACHE_SIZE_FACTOR * exp2(log_step_multiplier);              // line 142
```

### Constants

| Symbol | Value | Source |
|---|---|---|
| `HASHGRIDCACHE_STEP_FACTOR` | `1e3f` | `hash_grid_cache.hlsl:92` |
| `HASHGRIDCACHE_SIZE_FACTOR` | `1e-3f` | `hash_grid_cache.hlsl:93` |

### What `cell_size` (the constant-buffer field) actually is

It is **NOT a world-space distance**. It is an **angular footprint per unit distance** computed CPU-side from camera FOV and a user knob:

`gi1.cpp:1856-1859`:
```cpp
float const cell_size = tanf(camera.fovY * options_.gi1_hash_grid_cache_cell_size
                             * GFX_MAX(1.0F / static_cast<float>(render_dimensions.y),
                                       static_cast<float>(render_dimensions.y) /
                                       static_cast<float>(render_dimensions.x * render_dimensions.x)));
```

User knob default: `gi1_hash_grid_cache_cell_size = 32.0F` (`gi1.h:58`). The factor is roughly "32 vertical pixels in radians."

So `cell_size_step = distance(eye, hit) * angular_footprint` — a screen-space-pixel-equivalent **world-space distance** at the hit. Then it is power-of-two-quantized:

`hit_cell_size = 1e-3 * 2^floor(log2(1e3 * cell_size_step))`

The `1e-3 * 2^floor(log2(1e3 * x))` shape is "next-power-of-two below `x`", expressed in metres because `1e-3 / 1e3 = 1.0` (the constants cancel exactly — they exist so the `log2` argument never goes through `0`/negative for sub-millimetre `x`).

`min_cell_size` (default `1e-1F` = 10 cm, `gi1.h:59`) is the floor on the input to `log2` — it sets the smallest cell.

### Quantization shape

Power-of-two world-space cell size, growing with view distance and a user-tunable angular footprint. Cells near the eye are small; cells far from the eye are large; both grow as discrete pow-2 steps so that nearby and faraway hits never share a cell.

### Match against the previous attempt's transcription

The design doc described this as "`cellSize = SIZE_FACTOR * exp2(floor(log2(STEP_FACTOR * step)))` was the previous attempt's transcription." That transcription is **correct in form**, with constants `SIZE_FACTOR = 1e-3f` and `STEP_FACTOR = 1e3f`, but the `step` input is **NOT** "view-space distance" or "world-space pixel footprint" alone — it is `distance(eye, hit) * angular_footprint`, where the angular footprint is itself derived from `tan(fovY * user_knob * something_or_other_per_pixel)`. The previous attempt's transcription is form-correct; the input definition is the load-bearing piece that needs to come from the CPU side.

---

## 2. Cache capacity / sizing (Q2)

**Status: ANSWERED — explicit in source.**

Capacity is computed CPU-side in `gi1.cpp:442-461`. With the default knobs from `gi1.h:60-62`:

| User knob | Default | Meaning |
|---|---|---|
| `gi1_hash_grid_cache_num_buckets` | `14` | `1U << 14 = 16384` buckets total |
| `gi1_hash_grid_cache_num_tiles_per_bucket` | `4` | `1U << 4 = 16` tiles per bucket |
| `gi1_hash_grid_cache_tile_cell_ratio` | `8` | Each tile is 8×8 cells at mip0 |

Derived (`gi1.cpp:442-461`):

```cpp
uint32_t const num_buckets          = 1U << options.gi1_hash_grid_cache_num_buckets;    // 16384
uint32_t const num_tiles_per_bucket = 1U << options.gi1_hash_grid_cache_num_tiles_per_bucket;  // 16
uint32_t const size_tile_mip0       = options.gi1_hash_grid_cache_tile_cell_ratio;       // 8
uint32_t const num_cells_per_tile_mip0 = size_tile_mip0 * size_tile_mip0;                // 64
// (mip1/mip2/mip3 use 4/2/1 respectively, sum_all_mips ~= 64+16+4+1 = 85 cells per tile)
uint32_t const num_tiles  = num_tiles_per_bucket * num_buckets;                          // 262144
uint32_t const num_cells  = num_cells_per_tile * num_tiles;                              // ~22 million cells (all mips)
```

### Buffer sizing (per-buffer, per `num_cells = num_cells_per_tile * num_tiles`)

- `ValueBuffer` (mip0+1+2+3 cells): `uint2` per cell = 8 bytes/cell. ~22M cells × 8B = ~176 MB.
- `ValueIndirectBuffer` (only when `USE_MULTI_BOUNCE`): another ~176 MB.
- `UpdateCellValueBuffer` (atomic-int scratch): `4 × uint` per cell = 16 bytes/cell. ~22M cells × 16B = ~352 MB.
- `UpdateCellValueIndirectBuffer` (only when `USE_MULTI_BOUNCE`): another ~352 MB.
- `HashBuffer`: `uint` per tile = `262144 × 4 = 1 MB`.
- `DecayTileBuffer`: `uint` per tile = 1 MB.

**Total at default config with multibounce**: ~1 GB across the buffer family.

### Without `USE_MULTI_BOUNCE`

Roughly halved (no `*Indirect*` mirrors). ~530 MB.

### Tunable vs fixed

All knobs are exposed via `RenderOptions` (`gi1.h:48-89`) and re-read in `convertOptions`. **Resize on knob change** is wired in `gi1.cpp:470, 487, 499, 523, 537, 590, 642, 721` — every buffer is re-allocated when `num_tiles` or `num_cells` or `num_buckets` changes between init and current frame. So Capsaicin **does** support dynamic resize at the cost of reallocating each buffer.

### Match against design-doc estimate

The design doc estimated "~`2^20` cells × ~20 B = ~20 MB." Capsaicin's default is **~22 million cells (`2^24`-ish)** at ~24 B/cell across direct-only buffers, totalling ~530 MB without multibounce / ~1 GB with multibounce. **The design's estimate is ~25× too small for the cell count.** This is one of the deviations the implementer must adjudicate (see "Deviations" below).

---

## 3. Sample-cap / running-mean weighting (Q3)

**Status: ANSWERED — explicit in source.**

In `gi1.comp:2160-2200` (`UpdateTiles` kernel, MIP 0 block):

```hlsl
// Direct lobe (lines 2165-2181)
float4 direct_radiance     = HashGridCache_UnpackRadiance(g_HashGridCache_ValueBuffer[cell_index]);  // accumulated, with .w = sample count
float4 new_direct_radiance = HashGridCache_RecoverRadiance(uint4(...UpdateCellValueBuffer 4-tuple...));  // this-frame deltas, .w = this-frame sample count

float direct_sample_count = min(direct_radiance.w + new_direct_radiance.w,
                                g_HashGridCacheConstants.max_sample_count);  // 2170
direct_radiance     /= max(direct_radiance.w, 1.0f);
new_direct_radiance /= max(new_direct_radiance.w, 1.0f);
if (direct_radiance.w <= 0.0f) {
    direct_radiance = new_direct_radiance;
} else {
    direct_radiance = lerp(direct_radiance, new_direct_radiance, 1.0f / direct_sample_count);  // 2179
}
direct_radiance *= direct_sample_count;  // 2181 — sample count is used as a hint for picking prefiltering amount
```

Same shape for the indirect lobe (`gi1.comp:2182-2199`) but using `max_multibounce_sample_count`.

### Caps

| Field | Default | Source |
|---|---|---|
| `max_sample_count` | `16.0F` | `gi1.h:63` |
| `max_multibounce_sample_count` | `16.0F` | `gi1.h:65` |

### Weighting form

Running-mean — **NOT** a fixed-α EMA. `lerp(prev_mean, new_frame_mean, 1/N)` where `N = min(prev_count + new_count, max_count)`. Once `N == max_count`, the new-frame contribution stabilises at weight `1/max_count = 1/16`, equivalent to a low-pass filter with effective half-life of ~11 frames. Before saturation, it's a true running mean (each sample weighted equally).

### Storage convention

The cell stores `radiance_total` (NOT mean) and a sample count in `.w`:
- After `UpdateTiles`, `g_HashGridCache_ValueBuffer[cell].rgb = mean_radiance * sample_count` and `.w = sample_count`.
- Readers (`HashGridCache_FilteredRadianceDirect`, used at lines 1962, 2377, 2891) recover the mean via `radiance.rgb / max(radiance.w, 1.0f)`.

This is so the box-filter mip cascade in `UpdateTiles` (lines 2227-2347) can sum 4 cells without extra division, and so atomic accumulation into the scratch buffer is straightforward (radiance × sample_count is what's stored; sample-count `.w` is incremented by `1` per contribution at line 2095).

### Match against the previous attempt

The previous attempt used "sample-count cap of 32." Capsaicin uses **16** for both direct and multibounce. Form-equivalent (a cap with running-mean), value-different.

### Sample-count increment

`gi1.comp:2087, 2095`: `quantized_radiance.w` is set to `1` in `HashGridCache_QuantizeRadiance` (line 305: `uint(1)`), and `InterlockedAdd`'d into `UpdateCellValueBuffer[4*cell + 3]`. So each ray that contributes to a cell adds exactly 1 to the sample count for that frame. The `min(...)` in `UpdateTiles` caps this at 16.

---

## 4. Eviction-on-collision policy (Q4)

**Status: ANSWERED — explicit in source.**

There are **two distinct eviction mechanisms** in Capsaicin's cache:

### (a) Probe-budget exhaustion on insert — **silent skip, no replacement**.

`hash_grid_cache.hlsl:249-278` (`HashGridCache_InsertCell`):

```hlsl
for (bucket_offset = 0; bucket_offset < g_HashGridCacheConstants.num_tiles_per_bucket; ++bucket_offset) {
    tile_index = bucket_offset + desc.bucket_index * g_HashGridCacheConstants.num_tiles_per_bucket;
    InterlockedCompareExchange(g_HashGridCache_HashBuffer[tile_index], 0, desc.tile_hash, previous_hash);
    if (previous_hash == 0) { is_new_tile = true; break; }                  // empty slot → claim it
    if (previous_hash == desc.tile_hash) break;                              // existing match → reuse
}
if (bucket_offset >= g_HashGridCacheConstants.num_tiles_per_bucket) {
#ifdef DEBUG_HASH_STATS
    InterlockedAdd(g_HashGridCache_DebugStatsBucketOverflowCountBuffer[desc.bucket_index], 1, ...);
#endif
    return kGI1_InvalidId;  // too many collisions, out of tiles :(
}
```

Probe budget = `num_tiles_per_bucket = 16` slots per bucket. On exhaustion: **return `kGI1_InvalidId` and abandon the insert**. No LRU replacement, no lowest-confidence replacement. The site that called `InsertCell` checks `cell_index != kGI1_InvalidId` and skips the entire enqueue (e.g. `gi1.comp:998`). The bucket becomes saturated for the rest of the frame.

### (b) Tile decay (LRU) — **eviction by frame-age**.

`hash_grid_cache.hlsl:29`: `#define kHashGridCache_TileDecay 50` — 50 frames.

`gi1.comp:1715-1752` (`PurgeTiles` kernel, runs on `previous_packed_tile_index_buffer`):

```hlsl
uint tile_decay = g_HashGridCache_DecayTileBuffer[tile_index];
// (handles wraparound)
tile_decay = (g_FrameIndex - tile_decay);

if (tile_decay >= kHashGridCache_TileDecay) {
    g_HashGridCache_HashBuffer[tile_index] = 0;     // free the slot
    return;  // kill the tile
}
// otherwise keep the tile alive and re-pack it
InterlockedAdd(g_HashGridCache_PackedTileCountBuffer[0], 1, packed_tile_index);
g_HashGridCache_PackedTileIndexBuffer[packed_tile_index] = tile_index;
```

`DecayTileBuffer[tile]` is `InterlockedExchange`'d to the current `g_FrameIndex` *every time the cell is touched* (both insert sites: `gi1.comp:1002, 1777`; and the read site for glossy: `gi1.comp:2888`). Tiles untouched for ≥50 frames are freed back to the pool — slot becomes claimable again.

### Order of operations

`PurgeTiles` runs *before* `PopulateScreenProbes` each frame, so freed slots are available for the current frame's inserts. Otherwise the bucket would fill and stay full.

### Bucket overflow stats

When `DEBUG_HASH_STATS` is on, overflow events per bucket are counted. Default off; the implementation does not act on overflow other than logging.

---

## 5. Cache key composition (Q5)

**Status: ANSWERED — explicit in source.**

`hash_grid_cache.hlsl:133-192` (`HashGridCache_GetDesc`). The key is composed from **8 inputs** combined via `pcgHash` chain (for bucket index) and `xxHash` chain (for tile hash):

| Input | Encoding | Source |
|---|---|---|
| `l` (cell-size mip level) | `uint(log_step_multiplier)` from `floor(log2(1e3 * cell_size_step))` | line 148 |
| `c.x, c.y, c.z` | `asuint(int3(floor(hit_position / hit_tile_size)))` | line 145, 149 — quantized **tile position**, not cell position |
| `d.x, d.y, d.z` | `asuint(int3(floor(0.5 + (0.5*direction + 0.5) * 4.0)))` | line 146, 150 — quantized **ray direction** (3-bit per axis: 5 buckets `{-1, -0.5, 0, 0.5, 1}` because `floor(0.5 + x*4)` for `x ∈ [0,1]` gives `{0,1,2,3,4}`) |
| `t` | `uint(hit_distance < hit_tile_size)` | line 151 — **near/far flag** based on whether the hit is within one tile-radius of the eye |

```hlsl
uint bucket_index = pcgHash(l +                                                             // line 153
                    pcgHash(c.x + pcgHash(c.y + pcgHash(c.z +
                    pcgHash(d.x + pcgHash(d.y + pcgHash(d.z +
                    pcgHash(t)))))))) % g_HashGridCacheConstants.num_buckets;

uint tile_hash  = max(1, xxHash(l + xxHash(c.x + xxHash(c.y + xxHash(c.z +                  // line 158
                                xxHash(d.x + xxHash(d.y + xxHash(d.z +
                                xxHash(t)))))))));
```

Then within a tile, the cell offset (`hash_grid_cache.hlsl:164-179`) is the floor-quantized `hit_position / cell_size` modulo the tile, *projected onto the dominant axis of the direction* (so each tile has 64 cells = 8×8 grid in screen-space-aligned coordinates). The cell-offset is **NOT** part of the hash key — multiple cells within one tile share a tile_hash; collisions inside a tile are by-design (different cells of the same tile resolve to different array slots via `tile_index * num_cells_per_tile + cell_offset`).

### Important: NO normal binning

The key includes the **ray direction**, *not* the surface normal. The previous attempt's design assumed octahedral-packed normal in the key; Capsaicin uses **incoming-ray direction** (the direction from the eye/origin to the hit, line 137: `data.direction = ray.direction`) in 3-bit-per-axis quantized form. This is fundamentally different from a normal-keyed cache: two rays hitting the same world position from very different directions land in different tiles. Two rays hitting the same world position from similar directions (same pow-2 quantized direction bucket) share a tile, regardless of surface normal.

The choice has consequences (see "Deviations" below): a concave corner where two walls meet can land in the same tile if the camera-to-hit directions are close enough — but this is mitigated by the cell-offset projection onto the dominant ray axis (line 167-179), which separates the two walls if their normals point along different cardinal axes.

### Discriminators

- **Material ID**: NOT in the key. Two adjacent surfaces with different albedos that share a cell share radiance.
- **Mip-level (`l`)**: IN the key. Hits at different distances → different `cell_size_step` → different `l` → different tile.
- **Near/far flag (`t`)**: IN the key. Hits closer than `tile_size` from the eye are tagged separately. This separates self-illumination of nearby geometry from far-field caching.

### Hash function

`pcgHash` and `xxHash` are defined in `math/pack.hlsl` (not fetched, but their use is standard PCG and xxHash-32 respectively, chained with `+` to combine inputs). The chain composition is associative-by-addition, so order of inputs does not commute trivially — the auditor cannot verify this without `pack.hlsl`, but the key takeaway for the rework is that any port must use *the same hash chain* to guarantee key parity if cell-content is ever shared across implementations (it won't be — the rework is independent — but the chain shape is what the rework must replicate per `paper-port.md`).

---

## 6. Secondary-vertex cache READ site — the structural shift (Q6)

**Status: ANSWERED — explicit in source.**

This is the load-bearing question. The answer has multiple distinct read sites with different semantics. **The cache is NOT read at the primary hit anywhere in Capsaicin.** It is read only at secondary or later vertices, and even then only as a **terminator** (replacing further trace) or as **indirect feedback** between cells.

### Site 1 (PRIMARY READ — feeds screen probes): `ResolveCells` kernel, `gi1.comp:2350-2385`

```hlsl
[numthreads(64, 1, 1)]
void ResolveCells(in uint did : SV_DispatchThreadID) {
    // ...
    uint cell_index  = g_HashGridCache_VisibilityCellBuffer[visibility_index];
    uint query_index = g_HashGridCache_VisibilityQueryBuffer[visibility_index];

    float4 direct_radiance;
    HashGridCache_FilteredRadianceDirect(cell_index, false, direct_radiance);              // line 2377 — READ
    float3 radiance = (direct_radiance.rgb / max(direct_radiance.w, 1.0f));
#ifdef USE_MULTI_BOUNCE
    float4 indirect_radiance;
    HashGridCache_FilteredRadianceIndirect(cell_index, false, indirect_radiance);          // line 2381 — READ
    radiance += (indirect_radiance.rgb / max(indirect_radiance.w, 1.0f));
#endif
    ScreenProbes_AccumulateRadiance(query_index, radiance);                                // line 2384
}
```

**This is the only read site that emits radiance back to the integrator's primary consumer** (the screen probe). `query_index` traces back through `VisibilityQueryBuffer` to `did` of the original screen-probe ray (set at `gi1.comp:1016`). The screen probe is the structure spawned **from the primary visible point** — it is one bounce away from the camera. So **the cache is read at the secondary vertex, and its filtered radiance is the bounce-1 indirect estimator returned to the primary pixel.**

`HashGridCache_FilteredRadianceDirect` (`hash_grid_cache.hlsl:465-494`) walks mip0 → mip3 of the cell, picking the highest-mip whose accumulated sample count meets `max_sample_count`. Lower-confidence cells fall back to higher-mip (more box-filtered) values. This is intra-tile spatial denoising baked into the read.

### Site 2 (READ for indirect-feedback into a different cell): `UpdateMultibounceCells`, `gi1.comp:1948-1989`

```hlsl
void UpdateMultibounceCells(uint did) {
    uint did2 = did + g_HashGridCache_VisibilityCountBuffer0[0];
    uint cell_index      = g_HashGridCache_VisibilityCellBuffer[did2];                              // tertiary cell
    uint cell_index_base = g_HashGridCache_VisibilityCellBuffer[g_HashGridCache_VisibilityQueryBuffer[did2]];  // secondary cell

    float4 query_info = HashGridCache_UnpackBRDF(g_HashGridCache_MultibounceInfoBuffer[did]);
    float3 brdf = query_info.xyz;
    float pdf = query_info.w;

    float4 direct_radiance;
    HashGridCache_FilteredRadianceDirect(cell_index, false, direct_radiance);                       // line 1962 — READ tertiary direct
    float3 radiance = (direct_radiance.rgb / max(direct_radiance.w, 1.0f));
    radiance.rgb = (radiance.rgb * brdf) / pdf;                                                     // line 1965 — apply BRDF/pdf
    radiance.rgb /= 1.0 - g_HashGridCacheConstants.discard_multibounce_ray_probability;             // line 1966 — undo Russian roulette
    uint4 quantized_radiance = HashGridCache_QuantizeRadiance(radiance.rgb);

    if (dot(radiance, radiance) > 0.0f) {
        InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4 * cell_index_base + 0], quantized_radiance.x);
        // .y, .z similarly
    }
    InterlockedAdd(g_HashGridCache_UpdateCellValueIndirectBuffer[4 * cell_index_base + 3], quantized_radiance.w);   // sample count++
}
```

**This kernel reads the tertiary cell's direct radiance and writes it (BRDF-modulated, pdf-divided, RR-corrected) into the secondary cell's _indirect_ slot.** It is how multi-bounce GI propagates: cell-level recursive feedback rather than per-ray recursive trace.

### Site 3 (READ for glossy reflections): `TraceReflectionsHandleHit`, `gi1.comp:2865-2900`

```hlsl
// Use hash grid cache when previous frame lighting is not available
if (!previous_frame_available) {
    // jitter hit position to reduce tiled artifacts
    float2 jitter = (2.0f * payload.s - 1.0f);
    jitter       *= HashGridCache_GetCellSize(hit_position);                                        // line 2870

    HashGridCache_Data data;
    data.eye_position = g_Eye;
    data.hit_position = hit_position + jitter.x * t + jitter.y * b;                                  // jittered position
    data.direction    = ray.direction;
    data.hit_distance = hit_distance;

    uint tile_index;
    uint cell_index = HashGridCache_FindCell(data, tile_index);                                     // line 2882 — find, no insert

    if (cell_index != kGI1_InvalidId) {
        InterlockedExchange(g_HashGridCache_DecayTileBuffer[tile_index], g_FrameIndex, ...);         // bump decay
        float4 direct_radiance;
        HashGridCache_FilteredRadianceDirect(cell_index, false, direct_radiance);                   // line 2891 — READ
        float3 radiance = direct_radiance.rgb / max(direct_radiance.w, 1.0f);
#ifdef USE_MULTI_BOUNCE
        float4 indirect_radiance;
        HashGridCache_FilteredRadianceIndirect(cell_index, false, indirect_radiance);               // line 2895 — READ
        radiance += indirect_radiance.rgb / max(indirect_radiance.w, 1.0f);
#endif
        payload.radiance = radiance;
        payload.hit_distance = hit_distance;
    }
}
```

This is at the *hit of a glossy reflection ray* (a secondary vertex from the primary glossy surface). Note: the lookup uses `HashGridCache_FindCell` (read-only) — it does **not** insert. If the cell isn't present, `payload.radiance` remains zero and the trace returns no contribution.

### What the read replaces

Per the design doc's Q6 sub-question (replace-recursion / supplement / something-else):

- **Site 1 (`ResolveCells`)**: Replaces what the *secondary vertex's outgoing radiance* would be by combining (direct lighting at secondary + indirect feedback from tertiary cells). The screen probe sees this as the bounce-1 estimator. The screen probe ray itself is *always* re-traced — there is no caching of "is there geometry along this direction" at the primary site.
- **Site 2 (`UpdateMultibounceCells`)**: Replaces a tertiary trace+shade with a cache lookup, propagating bounce-2 light into the bounce-1 cell. This is the **only place a real "cache replaces trace" decision happens** in Capsaicin's main GI loop — and it's a cell-to-cell decision, not a ray-to-ray decision.
- **Site 3 (`TraceReflectionsHandleHit`)**: The secondary vertex of a glossy ray. Replaces a recursive lighting evaluation at that hit with the cache. This is "supplements recursive tracing as the lighting estimator at a single bounce" — the trace itself was performed; only the shading at the hit is cache-substituted.

### Conditional on bounce depth

There is no `if (bounce >= N) read_cache` shape in Capsaicin. Instead, the architecture is **kernel-per-stage, not loop-per-ray**: bounce 0 is the rasterizer (G-buffer / `g_VisibilityBuffer`), bounce 1 is `PopulateScreenProbes` (insert + reservoir-NEE), bounce 2 is `PopulateMultibounceCells` (insert + cell-feedback). Each kernel knows which bounce it owns; "depth" is implicit in the kernel identity.

### How this maps to our PT raygen

Our `GPUPathTracerRayGen.hlsl` runs all bounces in a single `for (bounce = 0; bounce < MAX_BOUNCES; bounce++)` loop. Capsaicin's structure does not — **kernel-per-bounce vs loop-per-bounce is a structural divergence** the rework must reconcile. The design doc anticipates this and specifies the cache read at `bounce >= 1` inside the loop. That mapping is correct in spirit (cache only at secondary+ vertices) but the implementer must decide:

1. Whether to read the cache at every secondary+ vertex (replacing the trace) or only at one specific bounce (e.g. truncating the loop at bounce-2).
2. Whether to write at every secondary+ vertex or only at the same bounce(s) read at.

Capsaicin's answer is **read-at-cell-feedback (the single cell-to-cell read in `UpdateMultibounceCells`) plus read-at-resolve (the read into the screen probe in `ResolveCells`); write at every cell touched.** The rework cannot directly transcribe this because we lack screen probes and the kernel-per-bounce decomposition.

---

## 7. Cache WRITE site (Q7)

**Status: ANSWERED — explicit in source.**

There are **two insert sites** (which establish the cell, queue visibility, and write zero) and **three update sites** (which atomic-add radiance into the cell's update buffer).

### Insert sites

| Site | File:line | Trigger | What it inserts |
|---|---|---|---|
| `PopulateScreenProbesHandleHit` | `gi1.comp:986-1076` | Hit of a screen-probe ray (= the secondary vertex from camera POV) | Cell + visibility queue entry into `VisibilityCountBuffer0`-indexed buffers (line 1013) |
| `PopulateMultibounceCellsHandleHit` | `gi1.comp:1761-1852` | Hit of a multibounce ray (= the tertiary vertex) | Cell + visibility queue entry into `VisibilityCountBuffer1`-indexed buffers (line 1788). Also writes the BRDF/pdf of the bounce-1→bounce-2 ray into `MultibounceInfoBuffer` (line 1789), so `UpdateMultibounceCells` can apply it later. |

Both sites use `HashGridCache_InsertCell` (lines 996, 1771) which calls `HashGridCache_GetDesc` to compute the bucket+tile_hash, probes `num_tiles_per_bucket=16` slots, and either claims an empty slot (`previous_hash == 0`) or finds an existing match (`previous_hash == desc.tile_hash`).

### Update sites — atomic radiance accumulation

| Site | File:line | What it writes | Target buffer |
|---|---|---|---|
| `PopulateCells` (after shadow ray) | `gi1.comp:2087-2095` | Direct lighting at secondary vertex | `UpdateCellValueBuffer[4*cell + 0..3]` (`+=` per channel + sample-count) |
| `UpdateMultibounceCells` (cell-to-cell feedback) | `gi1.comp:1971-1975` | Bounce-2 cell's direct radiance, BRDF-modulated, written into bounce-1 cell's _indirect_ slot | `UpdateCellValueIndirectBuffer[4*cell_index_base + 0..3]` |
| `GenerateReservoirs` (temporal feedback short-circuit) | `gi1.comp:2480-2495` | Previous-frame combined illumination written back into the cell when reprojection succeeds — bypasses shadow ray. | `UpdateCellValueBuffer[4*cell + 0..3]` |

### Read vs. write site relationship

Capsaicin's read sites are:
- `ResolveCells` — reads the cell whose insert was at `PopulateScreenProbesHandleHit`. **Insert and read are on the same cell, but in different kernels.** The read happens *after* `UpdateTiles` has resolved the per-frame `UpdateCellValueBuffer` deltas into the persistent `ValueBuffer`, so the read sees the prefix-frame mean (not "this-frame's writes" — those are in scratch until `UpdateTiles`).
- `UpdateMultibounceCells` — reads the *tertiary* cell, writes into the *secondary* cell. Different cells, different sites.
- `TraceReflectionsHandleHit` — read-only via `FindCell`, no matching insert in the same kernel.

There is no "writer pass" that runs separately from the integrator. **Writes happen inline at the integrator's hit kernels** (atomic-add into scratch); the only separate pass is `UpdateTiles` which moves scratch → persistent.

---

## 8. Filter / EMA pass (Q8)

**Status: ANSWERED — explicit in source.**

There is **one separate filter pass: `UpdateTiles`** (`gi1.comp:2142-2348`). It performs both the running-mean update *and* a 4-mip box-filter cascade.

### What `UpdateTiles` does

1. **Mip 0 update (lines 2161-2225)**: running-mean on every cell of every dirty tile. Reads `ValueBuffer` (prefix-frame mean × count), reads `UpdateCellValueBuffer` (this-frame delta), `lerp(prev, new, 1/N)`-merges them with `N = min(prev.w + new.w, max_sample_count)`, writes back to `ValueBuffer` and to `lds_UpdateTiles_ValueBuffer` (groupshared LDS).
2. **Mip 1, 2, 3 (lines 2227-2347)**: 2×2 box filter from the previous mip in groupshared memory. Each higher mip stores `sum_of_4_lower_cells * 4` (effectively just summed; the read-side division by `.w` accounts for the `4×` count).
3. **Scratch clear**: the per-cell `UpdateCellValueBuffer` is zeroed for the next frame (lines 2214-2223).

### Dispatch shape

`gi1.comp:2120-2123, 2142`: 8×8 thread groups (`UPDATE_TILES_GROUP_X * UPDATE_TILES_GROUP_Y = 64` threads). Each group covers `subgroup_size = 8*8/size_tile_mip0 = 8/8 = 1×1` subgroup at mip0 with `tile_cell_ratio = 8` — i.e. one tile per group in the default config.

`GenerateUpdateTilesDispatch` (`gi1.comp:2129-2140`) computes `num_groups_x = (UpdateTileCount + num_tiles_by_group - 1) / num_tiles_by_group` so only *dirty tiles* are processed (those touched this frame, tracked via `g_HashGridCache_UpdateTileBuffer`).

### Ordering relative to the integrator

The pipeline (from `gi1.cpp` dispatch order, compositionally):

1. `PurgeTiles` — evict tiles older than 50 frames.
2. `PopulateScreenProbes` — primary spawn → trace → insert at secondary, queue visibility.
3. `GenerateReservoirs` — sample lights for each visibility entry (also handles temporal feedback short-circuit).
4. `PopulateMultibounceCells` (if `USE_MULTI_BOUNCE`) — trace bounce-2 from secondary, insert at tertiary.
5. `PopulateCells` — shadow-ray each visibility, atomic-add direct radiance to scratch.
6. `UpdateMultibounceCells` (if `USE_MULTI_BOUNCE`) — read tertiary direct from cache, atomic-add into secondary indirect scratch.
7. **`UpdateTiles`** — resolve scratch → persistent, build mip cascade.
8. `ResolveCells` — read cell (mip-cascade-filtered) → write to screen probe.

So `UpdateTiles` is the explicit filter pass that performs both the EMA-style running mean *and* the box-filter mip cascade. The read sites (`ResolveCells`, `TraceReflectionsHandleHit`) only see the filtered, post-update value.

### Smoothing breakdown

- **Temporal**: running mean with cap of 16 samples → effective half-life ~11 frames at saturation. Per-cell.
- **Spatial intra-tile**: 4-mip box filter at write time; read-side selects the highest mip whose count meets `max_sample_count`. Effectively, **low-confidence cells fall back to coarser mips**, providing variable-radius spatial smoothing.
- **Spatial inter-tile (jitter)**: only at the glossy-reflections read (line 2868-2870), where the lookup position is jittered by ±0.5 cell-size. No inter-tile smoothing in `ResolveCells`.

### Match against the design doc's "phase-1.5" filter pass

The previous attempt's "phase-1.5" was a separate horizontal+vertical screen-space filter on the reprojected probe radiance. **Capsaicin's `UpdateTiles` is a different shape**: it's a per-tile (cache-space, not screen-space) update + mip cascade, run inline before the cache read. The previous attempt's filter pass was downstream of the cache read; Capsaicin's filter pass is upstream. **The rework does NOT need a separate phase-1.5 filter pass**, because the running-mean + mip-cascade in `UpdateTiles` handles all the smoothing the cache itself provides.

---

## 9. Bypass / debugging hooks (Q9)

**Status: ANSWERED — explicit in source.**

Capsaicin uses **runtime constant-buffer toggles**, not `#define`-gated bypass. The `#define`s present are for compile-time debug-overlay variants, not for "cache off / cache on at the consuming sites."

### Runtime toggles

`gi1.h:48-89` exposes `gi1_use_multibounce`, `gi1_use_direct_lighting`, `gi1_use_temporal_feedback`, `gi1_use_resampling`, etc. as `bool` `RenderOptions` — these gate features on/off via constant buffer flags read in the shaders.

There is **NO** master "disable hash-grid cache" runtime toggle. The cache is integral to the GI-1.0 pipeline; you cannot run GI-1.0 without it. The closest thing is `g_UseDirectLighting` (gates emissive and direct-light contributions; cache-write paths still run but produce zero radiance).

### Compile-time debug `#define`s

| Define | Default | Effect |
|---|---|---|
| `DEBUG_HASH_CELLS` | undefined | Adds debug cell descriptor + decay buffers (`hash_grid_cache.hlsl:124-129, 318-344`); allows visualising hash cell layout. |
| `DEBUG_HASH_STATS` | undefined | Adds bucket-overflow counter for histogram debugging (`hash_grid_cache.hlsl:270-273`). |
| `USE_MULTI_BOUNCE` | enabled when `gi1_use_multibounce=true` | Compiles in the bounce-2 indirect lobe + `MultibounceCells` kernels. |
| `USE_INLINE_RT` | enabled when DXR1.1 is available | Inline-RT vs. classic ray-gen. |

### Per-ray bypass-cache flag

There IS a per-ray bypass mechanism: the high bit of `VisibilityRayBuffer[did]` (`gi1.comp:2580, 2582, 2675`). Set when `ray_length < cell_size` (line 2561, 2674) — i.e. the secondary vertex is closer than one cell-radius to the primary, which would cause the cache to share its cell with neighboring rays in a way that produces light leaks (illustrated in the ASCII art at `gi1.comp:2538-2554`).

When the flag is set:
- `ResolveCells` returns early without reading from the cache (`gi1.comp:2368-2371`) — the screen probe doesn't get cache-resolved radiance, but `PopulateCells` already accumulated the direct lighting into the screen probe directly (`gi1.comp:2102-2106` and `gi1.comp:1977-1982` for multibounce).
- The cache is still **written** (atomic-add still fires at `gi1.comp:2091-2095`), so future-frame queries from properly-distant rays still benefit.

### Match against the design doc's `PT_HASH_GRID_CACHE_ENABLED`

The design specifies a compile-time `#define PT_HASH_GRID_CACHE_ENABLED` toggle in `GPUPathTracerRayGen.hlsl` defaulting to 0 (cache off, bit-identical to baseline). **Capsaicin has no precedent for a global on/off toggle of this shape** — it has per-ray bypass and feature-level runtime toggles, but no "compile this whole feature out" switch.

The toggle is a **rework-specific addition**, not a paper-port-aligned choice. It does not contradict Capsaicin (Capsaicin simply does not need it; GI-1.0 *is* the cache, you cannot turn it off and have GI-1.0). The toggle is justified for our engine because:
1. We need a bit-identical baseline for visual-validation regressions.
2. Our PT raygen *can* run without the cache (it currently does — that is the cache-off baseline at HEAD `10d7b158`).

So the design's `PT_HASH_GRID_CACHE_ENABLED` is a structural bypass shape with **no Capsaicin precedent** — flagged here as a rework-specific addition. The implementer should comment the `#if PT_HASH_GRID_CACHE_ENABLED` block to make clear this is a local toggle for visual A/B and is not part of the canonical Capsaicin port.

---

## Deviations the rework should consider

Three Capsaicin choices that DO NOT fit our engine's pipeline as-is. These are surfaced for the implementer + dispatcher; the auditor does not adjudicate.

### Deviation D1: Capsaicin's kernel-per-bounce vs. our loop-per-bounce

**Capsaicin**: bounce 0 is rasterizer (G-buffer); bounce 1 is `PopulateScreenProbes` (separate compute kernel); bounce 2 is `PopulateMultibounceCells` (separate compute kernel). Each kernel has its own visibility buffers (`VisibilityCountBuffer0`/`1`), reservoir buffers, and update-tile sets. The cache is read between kernels via the queue-buffer plumbing, not within a kernel.

**Our engine**: `GPUPathTracerRayGen.hlsl` runs all bounces in a single ray-gen loop. There is no kernel-to-kernel queue; the bounce loop directly recurses via `TraceRay` (or, more precisely, returns the closest-hit payload back to ray-gen, which then issues the next-bounce trace).

**Impact**: The design's "cache read at `bounce >= 1` inside the loop" maps to **Site 3 (glossy-reflections-style read)** semantically — read-only, replace-shading-at-hit, no insert-and-defer-resolve. This is the *simplest* of Capsaicin's three read sites and the only one that fits a single-kernel loop.

**The cell-to-cell feedback (Capsaicin Site 2 / `UpdateMultibounceCells`) is not directly portable** to our raygen-loop architecture. Implementing it requires either:
- Splitting the integrator into kernels (large refactor, out of scope for phase-1).
- Skipping the cell-to-cell feedback entirely and accepting that bounce-2+ contributions to the cache come from re-tracing (slower convergence; per-cell sample count grows linearly with frame count, capped at 16).
- A different scheme: read the cache at every bounce, write at every bounce, accept that the running-mean cap (16 frames) is now the convergence rate for *both* direct and indirect cell content.

**Recommendation surface**: option 3 ("read at every secondary+ vertex, write at every secondary+ vertex, use the running-mean cap as the convergence rate") is the most paper-faithful fit for our architecture. It collapses Capsaicin's two `Value*Buffer` pairs into one (no separate direct/indirect partition since the integrator is recursive), which is a simplification — but the implementer + dispatcher should adjudicate this against the visual-AC bar.

### Deviation D2: Capacity scale (~22 M cells in Capsaicin vs. design-estimated 1 M)

**Capsaicin defaults** (`gi1.h:60-62`): `2^14 = 16384` buckets × `2^4 = 16` tiles/bucket × `64+16+4+1 = 85` cells/tile (all mips) = ~22 million cells. With direct+indirect `Value*Buffer` (8 B/cell × 2) + scratch `UpdateCellValue*Buffer` (16 B/cell × 2) = ~50 B/cell × 22M = **~1 GB**. Without multibounce: ~530 MB.

**Design doc estimate**: "`2^20` cells × ~20 B = ~20 MB."

**Impact**: The design's estimate is **~25× too small for cell count and ~50× too small for bytes/cell** if the rework wants Capsaicin-equivalent capacity. For an engine targeting Sponza-class scenes (similar to Capsaicin's typical workload), the implementer must decide:
- **Match Capsaicin's defaults** → ~530 MB UAV budget for the cache. Probably too much; this engine has not been audited for this allocation scale.
- **Scale down**: e.g. `2^12 = 4096` buckets × 16 tiles × 85 cells = ~5.5 M cells, ~125 MB without multibounce. Closer to the design's intent. Risk: more bucket-overflow events at scenes with many distinct surfaces / view-distances, leading to silent drops at insert.
- **Scale way down to design's estimate** (~1 M cells, ~20 MB): risk is high collision rate and tile-saturation in real scenes.

**Recommendation surface**: ~5-10 M cells / ~100-200 MB seems a reasonable middle ground for first-cache-implementation. The implementer should run with this and observe `DEBUG_HASH_STATS`-equivalent telemetry (bucket overflows per frame) to size up or down empirically.

### Deviation D3: Key includes ray direction, not surface normal

**Capsaicin** (`hash_grid_cache.hlsl:146`): `signed_d = floor(0.5 + (0.5 * direction + 0.5) * 4.0)` — ray direction quantized to 5×5×5 = 125 buckets per spatial cell. Surface normal is NOT in the key.

**Design doc** (open question 2): "does the cache key include octahedral-packed normal" — assumed yes.

**Impact**: This is a substantive divergence from the design's mental model. The Capsaicin key uses **incoming-ray direction** (not surface normal). This has consequences:

- **Concave corners**: Two walls meeting at a corner can share a tile if the cameras-to-hit directions are close — though the cell-offset projection onto the dominant ray axis (lines 167-179) separates them when the walls' normals are along different cardinal axes.
- **Anisotropic shading**: A surface with strongly direction-dependent BRDF (specular, anisotropic) sees different cells for different incoming rays. This is *correct* for storing outgoing radiance toward the eye, since the radiance depends on view direction.
- **Diffuse case is conservative**: For a diffuse surface, the same world position viewed from different directions stores in different cells, but they all share the same "true" outgoing radiance. The running-mean is per-cell, so direction-quantization fragments the sample budget across direction buckets. Convergence is slower per direction bucket but each bucket is correct.

The design doc's normal-keyed assumption was **wrong** as a paper-port specification. The implementer should use ray-direction-keyed (Capsaicin-aligned) keys.

**Why ray-direction-keyed**: it is the right key for caching *radiance toward the camera*. Normal-keyed would be right for caching *irradiance at the surface*. GI-1.0 stores outgoing radiance; therefore direction-keyed.

---

## Direct answers to the 9 questions (summary table)

| # | Question | Answer (citation) | Status |
|---|---|---|---|
| 1 | Cell-size formula constants | `STEP_FACTOR=1e3`, `SIZE_FACTOR=1e-3`, `step = distance(eye, hit) * angular_footprint` where angular footprint comes from CPU-side `tan(fovY * user_knob * pixel_factor)` (`hash_grid_cache.hlsl:92-93,96-102,140-142`; `gi1.cpp:1856-1859`). Pow-2 quantization. | ANSWERED |
| 2 | Cache capacity / sizing | Default `2^14` buckets × 16 tiles × 85 cells = ~22M cells, ~1 GB across all buffers with multibounce (`gi1.h:60-62`; `gi1.cpp:442-461`). Resize on knob change supported. | ANSWERED |
| 3 | Sample-cap / running-mean | `lerp(prev, new, 1/min(prev.w + new.w, max_count))` with `max_count = 16` for both direct and indirect (`gi1.comp:2170,2179,2188,2197`; `gi1.h:63,65`). NOT a fixed-α EMA. | ANSWERED |
| 4 | Eviction-on-collision | Probe-budget exhaustion: silent skip (`hash_grid_cache.hlsl:268-275`). Frame-age decay: 50-frame timeout, evicted in `PurgeTiles` (`hash_grid_cache.hlsl:29`; `gi1.comp:1738-1752`). | ANSWERED |
| 5 | Cache key composition | Bucket = `pcgHash` chain over (mip-level `l`, tile-position `c.x/y/z`, ray-direction `d.x/y/z` quantized to 5/axis, near/far flag `t`); tile-hash = `xxHash` over the same inputs (`hash_grid_cache.hlsl:153-162`). **Surface normal NOT in key. Material ID NOT in key.** Cell offset within tile is computed from cell-position projected onto dominant-ray axis (`hash_grid_cache.hlsl:164-179`). | ANSWERED |
| 6 | Secondary-vertex READ site | Three sites: (a) `ResolveCells:2350-2385` reads cell at *secondary* vertex → emits to screen probe (the structural fix); (b) `UpdateMultibounceCells:1948-1989` reads cell at *tertiary* vertex → cell-to-cell feedback into secondary cell's indirect slot; (c) `TraceReflectionsHandleHit:2865-2900` reads cell at glossy-reflection hit (read-only, no insert). **Cache is NEVER read at the primary hit.** | ANSWERED |
| 7 | Cache WRITE site | Insert sites: `PopulateScreenProbesHandleHit` (secondary-vertex insert) and `PopulateMultibounceCellsHandleHit` (tertiary-vertex insert). Atomic-update sites: `PopulateCells:2087-2095` (direct light into secondary cell), `UpdateMultibounceCells:1971-1975` (tertiary's direct → secondary's indirect), `GenerateReservoirs:2480-2495` (temporal feedback). All writes are atomic-add into a scratch buffer; `UpdateTiles` resolves scratch → persistent. | ANSWERED |
| 8 | Filter / EMA pass | One pass: `UpdateTiles` (`gi1.comp:2142-2348`). Performs running-mean update at mip0 + 4-mip box-filter cascade. Dispatch: 8×8 thread groups, indirect dispatch sized by `UpdateTileCount` (only dirty tiles processed). Runs **between insert and read** per frame. | ANSWERED |
| 9 | Bypass / debugging hooks | No global cache-on/off `#define`. Per-ray bypass via high-bit of `VisibilityRayBuffer` (set when `ray_length < cell_size`). Compile-time debug `#define`s: `DEBUG_HASH_CELLS`, `DEBUG_HASH_STATS`, `USE_MULTI_BOUNCE`, `USE_INLINE_RT`. **The design's `PT_HASH_GRID_CACHE_ENABLED` toggle has no Capsaicin precedent — it is a rework-specific addition justified by our visual-AC requirements.** | ANSWERED |

---

## Not audited (out of scope)

- **`pcgHash` / `xxHash` implementations** in `math/pack.hlsl` — not fetched. The Capsaicin port chain (`pcg(a + pcg(b + pcg(c + ...)))`) shape is documented above and is what the rework must replicate; the per-call hash function body is not audit-blocking.
- **`packHalf4` / `unpackHalf4` / `packNormal` / `unpackNormal`** — not fetched (also in `math/pack.hlsl`). These are utility encoders for the radiance/visibility/direction packing; their exact bit layouts can be chosen by the rework as long as round-trip is exact.
- **Screen probe spatial reprojection / sparse directional search** (`screen_probes.hlsl`) — **out of scope per the dispatch brief** (which scopes the audit to `hash_grid_cache.hlsl` + the secondary-vertex read sites in `gi1.comp`). Screen-probe-specific design decisions do not directly constrain the PT cache rework.
- **GI denoiser / glossy-reflections atrous filter** (`gi_denoiser.hlsl`, `glossy_reflections.hlsl`) — out of scope. The PT rework currently does not have a downstream denoiser; the cache itself is the smoothing.
- **World-space ReSTIR** (`world_space_restir.hlsl`) — out of scope. Capsaicin uses ReSTIR-DI as a co-dependency for direct-light sampling at secondary vertices; our PT raygen has its own NEE and does not need ReSTIR for first-cache-implementation.
- **Cache-clear-on-scene-load** behavior — Capsaicin does not "clear-all" on scene load; it relies on `PurgeTiles`'s 50-frame decay to evict stale tiles after a scene change. The design's "clear cache UAV when scene loads" is a more aggressive choice; **flagged as a deviation** but probably correct for our engine because we already clear `AccumBuffer` on view-matrix change. (Auditor opinion withheld; recorded for the implementer's adjudication.)

---

## Sign-off

| Field | Value |
|---|---|
| Auditor | paper-auditor agent |
| Code-AI-Generated-By | Not applicable (alignment artifact is documentation, not code) |
| Sources verified by reading | `hash_grid_cache.hlsl` (495 lines, fetched in full); `gi1.comp` (4269 lines, key sections lines 950-1100, 1700-1850, 1900-2110, 2120-2400, 2600-2700, 2860-2900 read in full); `gi1.h` (key block lines 48-90); `gi1_shared.h` (key block lines 40-140); `gi1.cpp` (key block lines 442-470, 1845-1895). |
| Snapshot location | `.alignments/_audit_refs/` (auditor scratch; can be removed after the implementer reads this artifact, since all line-numbered citations point to upstream paths and this artifact is self-contained). |
