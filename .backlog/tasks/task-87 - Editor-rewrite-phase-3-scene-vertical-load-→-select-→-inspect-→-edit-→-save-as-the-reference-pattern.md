---
id: TASK-87
title: >-
  Editor rewrite phase 3: scene vertical (load → select → inspect → edit → save)
  as the reference pattern
status: To Do
assignee: []
created_date: '2026-04-19 10:10'
labels:
  - editor
  - editor-rewrite
  - scene
  - inspector
dependencies:
  - TASK-85
  - TASK-86
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Rebuild the scene-editing vertical cleanly on top of the phase-1 theme foundation and phase-2 IPC contract. This slice becomes the reference every other panel mimics.

## Shape

- **`sceneStore`**: reactive state owned by the store alone — UI reads, never writes directly. Actions:
  - `load(path)` → `request("LOAD_SCENE", { path })` → reply carries the full entity tree → store commits.
  - `select(id)` → `request("GET_ENTITY_DETAILS", { id })` → reply carries the component snapshot → store commits to `selectedEntity`.
  - `updateProperty({ entityId, component, path, value })` → `request("UPDATE_ENTITY_PROPERTY", { … })` → reply carries the **post-commit** value (engine may clamp) → store commits that, not the optimistic UI value.
  - `create / rename / delete` → each replies with the updated scene tree; no follow-up `GET_SCENE` round-trip.
  - `save(path)` → flushes any pending mutations, then `request("SAVE_SCENE", { path })`.
  - `onDisconnect()` → clears `entities`, `selectedEntity`.

- **Inspector editors** (`TransformEditor`, `LightEditor`, any others): bind `v-model` to a **local draft** that starts from `props.component`. Commits flow through `sceneStore.updateProperty`; the UI re-hydrates from the store's `selectedEntity` after the reply lands. Zero prop mutation, zero direct writes to props.

- **HierarchyPanel**: renders `sceneStore.entities`; select/create/rename/delete call store actions; context menu works against the authoritative reply, not against an optimistic pre-commit state.

## What gets deleted

- Every `setTimeout(..., 50)` in `sceneStore.js`.
- Direct `component.pos[i] = …` / `props.component.color = …` mutations.
- Any ad-hoc `ipcRenderer.send(..., 'GET_SCENE')` follow-up calls.

## Acceptance Criteria

- [ ] #1 No `setTimeout` on the scene-mutation path; scene tree refreshes come from the mutation reply itself
- [ ] #2 Rapid three-consecutive creates all land; Playwright regression asserts three entities visible in the outliner after a 200ms burst
- [ ] #3 Inspector editors emit zero Vue prop-mutation warnings; edits that the engine clamps/normalizes show the clamped value after the reply, not the user's typed value
- [ ] #4 Inspector editor `v-model` bound to local drafts; store is the single read source for authoritative state
- [ ] #5 Scene operations survive engine disconnect: in-flight requests reject, store clears, UI shows empty state cleanly

Depends on TASK-85, TASK-86.
<!-- SECTION:DESCRIPTION:END -->
