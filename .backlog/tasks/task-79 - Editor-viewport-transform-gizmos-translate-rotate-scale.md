---
id: TASK-79
title: Editor viewport transform gizmos (translate / rotate / scale)
status: To Do
assignee: []
created_date: '2026-04-19 11:30'
labels:
  - editor
  - viewport
  - gizmo
  - world-editing
dependencies:
  - TASK-62
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
TASK-62 AC #7 is half-done: entity create / rename / delete + per-property edits round-trip through `UPDATE_ENTITY_PROPERTY` and the existing Save button rewrites the scene. The remaining piece — transform-handle gizmos in the viewport — is its own substantial slice with its own moving parts and gets a dedicated task.

## Scope

- Render translate / rotate / scale gizmos as a viewport overlay on the currently selected entity. Either an engine-side debug-line pass that the editor toggles, or an editor-side WebGL overlay sitting on top of the shared-texture viewport — pick whichever lets the editor own the picking math.
- Mouse picking for the handles: hit-test on the editor side against the gizmo's projected screen rects.
- Handle drag math for each operation, with X / Y / Z axis-locked + screen-space variants.
- Round-trip the resulting transform delta to the engine via the existing `UPDATE_ENTITY_PROPERTY` IPC (already supports `pos` and `scale`; rotation needs a quat path on the engine side).
- Persist via the existing Save button (no new save IPC needed).
- Snap-to-grid + numeric step modifier (Shift / Ctrl) — nice to have, not required for the first cut.

## Non-goals

- Multi-select gizmos (select-and-drag-many) — single selection only for v1.
- Pivot mode toggles, parent-space vs world-space transforms — single mode (world-space) for v1.
- Undo / redo — separate task.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Selecting an entity in the Outliner shows a translate gizmo at its world position.
- [ ] #2 Dragging a translate handle on any axis updates the entity's `TransformComponent.m_LocalPos` in real time, with the change persisted on `Save`.
- [ ] #3 Toolbar / hotkey to switch between translate / rotate / scale gizmos.
- [ ] #4 Engine `UPDATE_ENTITY_PROPERTY` accepts a `rot` quaternion payload (current TODO in EditorService).
- [ ] #5 Playwright spec: select entity, drag translate-X handle by N pixels, assert the entity's `pos.x` advanced.
<!-- AC:END -->
