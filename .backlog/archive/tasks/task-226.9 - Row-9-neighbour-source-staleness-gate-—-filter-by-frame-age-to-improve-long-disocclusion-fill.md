---
id: TASK-226.9
title: >-
  Row #9 neighbour-source staleness gate — filter by frame age to improve
  long-disocclusion fill
status: To Do
assignee: []
created_date: '2026-05-16 21:07'
labels:
  - rendering
  - GI
  - radiance-cache
  - followup
dependencies: []
references:
  - .alignments/TASK-226.4-port-audit.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/master/src/core/src/render_techniques/gi1/gi1.comp#L780
parent_task_id: TASK-226
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Followup from TASK-226.4 (commit e0e68900) divergence (b). The Row #9 3×3 neighbour-probe fallback reads `in_RadianceCacheResults_Prev` + `in_ProbePosition` + `in_ProbeNormal` at neighbour positions, filtering with validity proxy `nCellRadiance.w > 0` (`Source/Shaders/HLSL/RadianceCacheReprojection.comp:203`). Under `upscaleFactor=(2,2)` (`RayTracingTypes.hlsl:13`), 3 of every 4 probe tiles per spawn tile carry multi-frame-stale positions — only one per spawn tile is refreshed per frame via RayGen (`RadianceCacheRayGen.hlsl:135-136`). The validity proxy does NOT filter by age, so on long camera-cut disocclusions the fallback can pull from neighbours whose probe position is several frames stale, producing visible quality loss vs Capsaicin's aged `g_ScreenProbes_ProbeCachedTileBuffer`.

Candidate gates (pick after measuring): plane-consistency `abs(dot(nProbePos - positionWS, normalWS)) < cellSize`, normal-dot threshold (already partly used at line 196), or an explicit per-tile age counter sourced from a new history buffer.

Touched files (likely):
- Source/Shaders/HLSL/RadianceCacheReprojection.comp (Row #9 validity check)
- Source/Shaders/HLSL/common/RadianceCacheReprojection.hlsl (if staleness logic is helper-able)
- Possibly a new per-probe age buffer if proxy-only gates prove insufficient.

Reopen condition: TASK-226.8 visual gate observes a measurable long-disocclusion artifact that this work would fix.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Row #9 fallback rejects multi-frame-stale neighbour probes via the chosen gate (plane-consistency / age counter / etc.)
- [ ] #2 Long-disocclusion camera-cut test case on Sponza renders without the staleness ghost artifact identified in TASK-226.4 review
- [ ] #3 Shader + engine build green
- [ ] #4 Decision (proxy-gate-suffices vs new-age-buffer-needed) recorded in alignment artifact
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
