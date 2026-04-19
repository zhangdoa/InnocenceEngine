---
id: TASK-84
title: >-
  Editor-Next inspector: stop mutating props directly in TransformEditor /
  LightEditor
status: To Do
assignee: []
created_date: '2026-04-19 09:57'
labels:
  - editor
  - vue
  - correctness
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`src/components/inspector/TransformEditor.vue` binds `component.pos[i]` / `component.scale[i]` (directly a prop) to `v-model:value` on Naive UI inputs. Vue 3 fires a prop-mutation warning, and if the engine never echoes back the change, the displayed value drifts from server truth.

Same pattern in `src/components/inspector/LightEditor.vue` (`props.component.color = ...`).

The correct shape: writes go through `sceneStore.updateProperty({ entityId, component, path, value })`, which sends the IPC; the next `ENTITY_DETAILS` message hydrates `selectedEntity`, and the inspector re-renders from canonical state. No direct prop writes.

## Acceptance Criteria

- [ ] #1 No prop mutations in `TransformEditor.vue` or `LightEditor.vue`; Vue dev warnings gone on property edit
- [ ] #2 Editor values re-hydrate from `ENTITY_DETAILS` after a commit — the displayed value matches server truth even if the engine clamps or normalizes
- [ ] #3 Playwright: edit a transform X value, Save, reload the entity, assert the persisted value matches what was displayed
<!-- SECTION:DESCRIPTION:END -->
