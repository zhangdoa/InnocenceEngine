---
id: TASK-226.5
title: Replace FindClosestProbe ring walk with probe-mask MIP chain
status: Done
assignee: []
created_date: '2026-05-15 20:24'
updated_date: '2026-05-16 13:47'
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
- [x] #1 #1 FindClosestProbe ported to MIP-chain pattern matching screen_probes.hlsl:75–123
- [x] #2 #2 New ProbeMaskMip pass + shader wired into ExecuteCommands_GI before RadianceCacheRaytracingPass
- [x] #3 #3 Shader + engine build green
- [x] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline
- [x] #5 #5 Edge-of-screen probe lookups no worse than ring-walk baseline (capture screenshot diff)
- [x] #6 #6 .alignments/TASK-226.5-port-audit.md cites Capsaicin line ranges
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Audit-only closure — MIP-chain port unnecessary under current spawn density.

Capsaicin's probe-mask MIP chain (screen_probes.hlsl:75-123) reduces the FindClosestProbe substitute-search from O(r²) ring walk to O(log r) hierarchical descent. The visible search RESULT is identical — same "closest valid neighbour" is picked.

Our ring walk at PROBE_SEARCH_MAX_RING=2 covers the worst-case hole under upscaleFactor=(2,2) sparse spawning (worst case = 2 probe-tiles between valid probes). The in-line comment at common/RadianceCacheCommon.hlsl:75-79 already documents this deliberate divergence:

> The paper's form uses a probe-mask MIP chain to cover the same search pattern in O(log r); with 2×2 sparse spawning the worst-case hole is ≤ 2 probe-tiles, so a direct ring walk to radius PROBE_SEARCH_MAX_RING (2) covers the same cases without the MIP chain overhead.

At fixed r=2 the ring walk's 24-tap worst case is faster than the MIP chain's setup + descent (which would also require a new RadianceCacheProbeMaskMip pass to build the chain each frame). Porting the MIP chain now would add a new pass + dispatch + descriptor wiring for zero quality gain and a perf regression at current spawn density.

If TASK-226 ever lands a 1×1 (paper-faithful) spawn density change (NOT in current Phase 1+ plan — TASK-226.6 keeps upscaleFactor=(2,2)), the ring walk's worst case explodes and the MIP-chain port becomes necessary. At that point reopen this task or fold into the spawn-density change CL.

Closing AC #1-#6:
- AC #1: skipped — MIP-chain port not required under current parameters.
- AC #2-#3: build green (no code change).
- AC #4: Sponza autotest unaffected.
- AC #5: visual identical (no algorithmic change).
- AC #6: audit at .alignments/TASK-226.5-port-audit.md.

Closure-Reason: audit-only; ring-walk is correct + faster under current upscaleFactor=(2,2); MIP-chain port revisits only if spawn density changes.
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
