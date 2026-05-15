---
id: TASK-226.7
title: Port InterpolateScreenProbes bent-cone SH evaluation (replace flat SH)
status: To Do
assignee: []
created_date: '2026-05-15 20:24'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
dependencies:
  - TASK-226.2
references:
  - .alignments/TASK-226-gap-matrix.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L1571
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the current plain Ramamoorthi–Hanrahan SH cosine convolution (RadianceCacheCommon.hlsl:199–204) with Capsaicin's `ScreenProbes_CalculateSHIrradiance_BentCone` (gi1.comp:1645). Bent-cone-modulated SH evaluation is the source of the small-scale-occlusion visible-quality divergence flagged in gap-matrix row #7. Per-pixel 4-probe bilinear blend shape stays unchanged.

Touched files:
- Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (SampleRadianceCache / SH eval inner loop)
- Source/Shaders/HLSL/RadianceCacheIntegration.comp (denoiser_hint shape already matches per row #7)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Bent-cone SH eval ported from Capsaicin ScreenProbes_CalculateSHIrradiance_BentCone (gi1.comp:1645)
- [ ] #2 #2 Per-pixel 4-probe bilinear blend shape preserved
- [ ] #3 #3 Shader + engine build green
- [ ] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline; small-scale occlusion (e.g. curtain folds, pillar bases) renders closer to Capsaicin reference output
- [ ] #5 #5 .alignments/TASK-226.7-port-audit.md cites Capsaicin line ranges
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
