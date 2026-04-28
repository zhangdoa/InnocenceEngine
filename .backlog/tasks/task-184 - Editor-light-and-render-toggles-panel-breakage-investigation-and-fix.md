---
id: TASK-184
title: 'Editor light + render-toggles panel breakage — investigation and fix'
status: To Do
assignee: []
created_date: '2026-04-28 17:30'
labels:
  - editor
  - bug
  - diagnostic
dependencies: []
priority: high
references:
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
  - Source/Engine/Services/EditorService.cpp
  - Source/Engine/Services/DevToggleRegistry.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"i can't tweak the light properties, the panel doesn't work somehow."*

Editor's light-property panel (and possibly the render-toggles panel) does not work. Symptom unclear without runtime evidence — could be: IPC handler regression, Vue component rendering error, sceneStore/devToggleStore desync, or naive-ui binding issue.

### Why high priority

Without working editor panels, every property tweak (light intensity / color / position, render-pass bypass, debug visualization mode) requires source edit + rebuild. This is the same diagnostic-cost-of-30s-rebuild gap that TASK-183 addresses for visualization, but for *property authoring*.

### Investigation directions

1. **Confirm symptom**: launch editor, open Sponza, click a light entity. Does the LightEditor panel render? Does the field for `intensity` / `color` / `castShadow` show? Does typing in a field do anything? Does the engine view update?
2. **IPC trace**: editor's `EntityProperty` panel issues `GET_ENTITY_DETAILS` and `UPDATE_ENTITY_PROPERTY` per TASK-101 contract. If either fails or returns wrong shape, the panel can silently no-op.
3. **devToggleStore vs sceneStore**: are they both functional, or is one broken? The render-toggles panel uses devToggleStore; the light editor uses sceneStore. If both broken: framework-side. If one: per-feature.
4. **Recent commits to editor**: `a544eb40` (workspace + outliner) was last editor commit; before it, `2eefa0f8` (TASK-149 castShadow + IPC). Both touched IPC paths. Recent regression possible.

### What this delivers

1. Reproduction — exact symptom captured (Playwright spec or screen recording).
2. Root cause identified with cited file:line.
3. Fix landed; both panels operate correctly: light properties round-trip; render-toggles flip GPU state.
4. Regression test added to `Source/Editor-Next/tests/` covering the failure shape.

### Owner

`editor-tooling-expert` (Vue + IPC-Node side). Coordinate with `software-architect` if engine-side IPC handler is at fault.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Symptom reproduced; root cause file:line identified
- [ ] #2 Fix lands; light editor round-trips position/color/intensity/castShadow
- [ ] #3 Render-toggles panel flips GPUPathTracer + RasterizedGI live (no engine restart)
- [ ] #4 Regression Playwright spec added covering the failure shape
- [ ] #5 Live-engine spec passes
- [ ] #6 Peer review per discipline
<!-- AC:END -->
