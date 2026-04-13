---
id: TASK-18
title: >-
  Implement mesh LOD — render different mesh resolutions based on camera
  distance
status: To Do
assignee: []
created_date: '2026-04-13 08:05'
labels:
  - rendering
  - performance
  - geometry
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add a Level of Detail (LOD) system so that procedural and imported meshes use lower-resolution variants when far from the camera. This directly addresses visible edge artifacts on low-sector-count meshes at distance.

Possible approach:
- LOD descriptor on MeshComponent: array of (distance_threshold, MeshAsset) pairs
- DrawCallService selects LOD level per entity based on camera distance
- Template meshes could expose multiple LOD variants (e.g., Sphere with 32/16/8 sectors)
- Imported meshes: integrate Assimp's mesh simplification or generate offline LOD chains at bake time
<!-- SECTION:DESCRIPTION:END -->
