---
id: TASK-14
title: Fix GISponza curtains rotation — 90-degree axis misalignment
status: To Do
assignee: []
created_date: '2026-04-12 18:35'
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
- [ ] #1 Curtains align correctly with the Sponza main structure
- [ ] #2 Curtains use a dedicated TransformComponent (not shared with main Sponza)
- [ ] #3 GISponza.InnoScene updated to reference GISponza.Curtains.TransformComponent
<!-- AC:END -->
