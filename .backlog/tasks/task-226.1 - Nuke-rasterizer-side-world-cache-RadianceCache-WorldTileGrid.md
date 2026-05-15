---
id: TASK-226.1
title: Nuke rasterizer-side world cache (RadianceCache* WorldTileGrid)
status: To Do
assignee: []
created_date: '2026-05-15 20:23'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
  - nuke
dependencies: []
references:
  - .alignments/TASK-226-gap-matrix.md
  - Build/GI1_0.pdf
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/hash_grid_cache.hlsl
parent_task_id: TASK-226
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Remove the WorldTileGrid hash-grid bindings/types from the rasterizer-side radiance cache. The direction key uses surface normal (DominantAxis(normalWS) at RadianceCacheRayGen.hlsl:391) while Capsaicin uses incoming-ray-direction quantized to 64 buckets — same divergence class flagged in TASK-77.1's audit. PT-side (TASK-77.x) already has the paper-faithful hash grid. The rasterizer-side duplicate is structurally wrong AND obsolete under PT-primary. Rasterizer ClosestHit falls back to sky-NEE + previous-frame light-pass only.

Per TASK-226 gap matrix row #8 (broken/nuke). User explicitly authorized nuking broken-from-prior-attempts code.

Touched files:
- Source/Shaders/HLSL/RadianceCacheRayGen.hlsl (lines 382–460 write path)
- Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl (lines 176–209 read path)
- Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl
- Source/Shaders/HLSL/RayTracingTypes.hlsl (WorldTileGrid type)
- Any binding/PSO declarations referencing WorldTileGrid
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 WorldTileGrid type + all bindings removed from rasterizer-side radiance cache shaders
- [ ] #2 #2 Shader build green (CompileHlslShaders.vcxproj)
- [ ] #3 #3 Engine + editor build green (ALL_BUILD.vcxproj)
- [ ] #4 #4 Runtime smoke: launch GISponza autotest, no validation errors, indirect lighting still present from prior-frame readback (not from the now-nuked WorldTileGrid)
- [ ] #5 #5 No reference in code/comments to the nuked WorldTileGrid types
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
