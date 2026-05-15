---
id: TASK-226.6
title: 'Port SampleScreenProbes (64 rays per probe, workgroup-parallel CDF scan)'
status: To Do
assignee: []
created_date: '2026-05-15 20:24'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
  - perf-risk
dependencies:
  - TASK-226.2
  - TASK-226.5
references:
  - .alignments/TASK-226-gap-matrix.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L481
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rewrite the radiance-cache raygen ray-spawn pattern to match Capsaicin's SampleScreenProbes (gi1.comp:481–553, scan at line 541). One ray per atlas cell = 64 rays per probe per frame (current: 16, NUM_SAMPLES_PER_PROBE at RadianceCacheRayGen.hlsl:185). CDF built via parallel scan across the workgroup (Capsaicin's ScreenProbes_ScanRadiance) — fundamentally different shape from the current in-host-shader 9×64 CDF loop. The 4× ray-count deficit is the dominant source of residual noise per the umbrella description.

Touched files:
- Source/Shaders/HLSL/RadianceCacheRayGen.hlsl (kernel body, dispatch dims, CDF construction)
- Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (StratifiedHaltonSample / BuildHemisphereImportanceCDF if shared)
- Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.{cpp,h} (dispatch shape change)

PERF-AC RISK: 4× ray-count plus parallel-scan CDF is the dominant cost increase. Run TASK-226.2 baseline FIRST; this CL re-runs the capture and writes a delta. If perf falls below the 60-FPS bar on Sponza, the umbrella AC #5 needs re-scoping to "no regression vs baseline" — that decision lives in TASK-226.8 (final gate), not here.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Ray count = 64 per probe, dispatch shape (W/8, H/8, 64) or equivalent matching Capsaicin SampleScreenProbes
- [ ] #2 #2 CDF built via parallel scan across workgroup (LDS scan, not per-thread 9×64 loop)
- [ ] #3 #3 Shader + engine build green
- [ ] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline (or surfaces the regression in the audit doc)
- [ ] #5 #5 Perf delta captured (frame-time min/avg/max) vs TASK-226.2 baseline
- [ ] #6 #6 .alignments/TASK-226.6-port-audit.md cites Capsaicin line ranges + records the perf delta + 60-FPS-bar status
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
