---
id: TASK-100
title: >-
  Editor workspace (AssetPanel): populate content; scene load from panel must
  work
status: To Do
assignee: []
created_date: '2026-04-19 18:13'
labels:
  - editor
  - bug
  - workspace
  - assets
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The center workspace panel (`AssetPanel.vue`) is currently empty — no folders, no files, no asset list rendered. At minimum it must enumerate `Data/` and let the user load `.InnoScene` files by double-click (the wiring for this already exists in the component).

## Likely causes to triage

- `baseDir = path.resolve(__dirname, '../../../../Data')` — resolves relative to the editor source dir. Works in dev mode, may break in a packaged build or if `__dirname` points somewhere different post-bundle.
- `window.require('fs')` — only works with `nodeIntegration: true` + `contextIsolation: false`. If Electron's webPreferences changed in a recent refactor, `fs` is undefined and `loadDirectory` silently returns.
- The onMounted `if (window.require)` guard means failures are silent. Should log / message when it can't resolve.

## Scope

- Fix the empty-panel bug so the `Data/` tree actually lists.
- Make the failure mode loud (`n-empty` with a diagnostic message or a console warn) instead of silent.
- Confirm dblclick `.InnoScene` still triggers `sceneStore.loadScene` end-to-end against the running engine.
- Playwright regression: open editor → assert AssetPanel lists at least one item (any `Data/` child); assert dblclick on a `.InnoScene` triggers SCENE_UPDATED round-trip.

## Out of scope (separate tasks)

- Asset thumbnails / previews
- Drag-to-scene
- Import pipeline UX polish

## Acceptance

- AssetPanel renders `Data/` contents on mount.
- Scene load via dblclick in the panel works against a live engine.
- Silent failures replaced with a visible diagnostic.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
