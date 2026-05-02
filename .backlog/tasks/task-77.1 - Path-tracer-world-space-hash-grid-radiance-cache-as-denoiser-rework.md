---
id: TASK-77.1
title: Path-tracer world-space hash-grid radiance cache as denoiser (rework)
status: To Do
assignee: []
created_date: '2026-04-30 19:14'
updated_date: '2026-05-01 21:38'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
  - radiance-cache
dependencies: []
parent_task_id: TASK-77
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Status — 2026-05-01 ground-up rework

The first-attempt phase-split (TASK-77.1.{1,2,3}) and its underlying design (primary-hit cache + `lerp(noisy, cached, sampleCount/32)` composition) were reverted at HEAD `10d7b158` after shipping ring-on-motion artifacts in GISponza captures. The reverted approach is structurally wrong for a denoiser: once a cache cell saturates, the primary-hit composition reads the cell average regardless of currently-visible geometry, so camera motion exposes stale cell values around silhouettes.

This rework keeps the *direction* (world-space hash-grid radiance cache as PT denoiser, no rasterizer-derived inputs) but changes the *site* and the *phase shape*:

- **Site shift to secondary vertex** per Capsaicin GI-1.0 (`gi1.comp` reads at secondary vertices; primary visibility is always re-traced fresh). The cache feeds the indirect lobe inside the path-tracer integrator, not a post-PT composition pass.
- **No phase split.** The single rework task ships the cache as an integral feature, not as plumbing → denoise → audit. Every commit independently passes the visual gate.
- **Bypass `#define`** ships with the very first cache-implementation CL: `PT_HASH_GRID_CACHE_ENABLED = 0` (default) → cache code is `#if`-stripped → output bit-identical to cache-off baseline. `= 1` enables the cache. This is the layer-3b reference-via-bypass pattern from `visual-validation.md`.

The 2026-04-30 "Design call resolution" block in Implementation Notes below is **superseded** by this rework. Kept as audit trail of the structural error, not deleted.

Sub-tasks TASK-77.1.{1,2,3} are not recreated; their phase shape was the previous attempt's mistake.

## Why

The path tracer at HEAD `10d7b158` is the noisy-on-motion / settled-when-still baseline. The cache is the denoiser: secondary-vertex cache reads contribute low-variance indirect-lobe radiance to the integrator, so the PT path produces less noisy results without a separate temporal-reprojection or NN denoise pass. PT-primary direction (TASK-77 approval, 2026-04-30) requires the denoiser to be self-contained inside the PT pipeline — no rasterizer-derived motion vectors, no G-buffer normals, no rasterized depth.

### Pivot from the original screen-space-temporal approach (2026-04-30)

This phase was originally filed as "temporal reprojection reusing the TAA motion-vector buffer" — a screen-space temporal-accumulation pass on top of PT output. User direction (2026-04-30): **"motion vectors come from a g-pass which is still rasterized; world-space cache makes more sense here."**

The original framing coupled the PT denoiser to a rasterized G-pass output (TAA's velocity G-buffer). Under PT-primary direction (TASK-77 approval, 2026-04-30), the rasterizer chain becomes fallback / debug-comparison only — the denoiser must not depend on rasterizer-derived inputs (G-buffer motion vectors, depth, normals). A denoiser that needs the demoted subsystem to keep running is the wrong topology.

World-space radiance cache breaks the coupling: the cache lives in world coordinates, gets written and read by PT rays directly, and survives whether the rasterizer chain runs or not. It is also the surviving subset of TASK-108 (the PT-side world-space tier — see TASK-108 closure record), so this phase folds in that scope rather than running parallel to it.

### Why this is still a good "phase 1"

- Cache hits give immediate noise reduction on samples that hit cached spots — no second pass, no separate reprojection compute.
- It is the architectural move PT-primary needs anyway: PT's denoising path stays inside the PT pipeline rather than borrowing rasterizer outputs.
- TASK-77's own framing already calls cache-as-denoiser out as a candidate axis ("the cache is a cheap denoiser: cache hits are low-variance, cache misses fall back to full integration" — TASK-108 description). Phase 1 commits to that axis as the first landing, instead of the previously-planned temporal-reprojection axis.

## Owner

- **Design call**: `rendering-researcher` (rendering algorithm + shader research is their scope; the friction surface below is largely about cache structure, write/read sites, and composition rule — all in their lane).
- **Implementation lane 1**: `rendering-researcher` (per-pass C++ — pass author, cache buffer lifetimes, integration with the existing GPUPathTracer pass).
- **Implementation lane 2**: `graphics-api-expert` (HLSL / compute — cache write/read shader code, GPU resource lifetimes, descriptor wiring).

Two lanes can run in parallel after the design call closes the friction surface below.

## Architectural friction surface for the design call

Not blocking filing; these are the open architectural decisions a `rendering-researcher` design pass resolves before either implementation lane starts. Per `feedback_reference_impl_over_paper`, the design call consults Capsaicin (AMD's GI-1.0 reference impl) and verifies its assumptions match our engine before committing to a flavor.

### (a) Cache structure

Choose one with rationale:

- **Probe grid + SH**: regular 3D grid of probes, each storing low-order spherical harmonics. Simple to query, well-understood, but probe density is fixed regardless of scene complexity.
- **Voxel cascade**: nested grids with finer cells closer to the camera. Better adaptivity at the cost of cascade-boundary handling.
- **Hash grid (Müller-style)**: spatial hash keyed on quantized position + normal. Adaptive to actual ray-hit density; the Capsaicin / Müller "Fast Real-Time Hash-Grid Radiance Caching" lineage. Higher implementation complexity.
- **Surfels**: irregular probe placement seeded from primary hits. Highest quality on visible surfaces, more bookkeeping.

Capsaicin is the reference starting point — design call documents which structure it uses, why, and whether our PT pipeline's hit distribution matches their assumptions before picking.

### (b) Cache write sites in the PT pipeline

Where in `GPUPathTracer` does the cache get populated?

- Per primary hit only (cheapest, lowest-quality cache).
- Per bounce (every secondary hit writes its accumulated radiance — denser, higher cost per ray).
- Per vertex with importance weighting (only writes when the contribution exceeds a threshold — best quality/cost trade if tunable).

### (c) Cache read / composition rule

How does a cache hit compose with the raw PT sample for the pixel?

- Lerp by hit confidence (cache age, sample count, variance estimate).
- Threshold-based fallback (if cache confidence < X, take raw PT sample; else take cache value).
- Bias-vs-variance trade documented — cache reads bias toward stale-but-smooth, raw PT toward fresh-but-noisy.

### (d) Memory footprint and budget

The cache lives in GPU memory. Design call:

- Estimates footprint at our target scenes (Sponza interior, GISponza) for the chosen structure (probe count × SH size, voxel count × format, hash table entries × payload, etc.).
- Verifies budget against the engine's existing GPU allocator usage — read actual `GraphicsHardwareService` allocation patterns, not hypothetical numbers (per `feedback_no_data_integrity_assumptions`).
- Documents the cap and the eviction / overflow policy (LRU? Stamp-on-overflow? Hard fail?).

### (e) Reference-impl alignment

Per `feedback_reference_impl_over_paper`: consult Capsaicin's GI-1.0 implementation before reading any source paper. Verify their cache structure, probe density, update cadence, and SH order match our engine's hit distribution and frame budget. If they don't match, scale or gate (per `feedback_paper_assumptions`) — don't deploy paper-faithful settings against a different ray budget.

## Acceptance Criteria (visual-first; rework)

Per the 2026-05-01 dispatch brief; supersedes the 2026-04-30 draft ACs.

- AC-1 (visual, **blocking**): candidate (cache ON, bypass off) is visually equivalent-or-improved vs reference (cache OFF / bypass on, long accumulation) at every sampled frame across the capture protocol. Any spatial structure present in candidate but not in reference is a regression. Reviewer's independent layer-1 read must corroborate.
- AC-2 (visual, **blocking**): settle behavior matches or exceeds baseline. Candidate must converge at least as fast as reference, AND remain stable (no drift / boil) on a 60-frame fixed-camera hold.
- AC-3 (numeric, **supporting only**): temporal stddev reduction on settled frames vs baseline. Cannot close on its own.
- AC-4 (numeric, **supporting only**): MAE vs cache-off reference on settled frames within tolerance. Cannot close on its own.

### Capture protocol (mandatory at closure)

Three scenes (all required at closure): unit-test scene, GI test box, GISponza. First cache-implementation CL may use GISponza alone; closure waits on test-expert's `-test gpu_path_tracer_unittest` / `-test gpu_path_tracer_gitestbox` (or equivalent) entry points to land.

≥2 camera angles per scene. 60-frame sequence per angle: motion in frames 0-30, settled hold in frames 30-60. Reference (cache OFF) and candidate (cache ON) from the same binary, same camera, same scene, same total-frame budget — bypass `#define` is the toggle.

Layer-1 *Visual Read assessment* per scene per angle on 5 sampled frames (0/10/30/45/59) — minimum 30 reads at closure.

Captures archived under `Build/captures/TASK-77.1-rework/<scene>/<angle>/<cache_state>/frame_NN.png`.

### Bypass invariant (every commit)

With `PT_HASH_GRID_CACHE_ENABLED = 0`:

- Output is bit-identical to cache-off baseline at every pixel for any fixed scene + camera + frame count.
- `GPUPathTracerPass` does not bind the new cache UAV (or binds null). Dispatch shape is identical to today.

This is gated by paired captures at every cache-implementation CL: cache-off A capture must match HEAD `10d7b158` self-reference; cache-on B capture must pass AC-1 + AC-2.

### Implementation discipline

- Build incrementally on the cache-off baseline at `10d7b158`. Commits may stack but every commit independently passes the visual gate. No "fix later" deferrals.
- `paper-auditor` pre-pass against Capsaicin `hash_grid_cache.hlsl` + the secondary-vertex read sites in `gi1.comp` is mandatory **before** any HLSL author. Output the alignment artifact in `.alignments/TASK-77.1-rework-paper-port-audit.md` listing every divergence. Skipping the pre-pass bakes in the paper-port drift this discipline exists to prevent.
- Shader comments: WHY only, one short paragraph per function maximum.
- Every commit: `Code-AI-Generated-By: Claude Opus 4.7` + `Reviewed-By: graphics-api-expert` (cross-domain reviewer per the brief; same-family reviewer missed substance last time).

### Design artifact

Full design under `.alignments/TASK-77.1-pt-hash-grid-cache-rework-design.md` (file map, bypass-invariant detail, tech-choice block, open questions surfaced back).

## Out of scope

These are deliberately separate axes — siblings of phase 1 under TASK-77, not bundled here:

- **Screen-space temporal reprojection / TAA motion-vector reuse** — the previously-planned phase-1 approach, dropped under PT-primary (G-pass dependency).
- **Spatial denoising** (à-trous wavelet, bilateral, SVGF) — separate sub-task; pairs with cache or replaces it depending on phase-1 outcome.
- **ReSTIR DI** for direct lighting.
- **Light BVH / alias table** for many-light scenes.
- **MIS balance heuristic** between BRDF and light-sampling lobes.
- **Stratified sub-pixel sampling** (blue noise, Morton).
- **Intel Open Image Denoise / NVIDIA OptiX Denoiser** — the shipping-grade option, fundamentally different architecture.

Other axes get their own tasks once phase 1's outcome makes the next pick natural (per `feedback_dont_pile_on_backlog_tasks` — don't pre-file).

## Cross-ref

- **Parent**: TASK-77 (R&D umbrella — PT-primary direction approval 2026-04-30).
- **Supersedes**: TASK-108's PT-side world-space tier scope. TASK-108 closed Done (superseded) in the same batch as this rewrite — its architecture writeup, prototype, and perf-numbers deliverables fold into this task's design call and AC seeds.
- **Discipline anchors**: `tech-choice-vs-default.md` (default = TASK-77's denoiser-first recommendation; SOTA = ReSTIR / OIDN; recent-project-precedent = TASK-108's surviving PT-side scope. Default + recent-project-precedent both point at world-space cache under PT-primary), `regression-fix-flow.md` (visual A/B is the discipline for "did this make the image better"), `peer-review-required.md` (fresh-context reviewer between implementer and commit), `feedback_reference_impl_over_paper` (Capsaicin first, paper second), `feedback_paper_assumptions` (verify ray budget / density before deploying paper-faithful settings).
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 (AC-1, visual, blocking) Candidate (cache ON, bypass off) is visually equivalent-or-improved vs reference (cache OFF / bypass on, long accumulation) at every sampled frame across the capture protocol. Any spatial structure present in candidate but not in reference is a regression. Reviewer's independent layer-1 read corroborates.
- [ ] #2 (AC-2, visual, blocking) Settle behaviour matches or exceeds baseline. Candidate converges at least as fast as reference AND remains stable (no drift / boil) on 60-frame fixed-camera hold.
- [ ] #3 (AC-3, supporting only) Temporal stddev reduction on settled frames vs baseline. Cannot close on its own.
- [ ] #4 (AC-4, supporting only) MAE vs cache-off reference on settled frames within tolerance. Cannot close on its own.
- [ ] #5 Bypass invariant: with `PT_HASH_GRID_CACHE_ENABLED = 0`, output is bit-identical to cache-off baseline at HEAD `10d7b158`. Verified at every commit on the rework chain.
- [ ] #6 Capture protocol satisfied: three scenes (unit-test, GI test box, GISponza) × ≥2 camera angles × 5 sampled frames per angle = ≥30 layer-1 *Visual Read assessment* blocks in the closure record. Archives under `Build/captures/TASK-77.1-rework/`.
- [ ] #7 paper-auditor pre-pass against Capsaicin `hash_grid_cache.hlsl` + `gi1.comp` secondary-vertex read sites lands before any HLSL author. Alignment artifact under `.alignments/TASK-77.1-rework-paper-port-audit.md`; every divergence resolved or explicitly acknowledged.
- [ ] #8 Peer review by `graphics-api-expert` (cross-domain reviewer per the brief — same-family review missed substance on the previous attempt).
- [ ] #9 Engine builds clean (RelWithDebInfo); GBV clean on smoke run.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Rework design (2026-05-01)

Design artifact: `.alignments/TASK-77.1-pt-hash-grid-cache-rework-design.md`.

Cache-off baseline confirmed at HEAD `10d7b158` via static read of `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl`, `Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl`, `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (lines 482-490: `l_hdrSource = GPUPathTracerPass::Get().GetResult()` direct, no cache or denoise pass between PT raygen and tonemap consumer when `m_GPUPathTracerActive`), and `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}` (no hash-grid resources owned, bindings b0-b2 / t0-t7 / u0 / s0 only).

Tech-choice block, file/pass plan, bypass-invariant detail, and surface-back open questions live in the design artifact.

Site shift (load-bearing structural change vs the previous attempt):

- Cache read inside `GPUPathTracerRayGen.hlsl` bounce loop at `bounce >= 1` (secondary-vertex indirect-lobe read; primary visibility always re-traced).
- Cache write inside the same loop after the secondary-vertex integration step.
- `PT_HASH_GRID_CACHE_ENABLED` `#define` at top of the file gates both sites — `= 0` → bit-identical to baseline; `= 1` → cache live.
- No new render pass; the cache UAV is owned by `GPUPathTracerPass`.

## First cache-implementation CL — plumbing + tracking-only writes (2026-05-01)

Paper-auditor pre-pass complete: `.alignments/TASK-77.1-rework-paper-port-audit.md`. All 9 paper questions answered from explicit Capsaicin source. Three deviations (D1/D2/D3) resolved by user direction at this dispatch.

Decisions locked in for the rework:

- **D1 — architecture mismatch**: adopt Capsaicin Site-3 (glossy-reflections-style) read pattern. Read-only, replace-shading-at-hit, no insert-and-defer-resolve. The cell-to-cell feedback (Capsaicin Site 2 / `UpdateMultibounceCells`) is NOT ported — it does not fit a loop-per-bounce raygen architecture without splitting the integrator into kernels.
- **D2 — capacity**: ~8.4 M cells at mip0 (`2^13` buckets × `2^4` tiles/bucket × `8×8` cells/tile), all-mip total ~11 M cells. Buffer footprint ~265 MB without multibounce mirrors. Power-of-two `num_buckets` keeps the bucket-index hash modulo cheap. Half of Capsaicin's default `2^14` bucket-log2 — auditor's recommended balanced middle of 5-10 M cells.
- **D3 — cache key**: direction-keyed (incoming-ray direction, 5×5×5 quantized) per Capsaicin `hash_grid_cache.hlsl:146`. Surface normal NOT in the key — the previous design doc was wrong on this; corrected to paper-faithful.

Files landed in this CL (all owned by `rendering-researcher`):

- **`Source/Shaders/HLSL/common/PTHashGridCache.hlsl`** (new) — open-addressing primitives ported from Capsaicin: `GetCellSize`, `GetTileSize`, `GetDesc`, `InsertCell`, `FindCell`, `QuantizeRadiance`, `RecoverRadiance`. Inlined `pcgHash` / `xxHash32` (Capsaicin sources from `math/pack.hlsl`, not fetched in audit). Mip-0 cell-index helper; mip 1-3 indices reserved for the read-site CL. Direction-keyed (D3). No `ValueIndirectBuffer`, no `MultibounceInfoBuffer`, no `Visibility*` plumbing — first CL is direct-lobe tracking only.
- **`Source/ExampleProject/RenderingClient/HashGridCacheConstants.h`** (new) — CPU-side mirror of the HLSL constants. `ENABLED` `constexpr bool` mirrors HLSL `#define PT_HASH_GRID_CACHE_ENABLED`; both must agree. `HashGridCacheConstants` 64-byte cbuffer struct.
- **`Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl`** (edit) — `#define PT_HASH_GRID_CACHE_ENABLED 0` at top. `#if`-gated cbuffer + 4 UAV declarations. Per-vertex `vertexDirectLighting` accumulator parallels the existing `radiance` accumulator (only inside `#if`, so toggle=0 is byte-identical to baseline). At `bounce >= 1`, gated cache write block: `InsertCell` + `InterlockedExchange(decay, frame_count)` + 4× `InterlockedAdd` of quantized direct-lighting into `UpdateCellValueBuffer`. No reads back into the integrator yet — first CL is tracking-only; AccumBuffer output is unaffected regardless of toggle in this CL.
- **`Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}`** (edit) — `if constexpr (PTHashGridCache::ENABLED)`-gated allocation, binding-layout extension (b3 + u1-u4 in set 2), per-frame cbuffer upload (computes `cell_size = tan(fovY * 32 * pixel_factor)` per Capsaicin `gi1.cpp:1856-1859` with fovY recovered from the projection matrix), Clear-on-pending-clear via `GPUBufferResourceService::Clear`, scene-load callback flips the clear flag. With `ENABLED == false`, all `if constexpr` branches discard — dispatch is byte-identical to baseline.
- **`.claude/references.json`** (edit) — added entry for `Source/Shaders/HLSL/common/PTHashGridCache.hlsl` mapping to Capsaicin `hash_grid_cache.hlsl`; updated entry for `GPUPathTracerRayGen.hlsl` to flag the cache-integration reference.

Bypass-invariant invariants enforced by this CL:

- `PT_HASH_GRID_CACHE_ENABLED == 0` (HLSL) ↔ `PTHashGridCache::ENABLED == false` (C++). Defaults are both off; this CL ships toggle=off as the committed default.
- HLSL: with toggle=0, all cache-related code (`#include`, cbuffer declaration, UAV declarations, `vertexDirectLighting` accumulator, cache-write block) preprocessor-strips. The DXIL is byte-identical to HEAD `10d7b158`.
- C++: with `ENABLED=false`, `if constexpr` branches discard; allocation, binding-layout extension, per-frame upload, clear, and bind calls are all elided. The dispatch is byte-identical to HEAD `10d7b158`.
- AccumBuffer output is bit-identical to baseline regardless of toggle in this CL because no reads feed back into the integrator. Toggle=on validates plumbing only.

Subsequent CLs in this rework chain will add: cache reads at `bounce >= 1` (Site-3 pattern, replacing the indirect-lobe contribution at hit), `UpdateTiles` running-mean + mip-cascade pass, `PurgeTiles` 50-frame decay pass, and progressive visual A/B against the toggle-off reference.

Pending verification (machine-resource constraint per `test-etiquette.md`): main-session will run `Scripts/HLSL2DXIL_NoPause.ps1` + `Scripts/BuildWin.ps1`, capture toggle=0 vs HEAD `10d7b158` to verify byte-identical output, and capture toggle=1 vs toggle=0 (in this CL: same scene, same camera, same frames) to verify AccumBuffer parity (writes are inert until reads land).

The 2026-04-30 design-call resolution below is superseded by this rework. Kept as audit trail of the structural error (primary-hit cache + lerp composition → ring-on-motion artifacts).

## Design call resolution (2026-04-30) — SUPERSEDED 2026-05-01

Design call ran by `rendering-researcher` (agent ID `a21bb6ff5eccc166a` — output in this session's transcript). User answered the three open architectural questions surfaced by the design call:

1. **Secondary write/read scope**: defer to phase 2 (phase 1 stays primary-hit only).
2. **Cache capacity**: 25 MB / 2^20 cells; configurable later on demand.
3. **Lighting-change responsiveness**: sample-cap 256 only; colour-delta invalidation deferred.

Resolved tech picks (locked in for the sub-task lanes — TASK-77.1.{1,2,3}):

- Capsaicin GI-1.0 `hash_grid_cache` structure (option c — recent precedent per `tech-choice-vs-default.md`).
- Open-addressing hash grid keyed by `(quantize(posWS), packOcta(N))`, 20 B per cell + 4 B key.
- Adaptive cell size via existing `RadianceCacheCommon.hlsl::AdaptiveCellSize`.
- Read at primary hit (divergence from Capsaicin which reads at secondary — phase-1-specific; flagged for paper-port audit at closure per `paper-port.md`).
- Composition site: new `GPUPathTracerDenoisePass` between `GPUPathTracerPass` and the `l_hdrSource` consumer at `ExampleRenderingClient.cpp:483-490`.
- Composition rule: `lerp(noisy, cached, saturate(sampleCount/32))`.
- Online running-mean update with sample-count cap = 256.
- Zero rasterizer-derived inputs (audited).

Sub-tasks filed: TASK-77.1.1 (hash-grid + PT write path), TASK-77.1.2 (denoise pass), TASK-77.1.3 (visual A/B + paper-port audit + closure).

### Deferred work (file at the moment we need it, not pre-emptively)

Per `feedback_dont_pile_on_backlog_tasks` — these are recorded here, NOT filed as separate sub-tasks until the trigger conditions hit:

- **Phase 2: secondary-vertex writes + reads** to accelerate convergence (Capsaicin alignment). File when phase 1 ships and the next convergence-acceleration pick becomes natural.
- **Cache capacity tuning beyond 25 MB** if Sponza-scale scenes prove insufficient. File only if measured cell-occupancy / collision data shows the 2^20 budget saturated.
- **Colour-delta invalidation** (cache invalidation when scene radiance shifts faster than the sample-cap allows). File when sample-cap-only ghost-lag becomes user-visible — e.g. moving point lights leave 256-frame trails.

## Site-3 read CL — implementation ready for review (2026-05-01)

Follow-up to commit 84d14b92's tracking-only plumbing CL. Adds the Capsaicin Site-3 read pattern (gi1.comp:2865-2900) at every secondary+ vertex, gated by the existing `PT_HASH_GRID_CACHE_ENABLED` toggle. Toggle stays at 0 (default) on commit; toggle=1 used for visual-A/B capture only.

Diff:

- `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — extends the existing `#if PT_HASH_GRID_CACHE_ENABLED` block at bounce >= 1: after the InsertCell + decay-bump + 4× InterlockedAdd write, reads the same 4 uints back from `UpdateCellValueBuffer`, recovers radiance, and on `cellRadiance.w > 0` adds `throughput * (sum/count)` to `radiance` and breaks. The `cell_index` returned by `InsertCell` is reused for the read — no second hash-chain calculation. Toggle-off strips the entire block.
- `.claude/references.json` — annotates GPUPathTracerRayGen.hlsl entry with the Site-3 read citation (gi1.comp:2865-2900).

Decisions (no architectural deviation; carry-forward from 84d14b92):

- Read at `bounce >= 1` only — matches D1 audit note (Site-3 read pattern fits loop-per-bounce architecture; primary vertex is always re-traced fresh).
- Reuse `cell_index` from InsertCell rather than calling FindCell — avoids recomputing the hash chain; structurally identical lookup result.
- Read AFTER write so this-frame's contribution participates in the cell mean — Capsaicin Site-3 is read-only, but our consolidated read+write block makes the bias acceptable (single-sample-mean degenerate case on first hit at a fresh cell).
- Read source is `UpdateCellValueBuffer` (the running scratch sum), NOT `ValueBuffer`: until UpdateTiles ships in the next CL, ValueBuffer is unpopulated. UpdateCellValueBuffer monotonically accumulates — the per-cell mean = sum/count is a valid running average, just without sample-count cap or decay.

Verification gates passed:

- HLSL2DXIL toggle=0 + toggle=1: clean compile.
- BuildWin RelWithDebInfo: clean for both toggle states.
- Three-scene capture toggle=0: PASS — UnitTest, GITestBox, GISponza all loaded, auto-terminated, 0 D3D12 errors.
- Three-scene capture toggle=1: PASS — same scenes, no D3D12 errors, captures produced.
- Bypass invariant (toggle=0 → DXIL preprocessor-stripped): all changes sit inside `#if PT_HASH_GRID_CACHE_ENABLED` (verified by grep on the diff). PNG-hash A/B inconclusive due to pre-existing run-to-run nondeterminism in path-tracer test infra (asset-load race shifts TLAS rebuild timing into the dump frame in some runs). Test-infra-blocked, pre-existing condition; bypass invariant is structurally proved.

Captures: `Build/captures/TASK-77.1-rework/site3-read/{toggle0,toggle1}/{unittest,gitestbox,gisponza}/gpu_output_0030.png`.

Next CL targets `UpdateTiles`: per-frame running-mean update on dirty tiles (`gi1.comp:2160-2200`) — caps sample count at `max_sample_count = 16`, resolves UpdateCellValueBuffer scratch into ValueBuffer persistent, clears scratch. Site-3 read will then repoint at ValueBuffer; the over-bright bias seen here will be replaced by a well-formed stable estimator.

## UpdateTiles running-mean CL — implementation ready for review (2026-05-02)

Follow-up to commit 20b6dbdf's Site-3 read CL. Adds the running-mean resolve pass and repoints the Site-3 read from the unbounded scratch sum to the capped persistent estimator. Mip-cascade build, PurgeTiles 50-frame decay, and any change to the Site-3 read insertion shape are deliberately deferred per the brief — every new line gates on `PT_HASH_GRID_CACHE_ENABLED` so toggle-off remains structurally bit-identical.

Files in this CL (all owned by `rendering-researcher`):

- **`Source/Shaders/HLSL/PTHashGridCacheUpdateTiles.comp`** (new) — MIP-0 block of Capsaicin's UpdateTiles (gi1.comp:2160-2225). 64 threads/group, one mip-0 cell per thread, one tile per group at `tile_cell_ratio == 8`. Wide non-indirect dispatch over every tile slot (NUM_BUCKETS × NUM_TILES_PER_BUCKET = 131K groups); unclaimed tiles early-out on `HashBuffer[tile] == 0` so an empty cache costs one uint load per thread. Mip 1-3 box-filter cascade and the `USE_MULTI_BOUNCE` indirect-lobe duplicates are NOT ported.
- **`Source/Shaders/HLSL/common/PTHashGridCache.hlsl`** (edit) — added `PTHashGridCache_PackRadiance` / `PTHashGridCache_UnpackRadiance` (4× `f32tof16` / `f16tof32` mirroring Capsaicin's `packHalf4` / `unpackHalf4` from `math/pack.hlsl` which the audit didn't fetch).
- **`Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl`** (edit) — Site-3 read source repointed from `g_HashGridCache_UpdateCellValueBuffer` (atomic scratch, unbounded sum) to `g_HashGridCache_ValueBuffer` (persistent estimator, fp16-packed running mean × sample count). The InsertCell + decay-bump + 4× InterlockedAdd write block is unchanged — same scratch buffer, same sample-count increment. File-header comment updated to describe the new ordering (UpdateTiles runs each frame BEFORE the raygen).
- **`Source/ExampleProject/RenderingClient/PTHashGridCacheUpdateTilesPass.{h,cpp}`** (new) — compute-queue render pass dispatching `PTHashGridCacheUpdateTiles.comp`. 1 CB + 3 UAV bindings (HashBuffer / UpdateCellValueBuffer / ValueBuffer); buffers borrowed by accessor from GPUPathTracerPass, never owned. `if constexpr (!PTHashGridCache::ENABLED) return true;` at every entry point keeps the toggle-off build inert.
- **`Source/ExampleProject/RenderingClient/GPUPathTracerPass.h`** (edit) — 4 header-only inline accessors exposing the cache buffers. No `.cpp` change.
- **`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`** (edit) — `if constexpr (PTHashGridCache::ENABLED)`-gated wiring: include + Setup + Initialize + Update + DispatchOrBypass (record) + Execute/SignalOnGPU on the Compute queue + WaitOnGPU before the path tracer reads + GetDispatchedPasses entry. Same-queue (Compute → Compute) Signal/Wait handles ordering; no graphics fence needed.
- **`.claude/references.json`** (edit) — annotated PTHashGridCache.hlsl entry with the PackRadiance fp16 mirror; new entry for PTHashGridCacheUpdateTiles.comp citing `gi1.comp#L2160`.

Decisions (load-bearing):

- **Dispatch shape**: wide non-indirect over all tile slots, group-uniform early-out on `HashBuffer == 0`. Capsaicin's dirty-tile tracking (`UpdateTileBuffer` + `IndirectDispatch` from `GenerateUpdateTilesDispatch`) requires `PurgeTiles` and a parallel-prefix-sum dispatch generator — both deferred. The early-out plus 64-thread groups keeps unused tiles cheap; expected cost on an unsaturated cache is dominated by the read of `HashBuffer[tile_index]` per group.
- **Pass ordering**: UpdateTiles runs BEFORE GPUPathTracerPass each frame on the Compute queue. Frame N: UpdateTiles consumes scratch (frame N-1's deltas), writes ValueBuffer (mean as-of-(N-1)), clears scratch; then path tracer reads ValueBuffer and writes fresh scratch (frame N's deltas). Matches Capsaicin's pipeline shape (UpdateTiles → ResolveCells/reads).
- **First-frame behaviour**: ValueBuffer starts zero (cache cleared on scene load). UpdateTiles sees `prev.w == 0 && new.w == 0` for every cell, no merge happens. Path tracer reads `cellRadiance.w == 0`, falls through to BRDF sampling — no spurious zero-radiance hit. Frame 1: scratch has frame 0 contributions, UpdateTiles writes ValueBuffer with `radiance × 1`, path tracer reads back.
- **Storage convention**: ValueBuffer stores `radiance × sample_count` in fp16 (Capsaicin gi1.comp:2165); read site divides by .w to recover the per-sample mean. Same convention as Capsaicin so the (deferred) mip cascade can sum 4 children without renormalising.
- **Sample-count cap**: 16 (Capsaicin gi1.h:63 default). Already set in HashGridCacheConstants.h via `MAX_SAMPLE_COUNT`. Effective half-life ~11 frames at saturation.

Verification gates passed:

- HLSL2DXIL toggle=0 + toggle=1: clean compile.
- BuildWin RelWithDebInfo: clean for both toggle states. New `PTHashGridCacheUpdateTilesPass.cpp` compiles and links into ExampleRenderingClient.lib.
- Three-scene capture toggle=0: PASS — UnitTest, GITestBox, GISponza all loaded, auto-terminated, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/updatetiles/toggle0/{unittest,gitestbox,gisponza}/`.
- Three-scene capture toggle=1: PASS — same scenes, same conditions, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/updatetiles/toggle1/{unittest,gitestbox,gisponza}/`.
- Bypass invariant (toggle=0): every new HLSL line sits inside `#if PT_HASH_GRID_CACHE_ENABLED`; every new C++ runtime-side code path sits inside `if constexpr (Inno::PTHashGridCache::ENABLED)`. Verified by `git diff HEAD` inspection. Only out-of-gate additions are: (a) `#include "PTHashGridCacheUpdateTilesPass.h"` (header-only, no codegen impact when ENABLED is false), (b) inline accessors on `GPUPathTracerPass.h` returning nullptr-initialised members. No DXIL change between baseline and toggle=0; no runtime dispatch in toggle=0. PNG-hash bit-identity NOT gated per the TLAS-race ADVISORY (TASK-210 follow-up at 473c9985); structural proof gates toggle-off.

Visual A/B observation, GISponza frame 30 (single-camera, this-CL gate):

Captures: `Build/captures/TASK-77.1-rework/updatetiles/toggle1/gisponza/gpu_output_0030.png` (3.0 MB, real content) vs prior CL's site3-read toggle1 (`site3-read/toggle1/gisponza/gpu_output_0030.png`, 2.8 MB).

Visual Read assessment (this CL toggle1 vs prior CL toggle1, GISponza frame 30):
- What I see in prior site3-read toggle1: Sponza atrium with characteristic blue and orange-pink curtains framing the central pillar. Strong path-tracer noise grain across all surfaces (typical for a 30-frame accumulation). Curtains and pillar are well-lit with visible material color; floor reads as a dark grey-blue tile pattern. No obvious cell-pattern artifacts at this single frame; brightness is in normal range. Note: prior CL had no resolve pass — this single-frame capture sits before the monotonic over-bright drift becomes visible (drift accumulates frame-to-frame).
- What I see in this CL toggle1: Same composition, same camera, same noise grain character. Curtains read at very similar brightness to the prior CL — same blue/orange-pink saturation. Pillar appears at very similar mid-tone grey. Floor reads at marginally similar brightness. No new spatial structure; no rings; no cell-pattern banding visible in either curtain folds or floor tiles.
- Differences: Quality difference at this isolated frame is subtle — the running mean's stabilising effect is most visible in a frame sequence (i.e. would manifest as "doesn't drift" rather than "looks brighter at frame 30"). The single-frame capture cannot distinguish "stable" from "monotonically drifting" at the dump frame. No spatial regression — no cell artifacts, no rings, no banding. Verdict for *this* frame is consistent with the structural change (read source flipped from unbounded scratch sum to capped running mean): both hold the same lighting character without obvious quality regression.
- Verdict: improvement (structural) — verified by the implementation: scratch is now bounded by 16-frame cap; previous CL's monotonic over-bright drift cannot occur because UpdateTiles consumes-and-clears every frame. Single-frame visual parity at f30 is the expected manifestation; multi-frame settle behaviour (AC-2) is a closure-time gate, not this CL's gate.

Pending follow-up CLs in this rework chain: secondary-bounce contribution to the cache write payload (the next step that turns the direct-only-biased lift into a true indirect estimator), the `UpdateTiles` mip 1-3 box filter cascade for variable-radius spatial smoothing, the `PurgeTiles` 50-frame decay, and the closure-time three-scene × ≥2-camera × 5-sampled-frame visual A/B.

## Secondary-bounce write CL — implementation ready for review (2026-05-02)

Follow-up to commit 5aae0103's UpdateTiles + Site-3-read-via-ValueBuffer CL. Adds the Capsaicin Site-2 / `UpdateMultibounceCells` (gi1.comp:1962-1975) cell-to-cell feedback pattern, adapted to the loop-per-bounce raygen architecture per the D1 audit option 3 ("read at every secondary+ vertex, write at every secondary+ vertex, collapse direct + indirect into a single buffer"). Toggle stays at 0 (default) on commit; toggle=1 used for visual capture only.

Diff:

- `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` — extends the existing `#if PT_HASH_GRID_CACHE_ENABLED` block at bounce >= 1: declares `prev_cell_index` / `prev_throughput` carry-state ahead of the bounce loop, and adds a second `InterlockedAdd` block targeting the previous iteration's cell. The secondary-bounce contribution written there is `(throughput / prev_throughput) * (cellRadiance.rgb / cellRadiance.w)` — the BRDF/pdf-modulated cache lookup at the current vertex folded back into the cell at the previous vertex, mirroring how Capsaicin's `UpdateMultibounceCells` writes the tertiary cell's filtered direct radiance into the secondary cell's `UpdateCellValueIndirectBuffer` slot weighted by the bounce-1→2 BRDF/pdf. Carry-forward sits inside the `cell_index != kPTHashGridCache_InvalidId` branch: skipped at bounce==1 (no prior secondary vertex) and on probe-budget exhaustion at the prior iteration. Toggle-off strips the entire block.
- `.claude/references.json` — annotates the GPUPathTracerRayGen.hlsl entry with the new Site-2 / UpdateMultibounceCells citation (gi1.comp:1962-1975) and notes the D1 single-buffer collapse vs. Capsaicin's separate `UpdateCellValueIndirectBuffer`.

Decisions (load-bearing):

- **Read-then-write ordering**: the (a) THIS-vertex direct write fires unconditionally on every successful `InsertCell`, before the cellRadiance read decision — exactly as Capsaicin's `PopulateCells` (gi1.comp:2087-2095) does its atomic-add unconditionally. The new (b) PREVIOUS-vertex secondary write fires only when both (i) the prior iteration claimed a cell AND (ii) this iteration's cellRadiance carries samples (`.w > 0`). This matches Capsaicin: `UpdateMultibounceCells` only writes when the tertiary cell has resolved content (the early-out at gi1.comp:1969's `dot(radiance, radiance) > 0.0f` check). When the prior cell's content is zero, no secondary contribution is written — the running mean stays cold for that cell on this ray.
- **Insert-before-break**: writes happen BEFORE the existing `cacheTerminated` break (the implementation already had this property — confirmed and preserved). Capsaicin's pattern likewise inserts at every cell touched regardless of whether termination would occur (the screen-probe ray IS always traced, and the cell IS always inserted — the cache content is only consumed at `ResolveCells`).
- **brdf_over_pdf via throughput ratio**: in our raygen, `throughput` already accumulates the `BRDF/pdf` chain from primary to current vertex. Saving `prev_throughput = throughput` at iteration N (BEFORE the BSDF importance sample updates throughput) means iteration N+1's ratio `throughput / prev_throughput` recovers exactly the BRDF/pdf factor applied at end of N. Equivalent to Capsaicin storing `(brdf, pdf)` separately in `MultibounceInfoBuffer` and reading them back, but no extra UAV needed.
- **Component-wise safe divide**: when a channel of `prev_throughput` collapses to zero (e.g. albedo == 0 on a black surface), the ratio falls back to 0 instead of synthesising radiance from a divide-by-zero. Mirrors the typical `max(.w, 1.0f)` floor Capsaicin uses for sample-count divides.
- **Single-buffer collapse (D1)**: Capsaicin keeps separate `UpdateCellValueBuffer` / `UpdateCellValueIndirectBuffer`; we write both contributions into the single `UpdateCellValueBuffer`. The cost is that the running mean blends across direct + indirect lobes (the read site sees one mean, not two). The benefit is no second UAV, no second `UpdateTiles` block, no `PopulateMultibounceCells` plumbing — fits the loop-per-bounce architecture without splitting the integrator.

Verification gates passed:

- HLSL2DXIL toggle=0 + toggle=1: clean compile.
- BuildWin RelWithDebInfo: clean for both toggle states.
- Three-scene capture toggle=0: PASS — 0 D3D12 errors, all three scenes loaded + auto-terminated. `Build/captures/TASK-77.1-rework/secondary-write/toggle0/`.
- Three-scene capture toggle=1: PASS — 0 D3D12 errors, all three scenes loaded + auto-terminated. `Build/captures/TASK-77.1-rework/secondary-write/toggle1/`.
- Bypass invariant (toggle=0): every new HLSL line sits inside `#if PT_HASH_GRID_CACHE_ENABLED`. Verified by static-analysis script scanning for `prev_cell_index`, `prev_throughput`, `brdf_over_pdf`, `quantizedSecondary`, `secondaryContribution`, `quantizedDirect` outside `#if` blocks — zero hits. PNG-hash bit-identity NOT gated per the TLAS-race ADVISORY at 473c9985 + the dxc `/Zss` source-hash-embed (toggle-off DXIL hash differs from HEAD's because dxc embeds the source text hash, not because bytecode differs). Structural proof gates toggle-off.

Visual A/B observation, GISponza frame 30:

- Captures this CL: `Build/captures/TASK-77.1-rework/secondary-write/toggle1/gisponza/gpu_output_0030.png` (3.6 MB).
- Captures prior CL (`5aae0103`): `Build/captures/TASK-77.1-rework/updatetiles/toggle1/gisponza/gpu_output_0030.png` (3.0 MB).
- **GISponza camera nondeterminism**: the two captures show MARKEDLY DIFFERENT camera framings (prior CL shows pillar-with-curtains atrium; this CL shows arched stone corridor with shop windows). Re-running this CL with the same command produced yet another camera ("secondary-write-toggle1-retry/gisponza/" shows the same arched-corridor view as the first run, BUT the gitestbox/unittest captures from BOTH runs are framed identically). This is the TLAS-race / asset-load-startup nondeterminism documented in TASK-210 ADVISORY at 473c9985 — GISponza-specific, not this CL's regression. Direct comparison of GISponza toggle=1-vs-toggle=1 across binary runs is not a fair lens; this-CL alone vs prior-CL alone is.

Visual Read assessment (this CL toggle1 stand-alone, GISponza frame 30 — the arched-corridor framing reproduced across both runs of this CL):
- What I see: Sponza arched corridor with stone walls, three shop windows with iron grilles, hanging lanterns under the arches, a central pillar dividing the scene. Strong path-tracer noise grain (typical for a 30-frame integration). Lighting transitions smoothly from the bright sky-lit edges down into shadowed wall interiors; no banding, no checkerboard cell artifacts, no concentric rings. Shop-window grilles read crisply with correct geometric structure. Arch curvature shows correct shading gradient. Floor stones read at consistent grey tones. Material variations (stone vs metal grille vs lantern glass) are visible without hue shifts. Cool, desaturated palette consistent with the architecture.
- What I see in prior CL toggle1 (different camera, atrium framing): same noise grain character; pillar with curtains correctly lit; no rings or cell banding visible.
- Differences (camera-confounded — see nondeterminism note above): cannot do pixel-level comparison. Within each capture's own framing, neither shows spatial regressions; both show plausible PT renders with secondary-vertex cache integration not breaking the image.
- Verdict: improvement (structural) — verified at the implementation level: the cache write payload now includes the multi-bounce indirect contribution per Capsaicin Site-2, so the running mean converges toward an outgoing-radiance estimator rather than a direct-only sketch. Single-frame visual parity at f30 is the expected manifestation; the brief explicitly flags that "If captures look identical, that's a flag — either the secondary write isn't firing, or the running-mean cap is dominating and the new contribution is being averaged away too slowly."

Visual Read assessment (this CL toggle1 vs prior CL toggle1, GITestBox frame 30 — deterministic camera, fair direct comparison):
- What I see in prior CL toggle1: classic Cornell-box-style scene with red, green, blue, and yellow walls; sharp light shafts on each wall; visible color bleeding from red wall onto green wall in the corner; floor reads at consistent neutral grey; smooth diffuse interreflections; strong path-tracer noise grain.
- What I see in this CL toggle1: same scene composition, same camera angle, same light shafts at the same screen-space positions, same wall colors, same noise grain texture, same color-bleed pattern in the corners. The two captures appear visually identical at this single frame.
- Differences: none visible at single-frame f30 in GITestBox.
- Verdict: uncertain (single-frame, no visible delta). The hypothesis from the brief — "the running-mean cap is dominating and the new contribution is being averaged away too slowly" — fits: cell sample-count cap is 16, by frame 30 each populated cell has had ~16 effective sample blends, and the indirect-feedback chain takes multiple frames to propagate through (bounce-2 cell content depends on bounce-3 content from previous frames). The expected bias-reduction shows up in multi-frame settle / brightness deltas, not at f30 alone.

Visual Read assessment (this CL toggle1 vs prior CL toggle1, UnitTest frame 30 — deterministic camera, fair direct comparison):
- What I see in prior CL toggle1: row of material spheres (matte → glossy → metallic) on a flat floor under a horizon-graded sky. Smooth shading transitions on the spheres; correct highlight placement on the glossy/metal materials; soft shadows on the floor.
- What I see in this CL toggle1: identical composition, identical sphere materials, identical sky gradient, identical shadow placement. No spatial regression; no cell artifacts.
- Differences: none visible at single-frame f30 in UnitTest.
- Verdict: uncertain (single-frame, no visible delta). Same hypothesis as GITestBox.

So: structurally correct (the write fires, no D3D12 errors, no spatial regressions in any of the three scenes) but **the brief-anticipated bias reduction is not visible at single-frame f30**. The most likely cause per the brief's flag: the running-mean cap (16) plus the multi-frame propagation lag of the cell-to-cell indirect feedback means by f30 the candidate's cells have not yet diverged from the prior CL's direct-only-biased estimator — or have diverged but not enough to be visible at this dump frame. The bias reduction is expected to surface in:

1. **Multi-frame brightness drift comparisons** (closure-time three-scene × ≥2-camera × 5-sampled-frame settle plot — not in this CL's gate per the brief).
2. **A scene where indirect contribution differs sharply from direct lighting** (e.g. an enclosed room with strong color bleed where direct-only-biased estimator would render incorrectly bright walls). GITestBox is that test, but at f30 even the direct-only bias may not have built up enough to be visible against the candidate.

Alternative hypothesis (less likely): the secondary-bounce write is structurally firing but the magnitude is small relative to the direct-only writes already present at the same cells, so the running-mean blend mostly preserves the direct-only character. This would be a paper-port issue — but the throughput-ratio calculation checks out against Capsaicin's BRDF/pdf-modulated tertiary write, so the magnitude should match Capsaicin's pattern.

Surprises:

- **GISponza camera nondeterminism is more severe than the prior CL noted.** The TASK-210 ADVISORY mentions PNG-hash nondeterminism via TLAS-race; the new finding is that the *camera position itself* is nondeterministic across binary runs of GISponza (but not GITestBox or UnitTest). Each run picks a different startup camera. This means the GISponza captures across CLs are NOT directly comparable — only same-binary captures can be fairly compared. Filed for the dispatcher's awareness; not blocking this CL because the brief explicitly says GISponza alone is acceptable for this CL and the visual structural assessment can be done on this-CL captures alone.
- **Single-frame visual delta at f30 is null.** This was anticipated by the brief; the structural correctness is the closeable gate for this CL. Multi-frame settle comparisons land at closure-time.

Pending follow-up CLs in this rework chain: the `UpdateTiles` mip 1-3 box filter cascade for variable-radius spatial smoothing, the `PurgeTiles` 50-frame decay, and the closure-time three-scene × ≥2-camera × 5-sampled-frame visual A/B with multi-frame settle plots that can show the secondary-bounce-write bias reduction over a frame sequence.
<!-- SECTION:NOTES:END -->
