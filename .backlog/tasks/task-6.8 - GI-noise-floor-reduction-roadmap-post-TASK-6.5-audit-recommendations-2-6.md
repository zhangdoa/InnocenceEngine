---
id: TASK-6.8
title: 'GI noise-floor reduction roadmap (post-TASK-6.5 audit recommendations #2-#6)'
status: Done
assignee: []
created_date: '2026-04-26 13:01'
updated_date: '2026-05-15'
labels:
  - rendering
  - GI
  - paper-faithful
  - noise-reduction
  - superseded
dependencies:
  - TASK-6.7
references:
  - .alignments/post-TASK-6.5-noise-gap.md
parent_task_id: TASK-6
priority: medium
---

## Closure (2026-05-15) — superseded by TASK-226

Closed as superseded by the AMD GI 1.0 reference-port effort (TASK-226). The recommendations 2-6 from the TASK-6.5 self-audit are a roadmap for tuning the current broken impl. Direction (2026-05-15) abandons that path in favour of mirroring the reference implementation. Any noise-reduction recommendations the reference impl applies will land naturally during the port.

If after the port the noise floor is still above the paper's quality bar, file a fresh task scoped against the post-port code paths and reference-impl noise levels.

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Roadmap of remaining auditor-prescribed work after TASK-6.7 (NUM_SAMPLES 1→16, recommendation #1) lands.

The auditor's full alignment artifact is at `.alignments/post-TASK-6.5-noise-gap.md`. Headline finding: GISponza noise gap is dominated by a 64× ray-budget shortfall (1 ray per spawn-tile per frame vs Capsaicin's 64 = one per octahedral cell per probe). Three structural amplifiers (D2/D3/D5) compound it. Denoiser/filter stages are line-for-line ports of Capsaicin and not the source of the gap.

This task tracks the remaining recommendations (#2-#6) so they don't get lost. Each should be filed as its own task as it becomes the next-best move post-measurement.

### Pending recommendations (in auditor's ranked order)

**#2 — Restructure dispatch to `(W/16, H/16, 64)`** (1-2 days). One thread = one cell of one probe (Capsaicin's exact shape). Move per-probe state (CDF builds, ImportanceSampleFromCDF) to LDS shared across the 64 threads of a probe. Closes the remaining 20% of D1 after #1 lands. Only do this if #1 alone doesn't close enough of the visible gap.

**#3 — Drop `kGIDenoiser_MaxBlurMask` 16→8** (15 min). `GIDenoise.comp:101`, `GIFilterCommon.hlsl:34`, derived `MaxCapFactor` at `GIDenoise.comp:113`. Matches Capsaicin's `gi_denoiser.hlsl:26`. Tightens edge preservation once upstream noise is reduced. Should land *after* #1 — before #1 the wider blur is compensating for upstream noise.

**#4 — Lower `COVERAGE_ACTIVATION_THRESHOLD` 0.9→0.5** (15 min). LANDED in TASK-6.9 (commit 94949890).

**#5 — Sub-pixel jitter in `SampleRadianceCache`** (1h). `RadianceCacheCommon.hlsl:250`. Resolves D3: every pixel reads the same 4 probes every frame, so noise is fixed-pattern that bilateral kernel reads as signal. Capsaicin pattern at `gi1.comp:1540-1553` (blue-noise jittered ±probe_spawn_tile_size + world-plane gate). Halton(2)/Halton(3) on `g_Frame.frameIndex` is also paper-acceptable, no new bindings needed.

**#6 — Probe-mask MIP chain** (30 min interim / 2-3h full). Resolves D2: our `FindClosestProbe` Chebyshev ring bails at radius 2; Capsaicin cascades up MIP chain to whole screen O(log r). Adds disocclusion-edge noise specifically. Interim: raise `PROBE_SEARCH_MAX_RING` from 2 to 4 (each step costs (2k+1)² - (2k-1)² = 8k texels). Full: add `FilterProbeMask` dispatch + multi-mip RTexture2D + port Capsaicin `screen_probes.hlsl:110-166`.

### #7 — WITHDRAWN 2026-04-26 (wrong premise)

**D4 closest-hit reads previous-frame LightPass output (energy double-count risk)** was originally tracked here and promoted to TASK-6.11. TASK-6.11 verified the audit's premise is wrong: the closest-hit's `in_LightPassOutgoingLuminance` SRV maps to `LightPass::GetIlluminanceResult()` = `out_lightPassRT1` = Lambertian-only direct (not RT0 = direct + indirect). The shader's own design comment at `lightPass.comp:140-143` confirms the engine is already paper-faithful on this point. TASK-6.11 closed wrong-premise; the post-TASK-6.5 audit's D4 entry annotated WITHDRAWN. **Do not re-open.**

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
