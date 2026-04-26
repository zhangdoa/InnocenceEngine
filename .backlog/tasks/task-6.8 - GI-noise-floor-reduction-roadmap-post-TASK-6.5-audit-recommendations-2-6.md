---
id: TASK-6.8
title: 'GI noise-floor reduction roadmap (post-TASK-6.5 audit recommendations #2-#6)'
status: To Do
assignee: []
created_date: '2026-04-26 13:01'
labels:
  - rendering
  - GI
  - paper-faithful
  - noise-reduction
dependencies:
  - TASK-6.7
references:
  - .alignments/post-TASK-6.5-noise-gap.md
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Roadmap of remaining auditor-prescribed work after TASK-6.7 (NUM_SAMPLES 1→16, recommendation #1) lands.

The auditor's full alignment artifact is at `.alignments/post-TASK-6.5-noise-gap.md`. Headline finding: GISponza noise gap is dominated by a 64× ray-budget shortfall (1 ray per spawn-tile per frame vs Capsaicin's 64 = one per octahedral cell per probe). Three structural amplifiers (D2/D3/D5) compound it. Denoiser/filter stages are line-for-line ports of Capsaicin and not the source of the gap.

This task tracks the remaining recommendations (#2-#6) so they don't get lost. Each should be filed as its own task as it becomes the next-best move post-measurement.

### Pending recommendations (in auditor's ranked order)

**#2 — Restructure dispatch to `(W/16, H/16, 64)`** (1-2 days). One thread = one cell of one probe (Capsaicin's exact shape). Move per-probe state (CDF builds, ImportanceSampleFromCDF) to LDS shared across the 64 threads of a probe. Closes the remaining 20% of D1 after #1 lands. Only do this if #1 alone doesn't close enough of the visible gap.

**#3 — Drop `kGIDenoiser_MaxBlurMask` 16→8** (15 min). `GIDenoise.comp:101`, `GIFilterCommon.hlsl:34`, derived `MaxCapFactor` at `GIDenoise.comp:113`. Matches Capsaicin's `gi_denoiser.hlsl:26`. Tightens edge preservation once upstream noise is reduced. Should land *after* #1 — before #1 the wider blur is compensating for upstream noise.

**#4 — Lower `COVERAGE_ACTIVATION_THRESHOLD` 0.9→0.5** (15 min). `RadianceCacheIntegration.comp:90`. Resolves D5: at our current 1-ray density the gate never fires; SH DC coefficient is biased low by 64×. Once #1 raises samples-per-probe-per-frame, lower the gate so the per-probe-average backup actually fills cold cells. Auditor notes: "Capsaicin equivalent: backup is unconditional".

**#5 — Sub-pixel jitter in `SampleRadianceCache`** (1h). `RadianceCacheCommon.hlsl:250`. Resolves D3: every pixel reads the same 4 probes every frame, so noise is fixed-pattern that bilateral kernel reads as signal. Capsaicin pattern at `gi1.comp:1540-1553` (blue-noise jittered ±probe_spawn_tile_size + world-plane gate). Halton(2)/Halton(3) on `g_Frame.frameIndex` is also paper-acceptable, no new bindings needed. **Compounds with #1**; safe to bundle if user wants one bigger CL.

**#6 — Probe-mask MIP chain** (30 min interim / 2-3h full). Resolves D2: our `FindClosestProbe` Chebyshev ring bails at radius 2; Capsaicin cascades up MIP chain to whole screen O(log r). Adds disocclusion-edge noise specifically. Interim: raise `PROBE_SEARCH_MAX_RING` from 2 to 4 (each step costs (2k+1)² - (2k-1)² = 8k texels). Full: add `FilterProbeMask` dispatch + multi-mip RTexture2D + port Capsaicin `screen_probes.hlsl:110-166`.

### #7 (separate scope, file separately if pursued)

**D4 closest-hit reads previous-frame LightPass output (energy double-count risk)** — `RadianceCacheClosestHit.hlsl:48-58`. Bias correction, not noise. Defer until #1-6 done. Fix is store a direct-only-luminance LightPass output (separate from RT0 = direct + indirect), or compute direct lighting inline at the hit point.

### Measurement-driven sequencing

After TASK-6.7 (recommendation #1) lands, measure noise level vs `Build/captures/TASK6_5_post_sponza/`. If the visible gap is mostly closed, prioritize #5 (jitter, low effort, additive). If still significant, tackle #2 (full restructure) before #5. Use the actual measurement to drive the order.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Each of #2-#6 either filed as its own task and worked through, or explicitly closed with rationale (e.g. 'TASK-6.7 alone was sufficient on GISponza, deferring #N until X scene/regression')
- [ ] #2 Final roadmap status: every auditor-numbered recommendation has a filed-or-deferred decision recorded with measurement evidence
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
