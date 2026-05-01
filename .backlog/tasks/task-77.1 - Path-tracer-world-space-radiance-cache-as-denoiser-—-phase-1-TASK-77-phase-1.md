---
id: TASK-77.1
title: Path-tracer world-space hash-grid radiance cache as denoiser (rework)
status: To Do
assignee: []
created_date: '2026-04-30 19:14'
updated_date: '2026-05-01 18:00'
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
<!-- SECTION:NOTES:END -->
