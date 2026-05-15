---
id: TASK-226.3
title: Port FilterScreenProbes weight kernel (Capsaicin depth+normal weights)
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
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L1455
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the current `pow(depthRatio, 8.0)` weight formulation in the probe-space radiance filter with Capsaicin's depth+normal weight at gi1.comp:1455–1519. Shape (separable H/V, kRadius=3, parallax-corrected direction reprojection, bilateral on depth+plane+hemisphere) is already paper-aligned per gap-matrix row #6 — this is the only known divergence in that row.

Touched files:
- Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp
- Source/Shaders/HLSL/RadianceCacheFilterVertical.comp
- Possibly Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (if weight is shared)

Surgical change — no pass restructure, no new C++ wiring.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Weight formulation matches Capsaicin gi1.comp:1455–1519 within line-level audit tolerance
- [ ] #2 #2 Shader build green
- [ ] #3 #3 Engine build green
- [ ] #4 #4 Sponza autotest renders without artifacts at RasterizedGI=ON
- [ ] #5 #5 Visual diff vs TASK-226.2 baseline: filter region no worse than baseline (capture screenshot diff)
- [ ] #6 #6 Paper-port alignment artifact .alignments/TASK-226.3-port-audit.md cites Capsaicin lines
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
