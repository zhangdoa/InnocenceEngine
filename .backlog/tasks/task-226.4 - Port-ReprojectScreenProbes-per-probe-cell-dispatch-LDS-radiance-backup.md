---
id: TASK-226.4
title: Port ReprojectScreenProbes (per-probe-cell dispatch + LDS radiance-backup)
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
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L659
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Rewrite RadianceCacheReprojection.comp to per-probe-cell `[numthreads(8,8,1)]` dispatch matching Capsaicin's ReprojectScreenProbes (gi1.comp:659–878). Compute a "radiance backup" via parallel reduction in LDS (gi1.comp:845–857) and write it to unvisited cells on disocclusion (line 920). Delete the in-house side-cache textures and the Halton-jitter-selection + InterlockedMin distance-scoring scheme (RadianceCacheReprojection.comp:56–60, 199–202, 270–315) — they're in-house heuristics not in the reference.

Touched files:
- Source/Shaders/HLSL/RadianceCacheReprojection.comp
- Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.{cpp,h}
- Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass_Setup.cpp (side-cache resource decls removed)

Design note: TASK-226.4's side-cache nuke assumes the LDS radiance-backup fully replaces the side cache. Capsaicin doesn't have a side cache, so this IS the paper-faithful position. If LDS backup turns out visibly inferior on long disocclusions during impl, the side cache STAYS nuked — avoid keeping a Plan-B redundancy that re-introduces the divergence.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 Reprojection dispatched per-probe-cell with LDS-backup parallel reduction matching gi1.comp:659–878
- [ ] #2 #2 In-house side-cache textures + Halton-jitter/InterlockedMin scheme removed from shader + pass C++
- [ ] #3 #3 Shader + engine build green
- [ ] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline on continuous-camera regions
- [ ] #5 #5 Disocclusion regions (camera-cut test case) render with LDS-backup fill, no stale ghost cells
- [ ] #6 #6 .alignments/TASK-226.4-port-audit.md cites Capsaicin line ranges + records the side-cache-nuke design call
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
