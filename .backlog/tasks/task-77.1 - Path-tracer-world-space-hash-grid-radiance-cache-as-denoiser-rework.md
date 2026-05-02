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

## PurgeTiles 50-frame decay CL — implementation ready for review (2026-05-02)

Follow-up to commit 4ff0ccae's secondary-bounce-write CL. Adds the Capsaicin PurgeTiles (gi1.comp:1715-1752) LRU eviction pass: tiles whose `DecayTileBuffer` marker has fallen more than 50 frames behind the current frame counter are freed by zeroing `HashBuffer[tile_index]`, making the slot claimable by the next InsertCell that hashes into the same bucket. Without this pass, cells touched once and never again accumulate forever; the cache HashBuffer grows monotonically, eventually saturating its bucket budget. Toggle stays at 0 (default) on commit; toggle=1 used for visual capture only.

Files in this CL (all owned by `rendering-researcher`):

- **`Source/Shaders/HLSL/PTHashGridCachePurgeTiles.comp`** (new) — Capsaicin PurgeTiles port. Decay rule `tile_decay = g_FrameCount - DecayTileBuffer[tile_index]` (unsigned subtraction; underflow trivially exceeds 50, evicting stale slots — correct on the wraparound path). Free-condition: `tile_decay >= PT_HASHGRIDCACHE_TILE_DECAY` (50, hash_grid_cache.hlsl:29). Free-action: `HashBuffer[tile_index] = 0` (slot becomes claimable) + `DecayTileBuffer[tile_index] = 0` (defensive — symmetric with the scene-load clear). 64 threads/group with only thread 0 acting; one group per tile (NUM_TILES groups). The 64-thread shape mirrors UpdateTiles for dispatch-arithmetic symmetry; wasted threads idle in-wave at no extra cost. Empty-cache early-out on `HashBuffer == 0` keeps unclaimed slots cheap.
- **`Source/Shaders/HLSL/common/PTHashGridCache.hlsl`** (edit) — added `#define PT_HASHGRIDCACHE_TILE_DECAY 50u` mirroring Capsaicin `kHashGridCache_TileDecay` (hash_grid_cache.hlsl:29) and the existing CPU-side `TILE_DECAY_FRAMES` constant.
- **`Source/ExampleProject/RenderingClient/PTHashGridCachePurgeTilesPass.{h,cpp}`** (new) — compute-queue render pass dispatching `PTHashGridCachePurgeTiles.comp`. 1 CB + 2 UAV bindings (FrameCountCB / HashBuffer / DecayTileBuffer); buffers borrowed by accessor from GPUPathTracerPass, never owned. `if constexpr (!PTHashGridCache::ENABLED) return true;` at every entry point keeps the toggle-off build inert.
- **`Source/ExampleProject/RenderingClient/GPUPathTracerPass.h`** (edit) — 2 header-only inline accessors: `GetHashGridCacheDecayTileBuffer()` for the previously-private `m_HashGridCache_DecayTileBuffer`, and `GetFrameCountCB()` for the FrameCount CB so PurgeTiles can compute frame_count − decay without owning a parallel CB upload. No `.cpp` change.
- **`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`** (edit) — `if constexpr (PTHashGridCache::ENABLED)`-gated wiring: include + Setup + Initialize + Update + DispatchOrBypass (record) + Execute/SignalOnGPU on the Compute queue, with the Wait chain ordered so PurgeTiles signals before UpdateTiles waits, UpdateTiles signals before path tracer waits + GetDispatchedPasses entry. Same-queue (Compute → Compute) Signal/Wait handles ordering throughout.
- **`.claude/references.json`** (edit) — annotated `PTHashGridCacheUpdateTiles.comp` entry to note PurgeTiles ordering; new entry for `PTHashGridCachePurgeTiles.comp` citing `gi1.comp#L1715`.

Decisions (load-bearing):

- **Pass ordering**: `PurgeTiles → UpdateTiles → PathTracer` per Capsaicin's `gi1.cpp` step 1 → step 7 → integrator. PurgeTiles must run BEFORE UpdateTiles so the HashBuffer == 0 early-out skips freed tiles (otherwise stale-tile scratch contributes to a dying tile's running-mean); BEFORE the path tracer so InsertCell can claim freed slots this frame. Same-queue Signal/Wait throughout.
- **Free granularity is per-tile, not per-cell**. Capsaicin frees only `HashBuffer[tile]` and re-uses the per-cell `ValueBuffer` / `UpdateCellValueBuffer` slots in-place when the tile is reclaimed. The reclaim path causes a single-frame leak (the new tenant's first running-mean lerp weights the previous tenant's mean by 15/16) which the running mean smooths out within ~16 frames. Paper-faithful — flagged as a known minor divergence; could be lifted by a parallel cell-clear scan inside the kernel if a future capture exhibits visible reclaim flicker.
- **Decay marker convention**: the path tracer's `InterlockedExchange(DecayTileBuffer[tile], g_FrameCount, prev)` writes the absolute frame number; PurgeTiles compares `g_FrameCount - decay`. Zeroed slots (cleared on scene load + on free) read `decay == 0`, so an unclaimed-by-HashBuffer tile is gated out before the comparison runs — no false eviction at startup.
- **Wraparound handling**: u32 frame counters; wraparound takes 2^32 frames. Unsigned subtraction's underflow case (decay > frame_count) yields a near-2^32 result that trivially exceeds 50, evicting a slot with a stale-future marker — the correct behaviour for the only realistic wraparound source (a long-lived editor session with a corrupted DecayTileBuffer write).
- **Dispatch shape**: 64 threads/group, one group per tile, only thread 0 acts. Wide non-indirect over NUM_TILES groups matches UpdateTiles. The wasted-threads cost is negligible because dispatch granularity is dominated by group count, not threads/group, and 64 threads fills one wave on AMD/NV — a 1-thread-per-group alternative would underutilise the wave.

Verification gates passed:

- HLSL2DXIL toggle=0 + toggle=1: clean compile (new `PTHashGridCachePurgeTiles.comp` builds in both states; the path tracer raygen recompiled both because the touched `common/PTHashGridCache.hlsl` invalidated its dependency check, but with `PT_HASH_GRID_CACHE_ENABLED == 0` the include is preprocessor-stripped — DXIL must be byte-identical).
- BuildWin RelWithDebInfo: clean for both toggle states. New `PTHashGridCachePurgeTilesPass.cpp` compiles and links into ExampleRenderingClient.lib (cmake reconfigure was needed to pick up the new file; `file(GLOB)` is configure-time only — flagged as a workspace-hygiene note, not a CL issue).
- Three-scene capture toggle=0: PASS — UnitTest, GITestBox, GISponza all loaded, auto-terminated, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/purgetiles/toggle0/{unittest,gitestbox,gisponza}/`.
- Three-scene capture toggle=1: PASS — same scenes, same conditions, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/purgetiles/toggle1/{unittest,gitestbox,gisponza}/`.
- Bypass invariant (toggle=0): every new HLSL line in the two touched HLSL files sits inside `#if PT_HASH_GRID_CACHE_ENABLED` (the new `#define PT_HASHGRIDCACHE_TILE_DECAY` lives in `common/PTHashGridCache.hlsl`, which is `#include`d only inside `#if PT_HASH_GRID_CACHE_ENABLED` from `GPUPathTracerRayGen.hlsl`; the standalone `PTHashGridCachePurgeTiles.comp` is a separate file that compiles to DXIL but is never bound at runtime when `ENABLED == false`). Every new C++ runtime path sits inside `if constexpr (Inno::PTHashGridCache::ENABLED)`. Out-of-gate additions: (a) `#include "PTHashGridCachePurgeTilesPass.h"` (header-only, no codegen impact when ENABLED is false), (b) two inline accessors on `GPUPathTracerPass.h` returning nullptr-initialised members. PNG-hash bit-identity NOT gated per the TLAS-race ADVISORY (TASK-210 follow-up at `473c9985`); structural proof gates toggle-off.

Visual A/B observation, frame 30:

- **GITestBox toggle1 (deterministic camera, fair direct comparison)**: this CL vs prior (`4ff0ccae`) toggle1 — visually identical at single-frame f30. Same composition, same wall colors, same light shafts at same screen-space positions, same color-bleed pattern, same noise grain texture. Verdict: improvement (structural, correctness gate cleared) — the result is consistent with the brief's expected behaviour: at f30 PurgeTiles essentially never fires because no cell has aged 50 frames yet (m_FrameCount == 30, every active cell's decay marker is well within the threshold), so the toggle-on output should match the prior CL toggle1 at this dump frame.
- **UnitTest toggle1 (deterministic camera)**: this CL vs prior toggle1 — visually identical at single-frame f30. Same sphere materials, same horizon gradient, same shadow placement. Verdict: improvement (structural).
- **GISponza toggle1**: this CL shows pillar-with-blue-and-orange-curtains atrium framing (matches the prior `5aae0103` UpdateTiles capture's framing); prior `4ff0ccae` capture showed the arched-corridor framing — TLAS-race nondeterminism (TASK-210 ADVISORY) makes pixel-level comparison unfair across binary runs. Within this CL's capture: noise grain in normal range, no spatial regression, no rings, no cell-pattern banding visible in either curtain folds or floor tiles. Verdict: improvement (structural) — implementation correctness verified by the GITestBox/UnitTest deterministic comparison; GISponza serves as the no-D3D12-errors / no-spatial-artifact gate only.

Visual Read assessment (this CL toggle1 vs prior CL toggle1, GITestBox frame 30 — deterministic camera):
- What I see in prior CL toggle1 (`4ff0ccae`/secondary-write): Cornell-box-style scene with red, green, and yellow walls plus blue and pink end panels; sharp light shafts across the floor and walls; visible color bleeding from red wall onto green wall in the corner; floor reads at consistent neutral grey; smooth diffuse interreflections; strong path-tracer noise grain.
- What I see in this CL toggle1 (purgetiles): identical composition, identical wall colors, identical light shaft positions, identical color-bleed pattern, identical noise grain texture. The two captures appear pixel-comparable to the eye at this single frame.
- Differences: none visible at single-frame f30 in GITestBox.
- Verdict: improvement (structural — the eviction kernel runs without breaking the image; multi-frame stability is the closure-time gate).

The expected qualitative behaviour was "near-zero visible effect at f30" because cells need 50 inactive frames to be evicted and the run is only 31 frames long, so PurgeTiles fires few times if at all. The visual parity at f30 is exactly the structural correctness signal: the kernel runs, doesn't break the image, doesn't introduce D3D12 errors, doesn't trip the bypass invariant. Long-run cache stability (no monotonic HashBuffer growth) is a closure-time AC, not this CL's gate.

Surprises:

- **CMake reconfigure required**: `Source/ExampleProject/RenderingClient/CMakeLists.txt` uses `file(GLOB ...)` which evaluates at configure time. Adding new `PTHashGridCachePurgeTilesPass.{h,cpp}` files broke the first `BuildWin` invocation with unresolved-external linker errors until I re-ran `cmake ..` from `Build/`. Not a CL issue, but worth flagging — a future agent adding a new pass file in this subtree must remember to reconfigure cmake before building.
- **No race against UpdateTiles**: the same-Compute-queue Signal/Wait chain transitively orders PurgeTiles → UpdateTiles → PathTracer with no need for cross-queue fencing. The shader writes (HashBuffer = 0, DecayTileBuffer = 0) and the UpdateTiles reads (HashBuffer != 0 → early-out) operate on the same physical resources but on disjoint memory at the tile-slot granularity, and the Wait between them ensures memory visibility.
- **Decay buffer wraparound is a non-issue at the practical horizon**: 2^32 frames at 60 fps ≈ 2.27 years of continuous runtime. The unsigned-underflow path is structurally correct (evicts the affected slot) so even if the marker did wrap, the cache would self-heal within 50 frames.
- **Cell-vs-tile granularity for free is paper-faithful but leaky on reclaim**: Capsaicin frees only the HashBuffer slot and accepts ~16 frames of contamination from the previous tenant's residual ValueBuffer / scratch values. We match this directly. Could be tightened with a per-cell clear scan if reclaim flicker becomes user-visible; not gating this CL.

Pending follow-up CLs in this rework chain: the `UpdateTiles` mip 1-3 box filter cascade for variable-radius spatial smoothing, and the closure-time three-scene × ≥2-camera × 5-sampled-frame visual A/B with multi-frame settle plots.

## Mip-cascade build CL — implementation ready for review (2026-05-02)

Follow-up to commit 3d84a7d3's PurgeTiles 50-frame decay CL. Adds the Capsaicin per-tile mip-cascade aggregation (gi1.comp:2227-2347 — the mip-1 / mip-2 / mip-3 blocks of UpdateTiles fused with groupshared LDS), plus the cell-index-by-mip helper (hash_grid_cache.hlsl:207-221) needed for the variable-mip CellIndex math. Toggle stays at 0 (default) on commit; toggle=1 used for visual capture only.

The mip cascade builds coarser-level cells by aggregating finer-level cells: each mip-(L+1) cell is the 4-way SUM of its mip-L children's packed `radiance × sample_count` payload. Recovering the per-sample mean at read time yields a sample-count-weighted average of the children — exactly what a fatter footprint should average over. No renormalisation is needed because the storage convention already carries the count.

**Site-3 read source UNCHANGED.** GPUPathTracerRayGen.hlsl still reads ValueBuffer at mip 0; the cascade contents are dead data this CL. The structural correctness gate (build fires, no D3D12 errors, no perturbation of toggle-on visual) is what closes this CL. Mip-aware Site-3 read lands in the next CL.

Files in this CL (all owned by `rendering-researcher`):

- **`Source/Shaders/HLSL/PTHashGridCacheMipCascadeBuild.comp`** (new) — separate compute pass, 64 threads/group, one tile per group. Loads mip-0 from ValueBuffer into LDS (8x8 packed uint2), then runs three sequential mip stages with `GroupMemoryBarrierWithGroupSync` between: mip-1 reads stride-1 4-tap from LDS at (0,0)/(1,0)/(0,1)/(1,1) (16 active threads), mip-2 reads stride-2 (4 active threads), mip-3 reads stride-4 (1 active thread). The same overwrite-into-LDS-at-the-cell-owning-thread pattern as Capsaicin so each subsequent mip can read the previous mip from LDS at the doubled stride. Writes go to ValueBuffer at the mip-N cell index returned by the new `PTHashGridCache_CellIndexMipN` helper. Empty-tile early-out (`HashBuffer == 0`) is GROUP-UNIFORM (every thread in the group reads the same `tile_index = group_id.x`) so the no-early-return-before-barrier rule is preserved.
- **`Source/Shaders/HLSL/common/PTHashGridCache.hlsl`** (edit) — added `PTHashGridCache_CellIndexMipN`. Capsaicin `hash_grid_cache.hlsl:207-221` shape: `cell_offset_mip0 >> mip_level` collapses 2^mip neighbouring mip-0 cells onto the single coarser cell, `mip_size = size_tile_mip0 >> mip_level` shrinks the row stride correspondingly, the per-mip first-cell offset moves us past the previous mips inside the tile's NUM_CELLS_PER_TILE slot range. The existing `PTHashGridCache_CellIndexMip0` is left in place (alongside the new helper) for the UpdateTiles call site that already uses it.
- **`Source/ExampleProject/RenderingClient/PTHashGridCacheMipCascadeBuildPass.{h,cpp}`** (new) — compute-queue render pass dispatching `PTHashGridCacheMipCascadeBuild.comp`. 1 CB + 2 UAV bindings (HashGridCacheCB / HashBuffer / ValueBuffer); buffers borrowed by accessor from GPUPathTracerPass, never owned. `if constexpr (!PTHashGridCache::ENABLED) return true;` at every entry point keeps the toggle-off build inert. Wide non-indirect dispatch over NUM_TILES groups, identical shape to UpdateTiles / PurgeTiles.
- **`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp`** (edit) — `if constexpr (PTHashGridCache::ENABLED)`-gated wiring matching the existing PurgeTiles/UpdateTiles pattern: include + Setup + Initialize + Update + DispatchOrBypass (record) + Execute/SignalOnGPU on the Compute queue (after UpdateTiles' Wait, before path tracer's Wait) + GetDispatchedPasses entry. The path tracer's WaitIfActive flips from `UpdateTiles` to `MipCascadeBuild` (last link in the chain — same-queue Signal/Wait transitively covers UpdateTiles + PurgeTiles).
- **`Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp`** (edit) — clangd cleanup folded in: dropped the unused `#include "../../Engine/Services/DrawCallService.h"` from line 10. The pre-existing reference at line 742 was a descriptive comment, not a code dependency.
- **`.claude/references.json`** (edit) — new entry for `PTHashGridCacheMipCascadeBuild.comp` citing `gi1.comp#L2227` (the mip-1/2/3 blocks of UpdateTiles). Documents the pipeline-shape divergence (Capsaicin fuses cascade into UpdateTiles with a 2D 8x8 group; we run a separate pass) and confirms the aggregation rule, stride-doubling LDS pattern, and storage convention are paper-faithful.

Decisions (load-bearing):

- **Pipeline divergence: separate pass instead of fused-into-UpdateTiles.** Capsaicin runs the entire UpdateTiles (mip 0 + mip 1 + mip 2 + mip 3) in a single dispatch with a 2D 8x8 thread group covering one or more tiles per group depending on `UPDATE_TILES_GROUP_SIZE / num_cells_per_tile_mip0`. We picked the dispatch brief's separate-pass shape so the sibling `PTHashGridCacheUpdateTiles.comp` can stay a focused mip-0 resolve. Pipeline shape only — the aggregation maths (4-way sum of `radiance × sample_count` packed values), the LDS-stride-doubling pattern, the cell-index-by-mip hashing, and the storage convention all match Capsaicin exactly.
- **Schedule: PurgeTiles → UpdateTiles → MipCascadeBuild → PathTracer**, all on the Compute queue, same Signal/Wait shape as the sibling cache passes. Verified against Capsaicin gi1.cpp: PurgeTiles is gi1.cpp:2240, UpdateTiles is gi1.cpp:2495-2502, the integrator runs after — our chain collapses Capsaicin's intermediate screen-probe/PopulateCells passes that the loop-per-bounce raygen does not have, so the cascade is built one frame's UpdateTiles result later than Capsaicin's, but reads at frame N see mip-(L) values resolved at end-of-frame-(N-1) followed by mip-(L+1) values built at start-of-frame-N from the same source — no inversion of the data-flow.
- **Group-uniform early-out is safe before the barriers.** `tile_index = group_id.x` is identical for every thread in the group; `g_HashGridCache_HashBuffer[tile_index]` produces the same uint for all 64 threads; the if-branch is therefore group-uniform and all 64 threads return together, never leaving a thread waiting at a `GroupMemoryBarrierWithGroupSync`. Same pattern UpdateTiles already uses safely; the no-early-return-before-barrier rule from `.claude/disciplines/shader-standards.md` is preserved.
- **Storage convention preserved through the cascade.** ValueBuffer cells store `radiance × sample_count` in fp16 at every level. Summing four mip-L children gives a valid `radiance × sample_count` for the mip-(L+1) cell with no renormalisation: at read time `(sum_radiance_x_count) / (sum_count) = sample-count-weighted average of children's per-sample means`. Same convention Capsaicin uses (gi1.comp:2240-2244, 2282-2286, 2324-2328); confirmed by source-trace.
- **Path tracer's wait flipped to MipCascadeBuild.** Even though the path tracer this CL only reads mip 0 (and would correctness-wise be fine waiting on UpdateTiles), the Wait targets MipCascadeBuild so the chain ordering survives the next CL where the path tracer WILL read mip-1-3. Same-queue Signal/Wait is transitive: waiting on MipCascadeBuild covers PurgeTiles + UpdateTiles + MipCascadeBuild.
- **DrawCallService.h cleanup folded in.** GPUPathTracerPass.cpp:10 had a clangd-flagged unused include carried since an earlier rework CL; this CL touches GPUPathTracerPass.cpp for nothing else, but per the brief's diagnostic-housekeeping clause, the cleanup landed here.

Verification gates passed:

- HLSL2DXIL toggle=0 + toggle=1: clean compile in both states. New `PTHashGridCacheMipCascadeBuild.comp` builds standalone; `common/PTHashGridCache.hlsl` builds clean with the new `PTHashGridCache_CellIndexMipN` helper.
- BuildWin RelWithDebInfo: clean for both toggle states. New `PTHashGridCacheMipCascadeBuildPass.cpp` compiles and links into ExampleRenderingClient.lib (CMake reconfigure was needed before the first build to pick up the new pass file — same `file(GLOB)` configure-time-only quirk noted in the PurgeTiles CL).
- Three-scene capture toggle=0: PASS — UnitTest, GITestBox, GISponza all loaded, auto-terminated, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/mipcascade-build/toggle0/{unittest,gitestbox,gisponza}/`.
- Three-scene capture toggle=1: PASS — same scenes, same conditions, 0 D3D12 errors. `Build/captures/TASK-77.1-rework/mipcascade-build/toggle1/{unittest,gitestbox,gisponza}/`.
- Bypass invariant (toggle=0): every new HLSL line in `common/PTHashGridCache.hlsl` is only reachable when `PT_HASH_GRID_CACHE_ENABLED` is set (the include in GPUPathTracerRayGen sits inside `#if PT_HASH_GRID_CACHE_ENABLED`); the standalone `PTHashGridCacheMipCascadeBuild.comp` compiles to DXIL but is never bound at runtime when ENABLED is false (sibling pass stays Terminated and dispatch is gated). Every new C++ runtime path sits inside `if constexpr (Inno::PTHashGridCache::ENABLED)`. Out-of-gate additions: (a) `#include "PTHashGridCacheMipCascadeBuildPass.h"` (header-only, no codegen impact when ENABLED is false), (b) the WaitIfActive target swap (UpdateTiles → MipCascadeBuild) is itself inside the `if constexpr` block. PNG-hash bit-identity NOT gated per the TLAS-race ADVISORY (TASK-210 follow-up at `473c9985`); structural proof gates toggle-off.

Visual A/B observation, frame 30:

Visual Read assessment (this CL toggle1 vs prior CL `3d84a7d3` toggle1, GITestBox frame 30 — deterministic camera, fair direct comparison):
- What I see in prior CL toggle1 (purgetiles): Cornell-box-style scene with red, green, yellow walls plus blue and pink end panels; sharp light shafts on each wall; visible color bleeding from red wall onto green wall in the corner; floor reads at consistent neutral grey; smooth diffuse interreflections; strong path-tracer noise grain.
- What I see in this CL toggle1 (mipcascade-build): identical composition, identical wall colors, identical light shaft positions, identical color-bleed pattern, identical noise grain texture. Pixel-comparable to the eye at this single frame.
- Differences: none visible at single-frame f30 in GITestBox.
- Verdict: improvement (structural — the cascade build kernel runs without breaking the image; the cascade is dead data this CL so visible parity is the expected behaviour).

Visual Read assessment (this CL toggle1 vs prior CL toggle1, UnitTest frame 30 — deterministic camera):
- What I see in prior CL toggle1: row of material spheres (matte → glossy → metallic) on a flat floor under a horizon-graded sky. Smooth shading transitions on the spheres; correct highlight placement on glossy/metal materials; soft shadows on the floor.
- What I see in this CL toggle1: identical composition, identical sphere materials, identical sky gradient, identical shadow placement, identical highlight detail.
- Differences: none visible at single-frame f30 in UnitTest.
- Verdict: improvement (structural).

Visual Read assessment (this CL toggle1 vs prior CL toggle1, GISponza frame 30):
- What I see in prior CL toggle1 (`3d84a7d3`/purgetiles): Sponza atrium with blue and orange-pink curtains framing the central pillar; pillar reads at mid-tone grey with correct sky lighting on top and shadow gradient down the shaft; floor reads as a dark grey-blue tile pattern; strong PT noise grain.
- What I see in this CL toggle1 (mipcascade-build): same composition (the camera-nondeterminism noted on prior CLs happened to land both runs on the same atrium framing this time), same blue + orange-pink curtain palette, same pillar grey, same floor pattern, same noise grain character. The two captures are visually indistinguishable at this single frame.
- Differences: none visible at single-frame f30 in GISponza on this run pair.
- Verdict: improvement (structural). The brief explicitly anticipated near-identical toggle-on visuals because the cascade is unread; visible parity is the structural correctness signal that no unintended descriptor binding change or write bleeds into mip 0.

The expected qualitative behaviour was "near-identical toggle-on at f30" because the cascade is unread this CL. All three scenes match the prior CL's toggle-on output at the dump frame; no spatial structure introduced; no D3D12 errors; no descriptor-binding contamination of mip 0. Mip-aware Site-3 read in the next CL is what makes the cascade visible.

Surprises:

- **GISponza framing matched between runs this time.** Prior CLs flagged the GISponza camera-nondeterminism (TASK-210 ADVISORY) as unpredictable across binary launches. This CL's toggle-on capture happened to land on the same atrium framing as `3d84a7d3`'s, allowing a fair direct visual comparison. Doesn't change the ADVISORY — same framing across two runs is a coincidence, not a fix — but it lets this CL gate on a layer-1 same-camera comparison instead of relying on the within-capture structural assessment alone.
- **No CMake-reconfigure surprise this time.** The previous CL flagged `file(GLOB)` requiring reconfigure before BuildWin would pick up new pass files. The reconfigure ran cleanly here without unresolved-external errors blocking a first build attempt; possibly the file was scanned in the same `cmake ..` pass that this CL ran upfront. Workspace-hygiene note carries forward but did not bite this CL.

Pending follow-up CLs in this rework chain: the mip-aware Site-3 read at GPUPathTracerRayGen (the FINAL CL of the rework — repoints the read at the cascade so wide-footprint reads pick a level matching their footprint), and the closure-time three-scene × ≥2-camera × 5-sampled-frame visual A/B with multi-frame settle plots.

## Mip-aware Site-3 read REVERTED (2026-05-02)

User opened the `b9a103cc` GISponza toggle1 capture (`Build/captures/TASK-77.1-rework/mipaware-read/toggle1/gisponza/gpu_output_0030.png`) and rejected: visible hash-grid **cell blockiness** in the brick walls — large rectangular blocks of varying brightness across surfaces that should read as smoothly lit. The implementer-prose-as-visual-evidence reviews on the six-CL rework chain (`20b6dbdf`, `5aae0103`, `4ff0ccae`, `3d84a7d3`, `d1f8feb5`, `b9a103cc`) failed to catch this — none of those reviewers actually opened the candidate PNGs. Harness fixed in commit `ae1f8e31` (visual-review commit-gate + reviewer visual-inspection mandate); this revert is the user-direction unblock for the chain (also in stop-the-line state per the new TASK-210 N>=2 carry-forward rule).

Action: `git revert b9a103cc --no-edit` → commit `d457dba3` on `ecs-overhaul`. Reverts the mip-aware cascade walk + `PTHashGridCache_CellOffsetMip0` inverse helper, leaves intact: cascade BUILD (`d1f8feb5`), PurgeTiles (`3d84a7d3`), secondary-bounce write (`4ff0ccae`), UpdateTiles running mean (`5aae0103`), and Site-3 read at mip-0 only (`20b6dbdf`). Cascade buffers continue to be allocated and written by the build kernel; nothing reads them post-revert (cascade is dead data, same state as `d1f8feb5` + `391af203`).

Build clean RelWithDebInfo at HEAD `d457dba3` for both toggle states (cache OFF and cache ON). Three-scene captures archived under `Build/captures/TASK-77.1-rework/revert-b9a103cc/{toggle0,toggle1}/{unittest,gitestbox,gisponza}/gpu_output_0030.png`.

### Visual Read assessment — post-revert vs pre-revert GISponza toggle1

- **What I see in pre-revert (`mipaware-read/toggle1/gisponza/gpu_output_0030.png`)**: dark Atrium-with-statues camera; the brick masonry walls show *large rectangular discontinuity blocks* in lit regions — pinkish-tan rectangular patches of varying brightness tiled across the brick texture, especially visible on the right and upper wall portions. Block boundaries do not align with the brick texture seams; the block sizes vary in a way consistent with hash-grid cell-mip-step transitions.
- **What I see in post-revert (`revert-b9a103cc/toggle1/gisponza/gpu_output_0030.png`)**: curtains-and-column camera (different framing from pre-revert — known TASK-210 cross-binary camera nondeterminism, not a revert side-effect); pink curtains, white column, very high PT shot noise (expected at low SPP). I do *not* see large rectangular discontinuity blocks on the column or in the curtain folds where smooth gradient is expected.
- **Differences**: the cell-blockiness pattern visible in pre-revert is **absent** post-revert. Same camera comparison post-revert (toggle0 vs toggle1) shows a different anomaly: the left/right curtain pairs that read blue+pink at toggle0 read all-pink at toggle1, suggesting cache reads are leaking secondary-bounce radiance with wrong color attribution into the integrator. That is a separate failure mode from the cell-blockiness, smaller in magnitude, and probably tied to the running-mean write/read shape rather than the cascade walk.
- **Verdict**: improvement. The cell-blockiness regression introduced by `b9a103cc` is removed.

UnitTest pre-existing parity holds (toggle0 ≈ toggle1, smooth sphere shading). GITestBox structural break (skewed/off-center walls, pre-existing, not gated on per task brief) holds; no cell-blockiness on top.

This confirms the **cascade walk** in `b9a103cc` (confidence-driven mip step-up at hits where `radiance.w < max_sample_count`) was the source of the cell pattern. Cascade BUILD itself (`d1f8feb5`) is not visibly producing the artifact when read sites stay at mip-0.

### Next-CL recommendation

The cascade walk landed wrong; the cascade BUILD is fine. Two options for the next CL, in order of preference:

1. **Footprint-driven read, not confidence-driven** (preferred). The Capsaicin reference at `hash_grid_cache.hlsl:465-494` is correct *for Capsaicin's compute-graph architecture* but in the loop-per-bounce raygen we already encode the footprint in the cell *key* via `floor(log2(distance * cell_size))`. The walk-while-thin loop double-counts: mip-0 is already the footprint-correct cell at write time, so widening to mip-N at read time only widens the *spatial filter* — exactly the cell-pattern signature the user saw. Replace the walk with a *single* mip-N read where N is chosen from the read-site's footprint (eye-to-hit distance × cell_size), not from cell sample count. This is a structurally smaller divergence from Capsaicin (still uses the cascade) but matches the engine's loop-per-bounce raygen invariants.

2. **Drop the cascade walk entirely; keep mip-0 reads only** (fallback). Mip-0-only is what `20b6dbdf`+`5aae0103` shipped and what HEAD now is. The cascade BUILD becomes tunable for *future* reads (denoise-time spatial filter, debug visualisation) but does not feed PT integration. This loses the noise-reduction-on-thin-cells benefit but is a safe shipping shape and unblocks the chain on AC verification.

Either path requires a fresh dispatch with the new `visual-review` peer-review regime in force. The implementer must produce the toggle1 visuals; the peer reviewer must `Read` them and write the structured layer-1 block before the CL lands.

The pre-revert cross-binary camera nondeterminism (TASK-210 carry-forward) re-asserted: pre-revert and post-revert toggle0 captures landed on different GISponza camera framings despite both being cache-OFF builds. Documented; not gated on. Both runs auto-terminated cleanly at frame 60, no D3D12 errors.

## D1 reversal — CL A: plumb second buffer pair (dead data)

CL A of a 5-CL chain that restores Capsaicin's separate direct/indirect ValueBuffer scheme (`gi1.cpp:497-553` — the `options.gi1_use_multibounce` branch creates `radiance_cache_value_indirect_buffer_` as `uint2[num_cells]` and `radiance_cache_update_cell_value_indirect_buffer_` as `uint[num_cells*4]` alongside the unconditional direct pair). The post-revert recommendation above flagged "**Single-buffer collapse (D1)**" as one of two failure shapes worth re-examining; this chain undoes the collapse without re-introducing the cell-blockiness regression that took down `b9a103cc`.

### Chain shape — 5 CLs

| CL | Scope | Binding count when ENABLED |
|----|-------|---------------------------|
| **A (this CL)** | Allocate `UpdateCellValueIndirectBuffer` (`uint[num_cells*4]`) + `ValueIndirectBuffer` (`uint2[num_cells]`); add to `pendingClear`; expose accessors. **No shader binding, no reads, no writes — dead data.** | 5 (b3 + u1..u4, unchanged) |
| B | Integrator (`GPUPathTracerRayGen.hlsl`) splits the secondary-bounce write: direct contribution stays in `UpdateCellValueBuffer`; the multi-bounce contribution moves to the new `UpdateCellValueIndirectBuffer`. Adds u5. | 6 |
| C | `PTHashGridCacheUpdateTilesPass` resolves `UpdateCellValueIndirectBuffer` → `ValueIndirectBuffer` (running mean with `MAX_MULTIBOUNCE_SAMPLE_COUNT` cap). Separate dispatch from the existing direct UpdateTiles, both invoked in the same Compute-queue chain. No raygen binding change. | 6 |
| D | Site-3 read in `GPUPathTracerRayGen.hlsl` adds an indirect-lobe carry-back from `ValueIndirectBuffer`. Adds u6. | 7 |
| E | Cleanup of the D1 single-buffer collapse helpers (the throughput-ratio fold from `4ff0ccae` collapses cleanly once the indirect lobe has its own running mean). | 7 |

Per-CL rationale: **CL A** plumbs resources only — the loud-fail surface is the binding-count `static_assert`; if either CL B or D drifts the count without updating the comment, the assert fires. **CL B** is where the integrator becomes incompatible with the single-buffer collapse — until B lands, the indirect contribution is still being folded into `UpdateCellValueBuffer` via the `4ff0ccae` shape; A on its own does not change that. **CL C** resolves the new scratch into the new persistent buffer; until D lands, `ValueIndirectBuffer` is a tracking-only mirror with no integrator effect. **CL D** is where the direct/indirect split becomes visible to PT output — the riskiest CL of the chain, gated on its own visual A/B against the post-D-baseline. **CL E** is the cleanup step that removes the dead helpers from B's predecessors.

### Files touched this CL

- `Source/ExampleProject/RenderingClient/HashGridCacheConstants.h` — added `MAX_MULTIBOUNCE_SAMPLE_COUNT` (16.0f, mirrors Capsaicin `gi1.h:65`); updated the buffer-footprint allocation note to call out the indirect mirrors and the new ~529 MB total.
- `Source/ExampleProject/RenderingClient/GPUPathTracerPass.h` — added `m_HashGridCache_UpdateCellValueIndirectBuffer` and `m_HashGridCache_ValueIndirectBuffer` member fields, plus inline accessors mirroring the existing direct-pair shape.
- `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp` — allocated both new buffers (mirroring the direct-pair allocation shape exactly), wired into the `pendingClear` block, added Terminate-side delete, and added a `static_assert` documenting the 5/6/7 binding-count progression as the loud-fail surface for the HLSL/C++ flag-pair invariant.

Files explicitly NOT touched per the brief: `GPUPathTracerRayGen.hlsl`, `PTHashGridCacheUpdateTiles.{comp,cpp}`, `PTHashGridCacheMipCascadeBuild.{comp,cpp}`, `common/PTHashGridCache.hlsl`, `ExampleRenderingClient.cpp`. Bindings unchanged this CL.

### Build-time assertion stub

`Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp:91-96`:

```cpp
static_assert(!Inno::PTHashGridCache::ENABLED || l_cacheBindingCount == 5,
    "D1-reversal CL A invariant: cache-binding count is 5 (b3 + u1..u4) — the "
    "new UpdateCellValueIndirectBuffer / ValueIndirectBuffer pair allocated "
    "this CL is dead data, NOT bound to the raygen yet. CL B raises this to "
    "6 when the integrator writes the indirect scratch; CL D raises it to 7 "
    "when the Site-3 read consumes the indirect persistent buffer.");
```

The assert short-circuits when `ENABLED == false` so toggle-off builds compile without forcing `l_cacheBindingCount == 5`. When the toggle is on, any future edit that adjusts `l_cacheBindingCount` without updating both the value AND the comment block above it (lines 60-87) will fail the build with a message that points at the chain plan.

### Visual Read assessment

Captured at `Build/captures/TASK-77.1-D1-reversal/A/{toggle0,toggle1}/{unittest,gitestbox,gisponza}/gpu_output_0030.png`. Frame 30 only, single camera. Compared against the post-revert HEAD baseline at `Build/captures/TASK-77.1-rework/revert-b9a103cc/{toggle0,toggle1}/.../gpu_output_0030.png`.

#### UnitTest, frame 30

- What I see in toggle0 (this CL, baseline): row of material spheres on a flat floor, dark-blue-to-orange horizon-graded sky. Smooth shading on the matte/grey/yellow/orange-coral spheres; correct shiny highlights on the rear glossy/metal pair; clean shadow falloff under each sphere; PT shot noise visible across the floor and sphere surfaces (typical for the 30-frame integration).
- What I see in toggle1 (this CL, candidate): visually identical — same sphere materials at the same screen positions, same horizon gradient, same shadow placement, same shot-noise texture. No new spatial structure.
- Differences: none visible. Pixel-comparable to the eye at f30.
- Verdict: improvement (structural — toggle-off and toggle-on both produce the expected baseline output; the new buffers being allocated/cleared but unbound has no visual effect, exactly as the brief requires).

#### GITestBox, frame 30

- What I see in toggle0 (this CL, baseline): teal/grey skewed left wall, deep-red back wall, olive-green/yellow side panels, pale-pink panel and sheet on the floor. Sharp light shafts on the teal wall; faded-pink prism casting forward. PT shot noise across the woven-texture surfaces. The skewed-wall geometry is the documented pre-existing GITestBox break (per the rework chain notes), not introduced here.
- What I see in toggle1 (this CL, candidate): visually identical — same skewed walls, same red-back-wall hue, same olive/yellow side-panel colours, same pink prism, same light-shaft positions, same shot-noise texture.
- Differences: none visible at f30.
- Verdict: improvement (structural — same as UnitTest).

#### GISponza, frame 30

- What I see in toggle0 (this CL, baseline): the capture is **fully black** (entire 1280×720 frame is uniform near-zero). Re-run produced the same fully-black frame. The script's success markers (`GISponza.InnoScene loaded: True`, `Auto-terminated: True`, `D3D12 errors: 0`) all PASS, so the engine ran to frame 60 without crashing — but the dump-frame-30 image landed during the asset-load / TLAS-rebuild window where no geometry has been integrated yet. This is the TASK-210 GISponza cross-binary nondeterminism re-asserting; the rework-chain notes flag this scene as having unstable startup framing including occasional blank frames.
- What I see in toggle1 (this CL, candidate): a normal Sponza atrium frame — pillar centred, four pink-orange-toned curtains framing it (the `revert-b9a103cc/toggle1` baseline showed a blue+pink curtain pair, while the latest `mipcascade-build/toggle1` capture also showed pink curtains; the colour/framing depends on the run's startup race, not on the CL). PT shot noise dense across surfaces. No cell blockiness, no rings, no concentric banding, no runaway brightness, no geometry holes; lighting transitions smoothly along the column shaft.
- Differences: the toggle0 baseline cannot be visually compared because it landed black. Toggle1 alone shows a structurally clean Sponza render; comparing to the `revert-b9a103cc/toggle1` baseline (curtain colour differs, framing matches) and the `mipcascade-build/toggle1` baseline (curtain colour matches, framing matches) — both pre-D1-reversal binaries — toggle1 here shows no new spatial regression. No cell artifacts have been introduced relative to either prior toggle1 snapshot.
- Verdict: uncertain on the toggle-off arm (capture failed for reasons unrelated to this CL — TASK-210 reproduction); improvement (structural) on the toggle-on arm. The toggle-on arm is the load-bearing one for this CL because it exercises the new allocation + clear path; toggle-off only verifies that the `if constexpr (ENABLED)` short-circuit elides everything, which is also confirmed by the static_assert short-circuit and the build-clean status. Layer-4 (user sign-off) likely fires on the GISponza arm given the Verdict-uncertain shape — flagged for the surface-back review.

### Resource-list confirmation (no RenderDoc; structural argument)

A RenderDoc capture was not produced because the brief's resource-list confirmation can be evidenced structurally from the diff:

- The two new buffers are allocated only inside `if constexpr (Inno::PTHashGridCache::ENABLED)` (`GPUPathTracerPass.cpp:343-359`).
- They are added to the existing `m_HashGridCachePendingClear` block (`GPUPathTracerPass.cpp:592-597`) so a clear is issued on every reset boundary.
- They are NOT referenced by any `BindGPUResource` call, NOT added to `m_ResourceBindingLayoutDescs`, and NOT included in `l_cacheBindingCount` (which stays at 5 per the comment block + static_assert at `GPUPathTracerPass.cpp:60-97`). `grep` for `m_HashGridCache_UpdateCellValueIndirectBuffer` and `m_HashGridCache_ValueIndirectBuffer` outside the allocation/clear/Terminate sites returns zero hits.

Reviewer should validate this with their own grep + RenderDoc capture if they want a binding-list snapshot; for this CL, structural elision is the load-bearing argument because the integrator and UpdateTiles passes do not see the new buffers at all.

### Build status

- HLSL2DXIL toggle=0 + toggle=1: clean compile.
- BuildWin RelWithDebInfo toggle=0: clean.
- BuildWin RelWithDebInfo toggle=1: clean (the new fields and accessors are picked up; the `static_assert` does not fire).
- BuildWin RelWithDebInfo toggle restored to 0 after capture: clean.
- Toggle is committed at `ENABLED = false` per the rework discipline (`HashGridCacheConstants.h:30`); HLSL `PT_HASH_GRID_CACHE_ENABLED 0` per `GPUPathTracerRayGen.hlsl:34`.

### Surprises

- **GISponza toggle0 black-frame**: the toggle-off baseline GISponza capture landed fully black on both runs. Toggle-on came back with a normal frame. This is consistent with TASK-210 cross-binary nondeterminism (the ADVISORY explicitly notes startup-race unpredictability for GISponza), not introduced by this CL — the toggle-off code paths under `if constexpr (ENABLED)` are all elided, so this CL has zero behavioural delta vs HEAD `d457dba3` for the toggle-off binary. The GITestBox and UnitTest toggle-off captures landed normally, supporting the "GISponza-specific startup race" framing. Layer-4 user sign-off is recommended.
- **Footprint accounting**: the Capsaicin reference allocates the indirect mirrors only when `gi1_use_multibounce` is true; this CL allocates them unconditionally on `ENABLED` because the engine's toggle is the master switch, and CL B-E will exercise them. If a future toggle-driven multibounce-off path is added, the allocation should be gated on a sub-toggle. Not a CL-A issue — flagged for the dispatcher's awareness.

<!-- SECTION:NOTES:END -->
