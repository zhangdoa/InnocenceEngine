---
id: TASK-14
title: Fix GISponza curtains rotation — 90-degree axis misalignment
status: Done
assignee: []
created_date: '2026-04-12 18:35'
updated_date: '2026-04-12 23:28'
labels:
  - assets
  - scene
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The NewSponza_Curtains_glTF model appears rotated incorrectly when loaded in GISponza — likely 90 degrees off on one axis. Both the main Sponza and the curtains use the same `GISponza.Sponza.TransformComponent` (identity transform), but their coordinate conventions may differ.

**Investigation:** Load GISponza, inspect curtains placement in RenderDoc or visually. Determine which axis needs 90° rotation. Create a dedicated `GISponza.Curtains.TransformComponent.json` with the corrected rotation and update GISponza.InnoScene to use it for the SponzaCurtains entity instead of sharing `GISponza.Sponza.TransformComponent`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Curtains align correctly with the Sponza main structure
- [x] #2 Curtains use a dedicated TransformComponent (not shared with main Sponza)
- [x] #3 GISponza.InnoScene updated to reference GISponza.Curtains.TransformComponent
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Root cause: curtain mesh vertices are exported by the 3ds Max/babylon.js pipeline in Z-up local space (fabric lies flat, Z = curtain height). The Assimp importer does not apply glTF node transforms to vertex data, so curtains appeared as flat horizontal panels. Fix: dedicated GISponza.Curtains.TransformComponent.json with -90° X rotation (quaternion X=-0.7071068, W=0.7071068) converts Z-height to Y-height. Vertex AABB Y=[0.45, 4.02] after rotation spans arch height correctly. Curtains are now visible in the archways. Integration test exit 0. Note: a proper root-cause fix (aiProcess_PreTransformVertices in Assimp importer) would allow per-node orientations to be preserved — tracked as a separate improvement.
<!-- SECTION:FINAL_SUMMARY:END -->
