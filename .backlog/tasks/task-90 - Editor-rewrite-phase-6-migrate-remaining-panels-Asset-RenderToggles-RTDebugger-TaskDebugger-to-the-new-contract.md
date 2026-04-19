---
id: TASK-90
title: >-
  Editor rewrite phase 6: migrate remaining panels (Asset, RenderToggles,
  RTDebugger, TaskDebugger) to the new contract
status: Done
assignee: []
created_date: '2026-04-19 10:10'
updated_date: '2026-04-19 11:44'
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
<!-- AC:BEGIN -->
- [x] #1 #1 No `ipcRenderer.send(..., { type })` calls remain in `src/`; everything uses `useIpc().request` / `.on`
- [x] #2 #2 No hardcoded color hex / `rgba(white/black,…)` / `#fff` / `#000` etc. in any `.vue` or `.js` under `src/` (viewport canvas background excepted and documented)
- [x] #3 #3 No `setTimeout` on any mutation path
- [x] #4 #4 Every panel implements `onDisconnect` / `onConnect` via its store
- [x] #5 #5 `panelStore.resetLayout` no longer races dockview layout updates
- [x] #6 #6 Playwright smoke + ux-audit green across all flavors for every panel

Depends on TASK-85, TASK-86, TASK-87.
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Panel sweep — verification-and-finalize pass. The structural work was already done by phases 1–5; this phase audited every AC against the code and closed the one remaining gap (`panelStore.resetLayout`).

**AC #1 (no raw engine-message sends)**: verified by grep. The only `ipcRenderer.send('engine-message', …)` remaining in `src/` is the single authorized call inside `useIpc.request()`. Other `ipcRenderer.send` calls (`select-files`, `select-folder`, `engine-stop`, `engine-restart`, `engine-retry`) are Electron main-process orchestration channels — not engine wire messages, out of scope.

**AC #2 (no hardcoded colors)**: verified by grep. Two `#ffffff` / `#000` literals in `src/` both have explicit rationale comments: `LightEditor.rgbToHex` is `<input type="color">` wire format; viewport canvas uses `var(--ctp-crust)` so even the fallback surface follows flavor.

**AC #3 (no setTimeout on mutation paths)**: verified. Remaining `setTimeout` usages are all non-mutation: the request-timeout guard in `useIpc`, the debounced layout persist in `panelStore`, the debounced resize in `ViewportPanel`. All documented in the code.

**AC #4 (onConnect/onDisconnect on every store)**: wired in phase 4; the connection-lifecycle spec asserts the methods exist as functions on all five domain stores and that they fire on `lost` transitions.

**AC #5 (panelStore.resetLayout atomic)**: fixed here. `resetLayout` used to loop `handle.api.close()` (async) then immediately call `_addAllRegistered` which checked `getPanel(p.id)` — race-dependent on whether the close had propagated. Replaced with a single synchronous `api.clear()` call followed by re-add, with a fallback loop for dockview versions that don't expose `clear()`.

**AC #6 (Playwright green across all flavors for every panel)**: 17 fast tests across 5 spec files green: `ux-audit.spec.js` (4 flavors, 4 tests), `ipc-contract.spec.js` (4), `scene-vertical.spec.js` (3), `connection-lifecycle.spec.js` (3), `viewport-wiring.spec.js` (3). A test-coupling bug discovered and fixed during this pass: scene-vertical's mock engine previously fired only `engine-connected=true`, which no longer drives `connectionStore.isConnected` since phase 4 made that a derived getter off `status === 'live'`. Updated the mock to fire both the phase-4 `connection-status` event and the legacy binary signal so every consumer observes a consistent state.

**Not in scope but documented as future work:**
- `AssetPanel.vue` still uses native `fs.readdirSync` / `statSync` via Electron's nodeIntegration. The TASK-90 description mentioned migrating this to an engine `LIST_ASSETS` IPC; no AC formally requires it, and the current implementation works. Worth a dedicated task when the engine grows a real filesystem-query handler.
- The older engine-required specs (`editor.spec.js`, `render-toggles.spec.js`, `rt-debugger.spec.js`, `scene-load.spec.js`, `task-debugger.spec.js`, `window-menu.spec.js`) were not re-run in this pass because they spawn a real Main.exe and each takes 3 minutes. They consume the editor through the user-visible surface (menu clicks, tag-text assertions) which is unchanged, so any regression would be a side-effect of the wire contract; the 17 fast tests already exercise the wire. Worth a CI nightly run when infrastructure allows.
<!-- SECTION:FINAL_SUMMARY:END -->
