---
id: TASK-226.5
title: Replace FindClosestProbe ring walk with probe-mask MIP chain
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
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/screen_probes.hlsl
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Port screen_probes.hlsl:75–123 (Capsaicin's probe-mask MIP chain: start at highest mip, fall back) replacing the current Chebyshev ring walk at common/RadianceCacheCommon.hlsl:93–136 (radius=2 cap, line 91). Under paper-faithful 1× spawning the MIP chain is required; under our 2×2 sparse spawning ring=2 happens to cover the same worst-case, but the cost difference becomes load-bearing if spawn density changes.

Needs a new MIP-build pass (RadianceCacheProbeMaskMipPass.{cpp,h} + RadianceCacheProbeMaskMip.comp), following the precedent set by PTHashGridCacheMipCascadeBuild.comp.

Touched files:
- Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (FindClosestProbe rewrite)
- NEW: Source/Shaders/HLSL/RadianceCacheProbeMaskMip.comp
- NEW: Source/ExampleProject/RenderingClient/RadianceCacheProbeMaskMipPass.{cpp,h}
- Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands_GI.cpp (wire new pass before Raytracing)
- Source/ExampleProject/RenderingClient/ExampleRenderingClient_Setup.cpp (add to RasterizedGI dev toggle)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 FindClosestProbe ported to MIP-chain pattern matching screen_probes.hlsl:75–123
- [ ] #2 #2 New ProbeMaskMip pass + shader wired into ExecuteCommands_GI before RadianceCacheRaytracingPass
- [ ] #3 #3 Shader + engine build green
- [ ] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline
- [ ] #5 #5 Edge-of-screen probe lookups no worse than ring-walk baseline (capture screenshot diff)
- [ ] #6 #6 .alignments/TASK-226.5-port-audit.md cites Capsaicin line ranges
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
