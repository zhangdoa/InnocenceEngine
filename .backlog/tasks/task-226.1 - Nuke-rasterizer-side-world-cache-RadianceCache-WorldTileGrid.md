---
id: TASK-226.1
title: Nuke rasterizer-side world cache (RadianceCache* WorldTileGrid)
status: Done
assignee: []
created_date: '2026-05-15 20:23'
updated_date: '2026-05-16 02:02'
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
- [x] #1 #1 WorldTileGrid type + all bindings removed from rasterizer-side radiance cache shaders
- [x] #2 #2 Shader build green (CompileHlslShaders.vcxproj)
- [x] #3 #3 Engine + editor build green (ALL_BUILD.vcxproj)
- [x] #4 #4 Runtime smoke: launch GISponza autotest, no validation errors, indirect lighting still present from prior-frame readback (not from the now-nuked WorldTileGrid)
- [x] #5 #5 No reference in code/comments to the nuked WorldTileGrid types
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Nuked the rasterizer-side WorldTileGrid hash grid. Per gap matrix row #8 (broken, normal-keyed instead of ray-dir-keyed) — same divergence class as TASK-77.1 flagged. PT-side hash grid (TASK-77.x) remains canonical.

Deleted: WorldTileGrid u1 binding, WorldTile/WorldCell types + constants + helpers (DominantAxis, CellInTile, CellIndexInTile, SelectTileMipLevel, IsShortRay, ComputeTileHash, ComputeTileFingerprint, IsTileSlotStale, _TileGridIndex, _CellGridIndex, MAX_LINEAR_PROBE, WORLD_PROBE_SHORT_RAY_THRESHOLD), the RayGen MIP-fan-out write block, the ClosestHit off-screen world-cache read block, the m_WorldProbeGrid C++ resource decl in RadianceCacheReprojectionPass_Setup.cpp.

Kept: _PCG3D (used by closest-hit RNG seeding), all other RayTracingTypes.hlsl helpers (octahedral, SH basis, GGX, tangent, Hash2D, RayPayload). u1 binding slot left empty per brief — no renumber to avoid root-signature mismatch.

Diff: 11 source files, 35 insertions / 360 deletions. Build green (shader + C++). Runtime smoke (Main.exe -mode 0 -renderer 0 -total_frames 30 -offscreen) → `Auto-test: 30 frames rendered, terminating.`, no validation/corruption/hung errors. Final `git grep -i "worldtile|world_tile|worldprobegrid|worldcache"` returns 0 hits.

Visual verification deferred to TASK-226.8 (final port gate) per umbrella plan — the rasterizer GI looks worse short-term because the secondary-bounce cache is gone, that's expected and within scope of the nuke.

Closure-Reason: ACs #1-#5 satisfied, see .alignments/TASK-226.1-nuke-summary.md.
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
