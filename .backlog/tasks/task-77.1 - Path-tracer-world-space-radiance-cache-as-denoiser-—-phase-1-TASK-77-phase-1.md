---
id: TASK-77.1
title: Path-tracer world-space radiance cache as denoiser — phase 1 (TASK-77 phase 1)
status: To Do
assignee: []
created_date: '2026-04-30 19:14'
updated_date: '2026-04-30 19:44'
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
## Why

Phase 1 of TASK-77 lands a **world-space radiance cache** as the path tracer's denoiser. Cache hits are low-variance reads of previously-integrated radiance keyed on world position + normal; cache misses fall back to full PT integration. The cache itself doubles as the denoiser — high-confidence cache values smooth out PT's per-pixel noise floor without a separate temporal-reprojection step.

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

## Acceptance Criteria (draft — refined by design call)

These are seeds; the design pass refines them. Don't over-specify before the design call closes.

- [ ] #1 Cache structure picked + rationale documented in Implementation Notes (probe grid / voxel cascade / hash grid / surfels — design call's call), with reference-impl-alignment evidence
- [ ] #2 Cache write site(s) in `GPUPathTracer` identified and authored (per-bounce / per-vertex / primary-hit-only — design call decides)
- [ ] #3 Cache read / composition rule decided and wired (lerp-by-confidence / threshold-fallback) — bias/variance trade documented
- [ ] #4 GPU memory footprint estimated against actual allocator usage; cap + overflow policy documented; within budget for target scenes
- [ ] #5 Engine builds clean (RelWithDebInfo); GBV clean on smoke run
- [ ] #6 On-screen visual A/B: denoiser off (raw PT) vs denoiser on (cache-assisted), same camera path on Sponza interior, stills + short video — documented in Final Summary with a clear noise-floor delta
- [ ] #7 Peer review per `peer-review-required.md` — fresh-context reviewer of opposite role family (graphics-api-expert reviews if rendering-researcher implements; vice versa)

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
- [ ] #1 Cache structure picked + rationale documented in Implementation Notes (probe grid / voxel cascade / hash grid / surfels — design call's call), with reference-impl-alignment evidence
- [ ] #2 Cache write site(s) in GPUPathTracer identified and authored (per-bounce / per-vertex / primary-hit-only — design call decides)
- [ ] #3 Cache read / composition rule decided and wired (lerp-by-confidence / threshold-fallback) — bias/variance trade documented
- [ ] #4 GPU memory footprint estimated against actual allocator usage; cap + overflow policy documented; within budget for target scenes
- [ ] #5 Engine builds clean (RelWithDebInfo); GBV clean on smoke run
- [ ] #6 On-screen visual A/B: denoiser off (raw PT) vs denoiser on (cache-assisted), same camera path on Sponza interior, stills + short video — documented in Final Summary with a clear noise-floor delta
- [ ] #7 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Design call resolution (2026-04-30)

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
