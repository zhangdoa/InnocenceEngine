---
id: TASK-61
title: >-
  Draw culling drops ground mesh at low camera pitch (both UnitTest landscape
  and Sponza)
status: Done
assignee: []
created_date: '2026-04-18 09:51'
updated_date: '2026-04-18 15:08'
labels:
  - bug
  - culling
  - rendering
  - frustum
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/OpaqueCullingPass.cpp
  - Source/ExampleProject/RenderingClient/SunShadowCullingPass.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User report (2026-04-18): when the camera pitches down past a certain angle, the ground mesh vanishes from rendering. Reproduces in both scenes — UnitTest's landscape box and Sponza's floor disappear the same way.

**Hypothesis:** The `OpaqueCullingPass` uses a frustum test against per-mesh AABB. For large, flat meshes (a single-box landscape or Sponza floor) the AABB may intersect the far plane or a camera-frustum plane in a way that yields a false negative when the view direction becomes grazing. Candidate causes:
- Degenerate AABB bounds (e.g. ground mesh's bounding box has `minY == maxY`, making the culler treat it as outside the frustum after near/far clip comparison).
- Frustum plane extraction bug (wrong normal direction or plane equation).
- Wrong world transform applied to the AABB before the test.

**Next investigation steps:**
1. Log the culled-mesh list each frame with entity names; see exactly when ground disappears.
2. Print ground's computed world-space AABB and the six frustum planes at the failing camera pose — check which plane rejects it.
3. Repro with a simpler cuboid of known size to narrow scene-data vs code.
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fixed in commit 44be771d `fix(culling): clip-space plane test handles AABBs that straddle near plane (TASK-61)` — rewrote the culling test to stay in clip space (valid for any sign of w) so AABBs that cross the near plane no longer false-negative from a perspective-divide sign flip. Status file was stale.
<!-- SECTION:FINAL_SUMMARY:END -->
