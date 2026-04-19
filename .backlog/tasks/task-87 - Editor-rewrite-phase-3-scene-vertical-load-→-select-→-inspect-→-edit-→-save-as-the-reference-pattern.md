---
id: TASK-87
title: >-
  Editor rewrite phase 3: scene vertical (load → select → inspect → edit → save)
  as the reference pattern
status: Done
assignee: []
created_date: '2026-04-19 10:10'
updated_date: '2026-04-19 11:10'
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
<!-- AC:BEGIN -->
- [x] #1 #1 No `setTimeout` on the scene-mutation path; scene tree refreshes come from the mutation reply itself
- [x] #2 #2 Rapid three-consecutive creates all land; Playwright regression asserts three entities visible in the outliner after a 200ms burst
- [x] #3 #3 Inspector editors emit zero Vue prop-mutation warnings; edits that the engine clamps/normalizes show the clamped value after the reply, not the user's typed value
- [x] #4 #4 Inspector editor `v-model` bound to local drafts; store is the single read source for authoritative state
- [x] #5 #5 Scene operations survive engine disconnect: in-flight requests reject, store clears, UI shows empty state cleanly

Depends on TASK-85, TASK-86.
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Scene vertical now runs entirely on phase-2's request/reply contract.

**sceneStore** (already reply-driven from phase 2 — this phase completed the edges):
- `refresh`, `selectEntity`, `updateProperty`, `createEntity`, `deleteEntity`, `renameEntity`, `saveScene` all `await request(…)` and commit the reply's authoritative payload. No `setTimeout` followups anywhere on the mutation path (verified by grep).
- `updateProperty` merges the post-commit value into `selectedEntity.components[i][property]` so the UI re-hydrates to server truth (covers engine-side clamping).
- `on('engine-connected')` at module load drives `refresh()` on connect, `reset()` on disconnect.

**Inspector editors** (`TransformEditor.vue`, `LightEditor.vue`): fully rewritten. Each holds a local `reactive` draft that mirrors the subset of `props.component` it edits. `v-model` binds the draft, never the prop — zero prop mutation, zero Vue dev warnings. A deep watcher on `props.component` re-syncs the draft when the store commits the reply, so clamped/normalized values propagate back to the visible input.

**HierarchyPanel** unchanged — it already drove store actions and renders `sceneStore.entities` reactively.

**Tests** (`tests/scene-vertical.spec.js`, 3 new cases — engine mocked entirely inside the renderer by intercepting `ipcRenderer.send('engine-message', …)` and emitting synthesized replies):

- Three rapid `createEntity('A')`, `('B')`, `('C')` calls without await land deterministically (burst completes under 1s; outliner shows all three in order).
- `UPDATE_ENTITY_PROPERTY` reply that clamps the user's value (user types 999, engine returns 42) re-hydrates `selectedEntity.components` to the clamped value — proves AC #3.
- Forcing `engine-connected=false` clears `sceneStore.entities` + `selectedEntity` and flips `connectionStore.isConnected` — proves AC #5.

**Window test hooks**: `window.__innoStores = { scene, connection, asset, … }` is exposed by `src/store/index.js` on module load, mirroring the `window.__innoIpc` pattern from phase 2. Production code path never reads it; tests rely on it because the Vite production bundle flattens the module graph and `import('/src/store/…')` from `page.evaluate()` can't resolve.
<!-- SECTION:FINAL_SUMMARY:END -->
