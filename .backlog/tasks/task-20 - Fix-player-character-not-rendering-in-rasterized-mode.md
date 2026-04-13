---
id: TASK-20
title: Fix player character not rendering in rasterized mode
status: To Do
assignee: []
created_date: '2026-04-13 08:05'
labels:
  - rendering
  - bug
  - rasterizer
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
A cube (likely the Player Character entity using UnitSquareMesh) is visible at the center of the scene in path tracer mode but renders incorrectly in rasterized mode. In rasterized mode it appears as a visible cube at the origin rather than a flat quad, suggesting either the wrong mesh is being used or the draw call is submitting the wrong geometry.

Investigate:
- Confirm which mesh the Player Character entity actually renders (UnitSquareMesh is a flat quad, not a cube)
- Check whether the DrawCallService submits the player entity in rasterized mode and what geometry it resolves to
- Verify that EDITOR_MODE (FPS, non-TP) suppresses player character rendering or renders it correctly
- Check if there is a fallback to UnitCubeMesh when mesh loading fails
<!-- SECTION:DESCRIPTION:END -->
