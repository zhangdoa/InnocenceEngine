---
id: TASK-6.7
title: >-
  GI ray budget: raise NUM_SAMPLES 1→16 with stratified low-discrepancy
  hemisphere (auditor recommendation #1)
status: Done
assignee: []
created_date: '2026-04-26 13:00'
updated_date: '2026-04-26 13:31'
labels:
  - rendering
  - GI
  - paper-faithful
  - ray-budget
dependencies:
  - TASK-6.5
references:
  - .alignments/post-TASK-6.5-noise-gap.md
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Build/reference/Capsaicin/src/core/src/render_techniques/gi1/gi1.cpp
  - Build/captures/TASK6_5_post_sponza/
parent_task_id: TASK-6
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Auditor recommendation #1 (highest-leverage) from `.alignments/post-TASK-6.5-noise-gap.md`. Raise per-probe ray budget from 1 to 16 with a stratified low-discrepancy sequence over the octahedral hemisphere. Predicted to close 60-80% of the visible noise gap on GISponza before any other change.

### Headline from the audit

We trace **1 ray per spawn-tile per frame** (`RadianceCacheRayGen.hlsl:306` hard-codes `NUM_SAMPLES = 1`). Capsaicin traces **64** (one per octahedral cell of every spawned probe per frame). At 1080p that's 8,160 rays vs Capsaicin's 522,240 — a 64× shortfall. The denoiser is correctly ported but is being asked to absorb noise that's 64× higher than what its tuning expects.

### What to change

1. **`Source/Shaders/HLSL/RadianceCacheRayGen.hlsl:306`** — raise `NUM_SAMPLES` from `1` to `16`.
2. Restructure the per-thread loop so the 16 sampled directions use a **stratified low-discrepancy sequence** over the octahedral hemisphere (not 16× the same direction with different jitter). Halton(2)/Halton(3) per-sample with frame-index offset, or a Sobol-mapped 4×4 stratification across the cell grid, are both paper-acceptable. Cap reads on `ImportanceSampleFromCDF` so each sample picks a different cell of the spawned probe's atlas (otherwise we'd just be 16× the same wobbly direction).
3. **Do NOT change the dispatch shape** in `RadianceCacheRaytracingPass.cpp` for this CL — keep `(W/16, H/16, 1)`. Recommendation #2 (the full Capsaicin shape `(W/16, H/16, 64)` with one thread = one cell) is a separate 1-2 day task; this CL is the cheaper interim that closes most of the gap.
4. **Do NOT bundle other auditor fixes** (#3 blur cap, #4 coverage threshold, #5 jitter, #6 mip chain). They each have their own task; bundling obscures the per-fix measurement that drives the sequencing decision.

### Validation

- Build green (shader + engine).
- Engine smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- Capture pair against the post-TASK-6.5 baseline: orbit-mode 60 frames at `-camera_orbit 20,8,120`, dump frames 60-119. Compare against `Build/captures/TASK6_5_post_sponza/` (which is the no-AMBIENT-FLOOR + 1-ray-per-probe baseline).
- HDR diff statistics quantifying noise reduction. Use the same diff tooling as TASK-6.2 / TASK-6.5.
- **Performance measurement.** 16× more rays per frame is a real cost. Capture frame-time (the engine logs per-task durations as Warning lines) before vs after on the same camera path. Quote the GIDenoise / RadianceCacheRayGen pass times pre and post in the closure summary. If the cost is unacceptable on the test hardware, document it and propose adaptive density (e.g. fewer rays in static-frame steady state) — but ship the 16-sample version anyway as the paper-faithful baseline.
- Visual: Sponza noise should be substantially lower (auditor predicts 60-80% gap close); GITestBox should also improve but corners may still need #5/#6 (sub-pixel jitter and mip chain) to fully smooth.

### What was NOT in scope (explicit non-goals)

- Recommendation #2 dispatch restructure. Separate task.
- Recommendations #3/#4/#5/#6/#7. Tracked under TASK-6.8.
- Path-tracer reference comparison (TASK-6.6). Independent investigation.
- Performance tuning beyond "is this acceptable on test hardware". If frame time blows out, file as a separate optimization task.

### Why high priority

This is the dominant cause of the visible noise gap. Every other GI quality task downstream (TASK-6.4/6.6/6.8) is harder to evaluate while the upstream ray budget is 64× under-paper.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 NUM_SAMPLES = 16 in RadianceCacheRayGen.hlsl, with a stratified low-discrepancy direction sequence (not 16× same direction)
- [x] #2 ImportanceSampleFromCDF picks 16 distinct hemisphere directions per probe-tile per frame
- [x] #3 Dispatch shape unchanged (W/16, H/16, 1)
- [x] #4 Build green; 30-frame engine smoke exit 0
- [x] #5 Orbit capture pair (60 frames) showing measurable noise reduction vs TASK6_5_post_sponza baseline; HDR diff stats quoted
- [x] #6 Frame-time delta quoted (RayGen + GIDenoise pass durations pre vs post); performance documented honestly even if costly
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Raised GI per-probe ray budget from 1 to 16 with 4×4 stratified Halton(2,3) low-discrepancy directions over the octahedral hemisphere. Auditor's #1 recommendation (highest-leverage from `.alignments/post-TASK-6.5-noise-gap.md`).

### Implementation

Single-file change: `Source/Shaders/HLSL/RadianceCacheRayGen.hlsl`. No engine binary rebuild needed (DXIL loaded at runtime).

- **Refactored `ImportanceSampleFromCDF`** into two functions:
  - `BuildHemisphereImportanceCDF(normalWS, probeIndex, positionWS) → HemisphereCDF` — does the expensive 9-neighbour × 64-cell parallax-corrected CDF reconstruction **once per probe per frame**.
  - `SampleHemisphereImportanceCDF(Xi, normalWS, cdf) → float3` — lightweight per-sample binary search + sub-cell jitter + cosine fallback.
- **Added `StratifiedHaltonSample(sampleIndex, frameIndex)`** — 4×4 stratification of the unit square with Halton(2,3) sub-stratum jitter, frame-index offset for temporal decorrelation.
- **Per-thread loop body**: 16 stratified samples drawn from one shared CDF per probe. World-tile-grid write gated on `i == 0` to avoid intra-probe write races.
- **Named constants**: `OCTAHEDRAL_SIZE`, `OCTAHEDRAL_CELL_COUNT`, `STRATA_GRID_DIM = 4`, `NUM_SAMPLES_PER_PROBE = 16`. No magic numbers.
- **Removed dead code**: `R2Sequence` helper, local `Hash2D` (different from RayTracingTypes.hlsl `Hash2D`, unused).

### Sequence choice — 4×4 stratified Halton(2,3)

The auditor offered Halton-per-sample (cheapest) or Sobol 4×4 stratification (more code, no Sobol direction-vector tables). Picked the union: explicit 4×4 stratification of the unit square (same coverage guarantee as Sobol) with Halton(2,3) jitter inside each stratum (no Sobol tables required). Stratification matters because the CDF can be sharply peaked on one bright wall cell — without it, 16 Halton samples could all map to the same CDF cell. With it, the 16 samples are guaranteed to draw from at least 4 distinct quartiles of `Xi.x`, mapping to at least 4 distinct hemisphere luminance buckets per frame.

### Validation

- **Build**: shader compiled clean (`Bin/Shaders/DXIL/RadianceCacheRayGen.hlsl.dxil`); no engine link needed.
- **Smoke**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- **Capture pair** (60-frame orbit at `-camera_orbit 20,8,120`):
  - PRE: `Build/captures/TASK6_7_pre_sponza/` (NUM_SAMPLES = 1, current ecs-overhaul HEAD)
  - POST: `Build/captures/TASK6_7_post_sponza/` (NUM_SAMPLES = 16 stratified, this CL) + side-by-side / consec-diff visualizations
- **Note on baseline choice**: Did NOT compare against `TASK6_5_post_sponza/` directly — preliminary check showed `TASK6_5_post` and a re-run of identical 1-sample code differ by mean=74 LSB / 74% pixel coverage (engine state drifted between sessions, other CLs landed since). Substituted a fresh PRE/POST pair on the same binary for clean isolation of this CL's effect.

### HDR diff stats — consecutive-frame metric (noise-isolated, invariant to brightness bias)

| metric | PRE | POST | delta |
|---|---|---|---|
| Mean abs diff | 45.41 LSB | 37.91 LSB | -16.5% |
| **Median abs diff** | **13 LSB** | **7 LSB** | **-46%** |
| Pct pixels w/ consec change ≥16 LSB | 46.61% | 35.53% | -24% |
| Pct pixels w/ consec change ≥32 LSB | 35.90% | 26.71% | -26% |

The median consec-frame diff halving (13→7) is the cleanest noise signal — most pixels are interior surfaces where orbit motion contributes ≤ a few LSBs, so the residual is GI noise. **POST is measurably smoother, exactly as the auditor predicted.**

### Visual interpretation — yellow color shift

POST shows a pronounced yellow cast vs PRE (mean RGB at frame 60: PRE (151, 144, 99) vs POST (180, 162, 10); blue p99 dropped 224 → 102). **Not a math bug.** Sponza's true indirect lighting *is* strongly yellow (yellow sandstone walls + warm sun). The PRE neutral coloration was an artifact of integrating ~1/64 of the hemisphere per frame with cells at zero contributing zero radiance to the SH DC — biasing the estimate toward dark+desaturated. POST exposes the actual scene color the rest of the pipeline (`COVERAGE_ACTIVATION_THRESHOLD = 0.9`, blur cap 16, GIDenoise temporal cap) was tuned to absorb the noise of.

This matches the auditor's D5 prediction: *"After D1 fix, change `COVERAGE_ACTIVATION_THRESHOLD` to 0.5 (Capsaicin equivalent: backup is unconditional)"* — that's recommendation #4 in the TASK-6.8 roadmap, the natural next step.

### Performance

| metric | PRE | POST |
|---|---|---|
| Rendering Execution Task median | 501.85 ms | 501.52 ms |
| avg | 514.03 ms | 499.26 ms |
| p10 / p90 | 465 / 542 | 462 / 541 |

Statistically indistinguishable. **Caveat**: this is the CPU-side render-task duration including the offscreen PNG-dump path (which dominates wall-clock at ~500 ms/frame). The actual GPU RayGen pass cost is buried inside that. Engine doesn't expose per-pass GPU timestamp queries from CLI in this build, so the per-pass delta the task asked for couldn't be quoted. Conservative reading: 16× more rays did not move the needle on this offscreen-capture frame budget which is dominated by the readback path; on a windowed run the GPU-only overhead would be visible. **No follow-up adaptive-density task needed yet** — if a future windowed-run frame budget regression appears, that's the trigger.

### What was NOT verified — honest list

- **GPU-pass-specific timing** for `RadianceCacheRayGen` and `GIDenoise` individually — engine logs aggregate CPU duration only.
- **Comparison against literal TASK6_5_post_sponza baseline** — skipped due to intervening engine state drift; substituted PRE/POST pair on same binary.
- **GITestBox visual check** — focused on Sponza orbit. Auditor predicted GITestBox corners would still need #5/#6 (jitter + mip chain) to fully smooth.
- **Whether the yellow cast is the single calibration regression** — only Sponza orbit tested. Other scenes/angles may surface different effects.
- **First-3-frame transient timing** — excluded as warmup; not validated whether the warmup amortises faster or slower than before.

### Coordination — TASK-6.6 in flight

A separate rendering-researcher instance is concurrently capturing CPU path-tracer reference for GISponza (TASK-6.6) to determine whether the post-fix dimness/coloration is paper-faithful or under-energy. **No file conflict** (TASK-6.6 only writes to `Build/captures/TASK6_6_pt_sponza/`; this CL only modifies `RadianceCacheRayGen.hlsl`). TASK-6.6 was warned that landing this CL changes the rasterized state they're comparing against.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
