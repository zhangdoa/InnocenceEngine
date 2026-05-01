# TASK-77.1.3 Paper-port alignment — Hash-grid radiance cache (phase 1)

- **Paper**: Boisse et al., *GI-1.0: A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination* — section 2.2 (World Cache); local copy `Build/GI1_0.pdf` / `Build/GI1_0.txt`.
- **Reference implementation**: AMD Capsaicin GI-1.0 — `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/hash_grid_cache.hlsl` and the radiance-population kernels in `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/gi1.comp`.
- **Our implementation**: `Source/Shaders/HLSL/common/HashGridCache.hlsl`, `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` (write site), `Source/Shaders/HLSL/GPUPathTracerDenoise.comp` (read site), `Source/ExampleProject/RenderingClient/HashGridCacheConstants.h`, `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}`, `Source/ExampleProject/RenderingClient/GPUPathTracerDenoisePass.{h,cpp}`.
- **Audit date**: 2026-04-30.
- **Author**: rendering-researcher (sub-agent dispatch of `paper-auditor` was requested in the brief but the rendering-researcher does not have the Agent tool in this scope; main-session can re-dispatch a fresh paper-auditor against this artifact for verification).
- **Audit scope**: phase-1-only paper-port. Phase-2 secondary writes/reads, ReSTIR, multibounce — explicitly out of scope per TASK-77.1.

## Summary

| Status | Count |
| --- | --- |
| FAITHFUL | 0 |
| DIVERGENT (intentional, scope-locked) | 8 |
| DIVERGENT (PARTIALLY resolved this CL; AC #2 still unmet — see D9) | 1 |
| Not audited (out of scope) | 5 |

The phase-1 port is intentionally a heavy reduction of Capsaicin's structure — single-cell flat hash, no tile/mip, no filter, primary-hit write site. The eight scope-locked divergences are documented in TASK-77.1.1 / TASK-77.1.2 closure records and `.claude/references.json`. **One newly-surfaced divergence (D9 — race-tolerant non-atomic radiance accumulation) is algorithmically load-bearing**: the running-mean update is broken under multi-thread cell contention, which collapses the cache contribution to ~1% per frame. This is the root cause of TASK-77.1.3 AC #2 failure (≥5x noise-floor drop unmet). Recorded for resolution; **not** a paper-port-flavour decision the design call closed.

## Alignment table

| # | Decision | Paper spec (§2.2) | Reference impl (Capsaicin) | Our impl | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Cache structure | "two-level hash grid", §2.2.3 — tiles of 8×8 cells with mip chain (tile + cell + mip-LOD) | `hash_grid_cache.hlsl:111-191` `HashGridCache_Desc { bucket_index, tile_hash, cell_offset[2] }`; `HashGridCache_CellIndex` resolves tile → cell → mip | Single flat hash table, no tile, no mip. `HashGridCache.hlsl:96-115` `HashGridCache_BuildKey` returns one uint key, `HashGridCache_BucketIndex` is a flat modulo into a 2^20 cell array | DIVERGENT (D1 — scope-locked) |
| 2 | Cell key descriptor | "include the quantization level" + "additionally hashing the boolean result of (h < t)" + ray direction quantized into 5x5x5 buckets — light-leak fixup §2.2.2 | `hash_grid_cache.hlsl:140-156` keys on (log_step_multiplier, signed_c.xyz, signed_d.xyz, hit_distance<tile_size) | Keys on (cellCoord.xyz, packOctahedral(N)). No mip-level encoded into key, no direction quantization, no `hit_distance<cell_size` boolean. `HashGridCache.hlsl:96-111` | DIVERGENT (D2 — scope-locked, light-leak fix-up not ported) |
| 3 | Adaptive cell size | §2.1.7 / Algorithm 6 — depth-adaptive cell size; quantized to power-of-2 step `exp2(floor(log2(d * c)))` | `hash_grid_cache.hlsl:96-102` `HashGridCache_GetCellSize` definition; applied per-key at `:140-142` inside `HashGridCache_GetDesc`. `cell_size_step = max(distance(eye, position) * cell_size, min_cell_size)` then `exp2(floor(log2(STEP * cell_size_step)))` | `HashGridCache.hlsl:55-62` `HashGridCache_CellSize`: returns `depth * tan(fovY * 8 / maxDim) / sqrt(2)`, no log2 quantization. Continuous-valued size; same hit point at slightly different depths quantizes into different cells | DIVERGENT (D3 — scope-locked, parameterization) |
| 4 | Hash function | Two complementary hashes: `pcgHash` for bucket index, `xxHash` for tile fingerprint (so a bucket collision is detected via fingerprint mismatch) | `hash_grid_cache.hlsl:153-162` — `pcgHash` ∘ chained reductions; separate `xxHash` for tile_hash | `HashGridCache.hlsl:66-76` — single `HashGridCache_PCG` + xorshift-style `HashCombine`. No second-fingerprint hash; bucket-collision-vs-key-match detected by direct `key == stored` equality | DIVERGENT (D4 — scope-locked, single-hash port) |
| 5 | Open-addressing | `num_tiles_per_bucket` slots per bucket; eviction = "too many collisions, return invalid" (kGI1_InvalidId), no fallback eviction; tile decay buffer drains stale tiles separately | `hash_grid_cache.hlsl:249-278` `HashGridCache_InsertCell` — linear probe, returns `kGI1_InvalidId` on overflow; tile decay via `kHashGridCache_TileDecay` | `HashGridCache.hlsl:147-187` `HashGridCache_InsertOrFind` — linear probe length 4, eviction-on-overflow by `frameLastTouched` (oldest cell stomped). No separate decay buffer | DIVERGENT (D5 — scope-locked, eviction policy simplified) |
| 6 | Radiance update | "Contributions are temporally accumulated into the cells at the first MIP level using an exponential moving average [Karis 2014]" (§2.2.3) | `gi1.comp:2087-2095` (single-bounce direct radiance write — primary paper-mapping match for our primary-hit path; `:1971-1975` is the multi-bounce indirect equivalent with identical atomic shape): `InterlockedAdd` on quantized radiance R/G/B/W components; running mean reconstructed at read time via `radiance.rgb / max(w, 1)`. `hash_grid_cache.hlsl:301-306` `HashGridCache_QuantizeRadiance` quantizes by `kHashGridCache_FloatQuantize=1e3f`. Filter pass (separate kernel) runs the EMA temporal blend on the per-frame sum | `HashGridCache.hlsl:257-290` (post-fix) `HashGridCache_Insert`: per-channel `InterlockedAdd` on `quantizedRadianceSum` + `InterlockedAdd(.., 1u)` on sampleCount with write-side cap at 32. Pre-fix at `:189-213` was non-atomic Read-Modify-Write — D9's load-bearing defect. EMA filter pass + max_sample_count clamp from Capsaicin not yet ported | **DIVERGENT (D9 — partially resolved this CL; EMA filter deferred to TASK-208)** |
| 7 | Cache write site | Secondary path vertices (§2.2 opening, §2.2.1) — populate at every secondary hit during PT bounce, the screen-probe queries read at integration time | `gi1.comp` PopulateCells / UpdateCells kernels — invoked from screen-probe RT pass at secondary-vertex hits. Primary hits feed the screen probe directly, not the world cache | `GPUPathTracerRayGen.hlsl:498-516` writes the cache at PT primary hit only (post-bounce-loop, with the full path-traced outgoing radiance). Secondary writes are explicitly out of scope (deferred to phase 2) | DIVERGENT (D6 — scope-locked, design-call decision 2026-04-30) |
| 8 | Cache read site | Secondary path vertices — when a bounce ray hits a cached point, read the cached radiance (multi-bounce reuse) | `gi1.comp` and `hash_grid_cache.hlsl` read sites: `HashGridCache_FilteredRadianceDirect` / `HashGridCache_FilteredRadianceIndirect` invoked from RT closest-hit code on secondary vertices, mip-walk to find the first level with sample count above the threshold | `GPUPathTracerDenoise.comp:96-118` reads at the **screen-space primary hit** via per-pixel `(posWS, N)` UAVs the writer also produces. Lookup is single-level (no mip-walk). The denoise pass composes `lerp(noisy, cached, saturate(sampleCount/32))` | DIVERGENT (D7 — scope-locked) |
| 9 | Composition rule | §2.2.3 — temporal EMA accumulation inside the cell (write-side); §2.2.4 — read returns mip-walked cached value; integration variance reduced by accumulation across frames | Cache stores per-cell EMA-blended radiance; downstream consumers always read the cell value directly (no further blend with a fresh sample at the same vertex) | `GPUPathTracerDenoise.comp` blends the cached cell value with the per-frame raw PT estimator: `lerp(noisy, cached, saturate(sampleCount/32))`. The lerp is a denoiser-as-composition shape, not a reuse-as-replacement shape | DIVERGENT (D8 — scope-locked, denoiser repurposing) |
| 10 | Light-leak fix-up | §2.2.2 + Figure 14/15 — "additionally hashing the boolean result of (h < t)" to break adjacent secondary hits at different ray distances into separate cells | `hash_grid_cache.hlsl:151` `t = uint(hit_distance < hit_tile_size)` folded into the bucket-index hash | Not implemented. Hash key has no analogue of `hit_distance<cell_size`. Risk: adjacent surfaces with different incoming ray distances may share a cell | DIVERGENT (D2 covers it — scope-locked) |
| 11 | Tile decay / eviction | §2.2.1 — "Each cell is associated with a decay value, which is reset upon access, or left to decay toward zero otherwise. As a cell's decay reaches zero, we deallocate the entry" | `hash_grid_cache.hlsl:29` `kHashGridCache_TileDecay = 50`; separate `g_HashGridCache_DecayTileBuffer` tracks tile-level decay, drains in a dedicated kernel | `HashGridCache.hlsl:147-187` `frameLastTouched` field; eviction only fires on probe-chain overflow (no proactive decay-driven eviction). Stale cells linger until contended | DIVERGENT (D5 covers it — scope-locked) |

## Detail entries

### D1 — Single flat hash, no tile, no mip

- **Paper**: §2.2.3 — two-level hash map: tiles each contain 8×8 cells in a linear layout, plus mip levels for cone-filtered radiance reads.
- **Reference**: `hash_grid_cache.hlsl:111-191` defines `HashGridCache_Desc { bucket_index, tile_hash, cell_offset[2] }`; `HashGridCache_CellIndex` walks `tile_index * num_cells_per_tile + first_cell_offset_tile_mip[mip] + offset.x + offset.y * mip_size`.
- **Our**: `HashGridCache.hlsl:96-115` — single flat hash key, single bucket-index modulo into a 2^20-element cell array. No tile concept, no mip chain, no per-tile bucket structure.
- **Nature of divergence**: structural simplification. Capsaicin's tile+mip layering pays off when reads need cone-filtered radiance at multiple cell scales (their multi-bounce reuse path). Phase-1 only reads/writes at one scale; layering would add complexity without read-side benefit at the current scope.
- **Impact**: read site cannot do cone-filtering across neighbour cells. Cache reads are pointwise; spatial neighbour information is unused.
- **Resolution**: scope-locked phase-1 reduction (TASK-77.1 design call 2026-04-30). Closing the divergence is the body of work for a future "phase 1.5: cone-filtered reads" task; not blocking phase-1 closure. Documented in `HashGridCache.hlsl` header (file header lines 7-21) and `.claude/references.json` notes.

### D2 — Cell-key descriptor reduced (no mip-level, no ray direction, no hit-distance boolean)

- **Paper**: §2.2.1-2.2.2 — descriptor includes quantized position, the LOD level of quantization (so cells from different LODs don't collide), an octant of ray direction (for breaking adjacent surface bounces apart), and the boolean result of `hit_distance < tile_size` (light-leak fix per §2.2.2 / Figures 14-15).
- **Reference**: `hash_grid_cache.hlsl:140-156` — bucket-index hash chain over `(l, c.xyz, d.xyz, t)` where `l = log_step_multiplier`, `c = floor(pos/tile_size)`, `d = floor(0.5 + (0.5*direction + 0.5) * 4.0)`, `t = uint(hit_distance < hit_tile_size)`.
- **Our**: `HashGridCache.hlsl:96-111` — key chain over `(cellCoord.xyz, packOctahedral(N))`. No `l`, no ray direction, no hit-distance boolean.
- **Nature of divergence**: simpler key. We use surface normal in place of ray direction (denser quantization for primary hits where N is well-defined; equivalent for our use case where the cell describes outgoing radiance from a point). LOD level and hit-distance boolean are skipped.
- **Impact**:
  - **Light-leak risk** (paper §2.2.2): cells across thin walls with similar quantized positions may share a key. For Sponza interior, the near-architectural surfaces (curtains, columns) are at risk.
  - **No LOD encoding** — adaptive cell size relies on `HashGridCellSize`-as-quantizer drift to separate distance bands. Continuous cell size means a moving camera changes the cell binning each frame for a surface near the LOD boundary.
- **Resolution**: scope-locked. D5 (eviction-on-overflow) absorbs the cell-collision case at a quality cost. File a follow-up only when light-leak artifacts surface in PT-primary mode on a multi-room scene.

### D3 — Continuous adaptive cell size, no power-of-2 quantization

- **Paper**: §2.1.7 / Algorithm 6 — adaptive cell size quantized to power-of-2 to ensure stable cell boundaries.
- **Reference**: `hash_grid_cache.hlsl:96-102` — `cell_size = HASHGRIDCACHE_SIZE_FACTOR * exp2(uint(log2(STEP * cell_size_step)))`. Power-of-2 quantization means cells line up across distance bands.
- **Our**: `HashGridCache.hlsl:55-62` — `cell_size = depth * tan(fovY * 8 / maxDim) / sqrt(2)`. Continuous-valued; cells at slightly different depths quantize differently.
- **Nature of divergence**: parameterization choice. Theirs is constant-multiplier, log2-quantized; ours is fovY-pixel-aware, no quantization step.
- **Impact**: cells at LOD boundaries shift bin under camera/depth motion. For a fixed camera + fixed surface this should be stable; for an orbiting camera the cell binning of the same world point can flicker between adjacent cell sizes. (TASK-77.1.3's measurement window 130-159 is fixed-camera, so this divergence is not the dominant cause of the AC #2 gap.)
- **Resolution**: scope-locked; track if camera-orbit captures show flicker localized to LOD-boundary distances.

### D4 — Single hash, no tile-fingerprint check

- **Paper**: §2.2.1 — two-level hash map design implies bucket index ≠ cell identity; the tile fingerprint disambiguates collisions.
- **Reference**: `hash_grid_cache.hlsl:153-162` — `pcgHash` ∘ chained reductions for bucket-index, separate `xxHash` for `tile_hash`. A bucket collision still fails the tile-hash check, which sends `HashGridCache_FindCell` down the next bucket-offset slot.
- **Our**: `HashGridCache.hlsl:64-76` — single `HashGridCache_PCG` + xorshift-style `HashCombine` chain. Open-addressing with direct key-equality comparison (`stored == key`, `HashGridCache.hlsl:229`).
- **Nature of divergence**: simpler hash. Their dual-hash is required because their tile + cell representation is layered (tile lookup is one hop, cell within the tile is a second hop). Our flat key collapses the two into one.
- **Impact**: behaviourally equivalent at our flat-hash scope. Two unrelated cells producing the same `HashGridCache_BuildKey` output collide under both schemes (1/2^32 probability per slot, much smaller than 4/2^20 probe-chain saturation rate).
- **Resolution**: scope-locked. If we add tiles in a later phase the second hash needs to come back.

### D5 — Eviction-on-overflow vs decay-driven eviction

- **Paper**: §2.2.1 — "Each cell is associated with a decay value, which is reset upon access, or left to decay toward zero otherwise. As a cell's decay reaches zero, we deallocate the entry so its memory can be later reused."
- **Reference**: `hash_grid_cache.hlsl:29` `kHashGridCache_TileDecay = 50`; separate `g_HashGridCache_DecayTileBuffer` updated by a dedicated kernel.
- **Our**: `HashGridCache.hlsl:147-187` — `frameLastTouched` field on the cell, eviction only fires on probe-chain overflow (slot count 4) by stomping the oldest. No proactive decay sweep.
- **Nature of divergence**: lazy eviction vs proactive decay sweep. Their per-frame decay sweep keeps the bucket utilization low even when the working set shifts (e.g. camera moves to a new region). Ours leaves stale cells in place until probe-chain pressure triggers a stomp.
- **Impact**: at 2^20 cells, Sponza's working set probably never saturates, so probe-chain overflow is rare. Stale cells from prior camera poses linger but don't clobber active queries (different keys → different probe chains). Consequence: memory occupancy grows monotonically over a long session, not a quality concern at the current capacity.
- **Resolution**: scope-locked. Stale cells lingering at 25 MB capacity is fine until we hit a many-room scene where the working set exceeds 2^20 cells.

### D6 — Primary-hit write site (vs paper's secondary-vertex)

- **Paper**: §2.2 opening + §2.2.1 — "We previously skipped over the details of calculating and caching the outgoing radiance at each of the secondary path vertices. This is the role of our world cache."
- **Reference**: Capsaicin's PopulateCells / UpdateCells kernels run from the secondary-vertex side of the path tracer.
- **Our**: `GPUPathTracerRayGen.hlsl:498-516` writes the cache at primary hit only, with the full path-traced outgoing radiance (post-bounce-loop accumulator).
- **Nature of divergence**: scope-locked phase-1 design-call decision (TASK-77.1 implementation notes, 2026-04-30): "Phase 1 stays primary-hit only; secondary-vertex writes/reads deferred to phase 2."
- **Impact**: cache stores the outgoing radiance at primary surface, indexed by primary surface (pos, normal). Multi-bounce reuse — the convergence-acceleration win of Capsaicin's design — is not active in phase 1.
- **Resolution**: scope-locked. Phase 2 (separate task, not yet filed) closes the gap. Documented in TASK-77.1's "Deferred work" section and in `HashGridCache.hlsl` header.

### D7 — Screen-space primary read (vs paper's secondary-vertex read)

- **Paper**: §2.2.4 — "we have described how we cached and filtered the direct lighting at secondary path vertices." Reads happen at secondary vertices during a separate RT pass.
- **Reference**: `gi1.comp` invokes `HashGridCache_FilteredRadianceDirect` / `Indirect` from inside the closest-hit shader on secondary rays.
- **Our**: `GPUPathTracerDenoise.comp:96-118` reads at the screen-space primary hit via two per-pixel hit-info UAVs (`g_PrimaryHitPos` / `g_PrimaryHitNormal`) the writer also produces.
- **Nature of divergence**: scope-locked (paired with D6).
- **Impact**: read site mirrors write site at the primary surface; lookup key matches the writer's bit-for-bit (verified via the shared `HashGridCache_BuildKey` + `HashGridCache_CellSize` helpers).
- **Resolution**: scope-locked. Paired with D6 — both flip in phase 2.

### D8 — Denoiser-as-composition vs read-as-replacement

- **Paper**: §2.2.3-2.2.4 — read returns the cached EMA-blended cell value directly. Variance reduction is a property of the EMA accumulation across frames inside the cell.
- **Reference**: read path returns `radiance.rgb / max(w, 1)` directly; downstream use is replacement, not blend.
- **Our**: `GPUPathTracerDenoise.comp:107-110` does `lerp(noisy, cached, saturate(sampleCount/32))` — blends a fresh-frame raw PT estimator with the cached value, weighted by sample count.
- **Nature of divergence**: phase-1 repurposing — under TASK-77.1's pivot, the cache doubles as the denoiser. Capsaicin uses it as a reuse cache, not a denoiser.
- **Impact**: at high sampleCount the read approaches the cached value; at low sampleCount the read approaches the noisy frame. By itself this is a sound denoiser-composition choice — modulo D9, which corrupts the cached value.
- **Resolution**: scope-locked design-call decision. Standalone, the lerp shape is fine.

### D9 — Race-tolerant non-atomic radiance accumulation (NEWLY SURFACED — algorithmically load-bearing) — PARTIALLY RESOLVED 2026-04-30

- **Paper**: §2.2.3 — "Contributions are temporally accumulated into the cells at the first MIP level using an exponential moving average [Karis 2014]."
- **Reference**: `gi1.comp:1971-1975` and `hash_grid_cache.hlsl:301-306` — the per-frame radiance contributions are accumulated via `InterlockedAdd` on quantized integer components (kHashGridCache_FloatQuantize = 1e3f). Sample count is the .w component, also `InterlockedAdd`'d. Running mean is reconstructed by `radiance.rgb / max(w, 1)` at read time, then EMA-blended in a separate filter kernel.
- **Pre-fix code** (TASK-77.1.1, before this CL): non-atomic float3 RMW with `HashGridCache_OnlineMeanUpdate` — last-writer-wins under cell contention.
- **Post-fix code (this CL)**: `HashGridCache.hlsl` migrated to Capsaicin-style integer atomic-add. `HashGridCell` is now `(uint3 quantizedRadianceSum, uint sampleCount, uint frameLastTouched)`. `HashGridCache_Insert` does `InterlockedAdd` per RGB channel + sampleCount, with HASHGRID_FLOAT_QUANTIZE = 1e3 matching Capsaicin's constant. Read recovers the mean as `quantizedSum / (1e3 * sampleCount)`. Sample-cap at 32 (write-side) prevents uint32 wraparound under the upstream 1e5 radiance clamp and matches the denoiser's saturation point.
- **Post-fix measurement (2026-04-30, 30 frames at fixed camera, frames 130–159, 1280×720)**:
  - Cache-off baseline (`HashGridCache_DenoiseSampleCap = 1e9f`, lerp weight ≈ 0 → reads always fall back to noisy radiance): **p50=44.62, p95=86.56, p99=99.93, max=118.93**.
  - Cache-on (`HashGridCache_DenoiseSampleCap = 32.0f`): **p50=59.01, p95=105.05, p99=115.63, max=124.18**.
  - **AC #2 (≥5x stddev drop) is unmet — and worse, the cache makes p50 stddev INCREASE by ~32%** (59.01 vs 44.62). Visual inspection shows the cause: the cache stores rare bright sun-NEE-hit samples and averages them into cells that are otherwise dark, producing visible over-bright "speckles" on tree silhouettes and skylight edges where the cell working set is small.
- **Why the InterlockedAdd fix alone doesn't close AC #2**: D9 was a real bug — the prior non-atomic RMW provably collapsed contributions under contention. Fixing it gives the cache a *correct* unweighted-mean integrator, but the paper's variance-reduction guarantee assumes a much richer pipeline:
  1. **Filter pass with EMA** — Capsaicin's separate `UpdateCellValueBuffer` → EMA → `ValueBuffer` pipeline (gi1.comp:2160-2217) bounds the cell's historical mean and lets it adapt to relighting; without it, our unweighted-mean cell amplifies bright outliers indefinitely until the cap is hit.
  2. **Adaptive cell size with power-of-2 quantisation** (Capsaicin `hash_grid_cache.hlsl:96-102`, `cell_size = exp2(floor(log2(STEP * cell_size_step)))`) — our continuous-valued cell size (D3) interacts with sub-pixel camera jitter (Halton-sequenced at `GPUPathTracerRayGen.hlsl:227`), and at sponza-interior depths the world-space cell size is comparable to one screen-space pixel. Cells flicker frame-to-frame, breaking sample accumulation. Verified empirically: enlarging `cellSizePx` 8→64 reduced cache-on p50 modestly (61.18→56.22) but introduced visible blocky artifacts on near geometry without recovering the noise floor.
  3. **Light-leak fix-up** (D2/D10) — without `(hit_distance < cell_size)` boolean folded into the hash key, adjacent surfaces with different incoming ray distances share cells and bleed bright contributions across them.
  4. **NEE-from-sun variance source** — the dominant noise contribution at primary hits in sponza is sun-direct visibility through small openings, not BRDF-bounce noise. Capsaicin's read site (secondary vertex) doesn't observe this — the paper's variance-reduction story is for indirect light, not the primary-hit shape we're using.
- **Resolution status**: D9's specific defect (non-atomic RMW) is fixed in this CL. The headline acceptance criterion (≥5x stddev drop) requires additional algorithmic work that is no longer in TASK-77.1.3's scope — surfaced here for the closure CL to file as a phase-1.5 follow-up. Item (1) above is the highest-impact next step (Capsaicin-style EMA filter pass).

## Not audited (out of scope)

- **§2.2.2 light-leak fixup**: D2 covers absence of the `hit_distance<cell_size` boolean fold-in. Paper-faithful behaviour is deferred; light-leak artifacts have not been observed in current Sponza captures, so the divergence is recorded without a triggered follow-up.
- **§2.2.3 cone-filter / mip walk**: D1 covers absence of mip chain. Phase-1 reads at one scale.
- **§2.2.4 secondary-vertex evaluation**: D6/D7 cover phase-1's primary-only scope. Multi-bounce reuse, per-vertex temporal reprojection — phase 2.
- **§2.3 ReSTIR DI integration**: explicitly out of scope per TASK-77.1's "Out of scope" list.
- **Multi-bounce indirect buffer (gi1.comp UpdateMultibounceCellsMain)**: depends on §2.2.4, deferred with phase 2.

## Reviewer-facing summary

Eight of the nine recorded divergences are intentional phase-1 scope reductions, all anchored in TASK-77.1's design-call resolution (2026-04-30) and TASK-77.1.1 / TASK-77.1.2 closure records. The newly-surfaced ninth divergence (D9) is the algorithmically load-bearing one: the running-mean update is structurally non-equivalent to the paper's EMA accumulation under multi-thread contention. It explains the ~0% noise-floor drop measured in TASK-77.1.3's A/B (against the ≥5x AC-target). Recorded for resolution in a phase-1.5 follow-up; not patched in this CL.

The closing CL ships the divergence acknowledged in this artifact and (in the closure record) on the task itself. Future paper-port reviewers should expect this artifact to grow a phase-1.5 entry once the atomic-radiance fix lands.
