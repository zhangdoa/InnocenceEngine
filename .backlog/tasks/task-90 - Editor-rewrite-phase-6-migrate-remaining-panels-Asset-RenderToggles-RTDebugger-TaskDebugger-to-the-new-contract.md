---
id: TASK-90
title: >-
  Editor rewrite phase 6: migrate remaining panels (Asset, RenderToggles,
  RTDebugger, TaskDebugger) to the new contract
status: To Do
assignee: []
created_date: '2026-04-19 10:10'
labels:
  - editor
  - editor-rewrite
dependencies:
  - TASK-85
  - TASK-86
  - TASK-87
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Every remaining panel uses the phase-1 theme tokens, the phase-2 IPC contract, the phase-3 store pattern, and the phase-4 connection lifecycle. No `setTimeout`, no direct `ipcRenderer.send(..., { type })`, no prop mutation, no hardcoded colors.

## Per-panel work

- **AssetPanel**: replace the native `fs.readdirSync`/`statSync` with an IPC-fetched listing (`request("LIST_ASSETS", …)`); wrap any residual fs in try/catch. Drop the `baseDir` that dies in bundled builds. Import flow goes through `assetStore` actions with reply-driven progress (not the current `IMPORT_PROGRESS` event dance that can strand the modal).
- **RenderTogglesPanel**: subscribes via `on("LIST_PASSES", …)` on connect; toggle emits `request("SET_PASS_ENABLED", …)`; store mirrors engine truth from the reply.
- **RTDebuggerPanel**: same shape — populate from `request("LIST_RENDER_TARGETS")`, select via `request("SET_SWAPCHAIN_SOURCE", …)`, commit authoritative selection from the reply.
- **TaskDebuggerPanel**: `on("TASK_GRAPH_FRAME", …)` events with a configurable sample rate set via `request("SET_TASK_GRAPH_RATE", …)`.
- **HierarchyPanel / Inspector / EditorHeader / EditorFooter**: audit for any remaining hardcoded colors or direct-prop mutations that phase 3 didn't touch.
- **panelStore.resetLayout**: replace the close-then-add dance with `api.clear()` + re-add, or await the close event properly.

## Acceptance Criteria

- [ ] #1 No `ipcRenderer.send(..., { type })` calls remain in `src/`; everything uses `useIpc().request` / `.on`
- [ ] #2 No hardcoded color hex / `rgba(white/black,…)` / `#fff` / `#000` etc. in any `.vue` or `.js` under `src/` (viewport canvas background excepted and documented)
- [ ] #3 No `setTimeout` on any mutation path
- [ ] #4 Every panel implements `onDisconnect` / `onConnect` via its store
- [ ] #5 `panelStore.resetLayout` no longer races dockview layout updates
- [ ] #6 Playwright smoke + ux-audit green across all flavors for every panel

Depends on TASK-85, TASK-86, TASK-87.
<!-- SECTION:DESCRIPTION:END -->
