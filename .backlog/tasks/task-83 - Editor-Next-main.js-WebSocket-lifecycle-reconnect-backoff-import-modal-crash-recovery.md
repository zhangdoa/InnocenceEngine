---
id: TASK-83
title: >-
  Editor-Next main.js: WebSocket lifecycle, reconnect backoff, import-modal
  crash recovery
status: To Do
assignee: []
created_date: '2026-04-19 09:57'
labels:
  - editor
  - electron
  - ipc
  - robustness
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Several correctness holes in `main.js`'s engine-connection lifecycle, surfaced by the Editor-Next audit.

1. **No `socket.on('error', ...)` handler** — WS errors from engine-side crashes surface as uncaught Node error events. Noisy at minimum, can crash main at worst.
2. **Reconnect racing exit** — `close` schedules a 5s reconnect iff `engineProcess` is non-null. The `exit` handler nulls `engineProcess` later. If `close` fires before `exit`, a reconnect is scheduled against a dead PID; if after, no reconnect on crash. Plus: `connectionStore.isUserInitiatedShutdown` is set by `restartEngine` but never consulted here.
3. **No reconnect backoff** — on persistent failure, main retries every 5s forever. Add exponential backoff capped at 30s, plus a max-attempts ceiling that surfaces a UI-level "give up" state.
4. **Import modal can't be closed on engine crash** — `ImportModal.vue` visibility is gated on `assetStore.isImporting`. If the engine dies mid-import no `IMPORT_FINISHED` arrives and the modal is stuck. Reset `assetStore.isImporting = false` on any disconnect/crash and add a timeout + manual cancel.
5. **`AssetPanel.vue` native `fs.readdirSync` / `statSync` has no try/catch** — a broken symlink or permission-denied entry kills the whole listing with an uncaught error. Also assumes `baseDir = __dirname/../../../../Data`, which breaks when Vite bundles.

## Acceptance Criteria

- [ ] #1 `socket.on('error', ...)` handler in place; logs and does not crash main
- [ ] #2 Reconnect consults `isUserInitiatedShutdown` before scheduling, and the reconnect timer is cleared on `stopEngine`/`restartEngine`/successful connect
- [ ] #3 Reconnect uses exponential backoff (5s → 10s → 20s → 30s cap) and emits a `connection-status` channel update after N failed attempts so the renderer can show an explicit "offline — retry?" state
- [ ] #4 On any disconnect, `assetStore.isImporting` is reset and `ImportModal` hides or shows a failure state
- [ ] #5 `AssetPanel` wraps `fs.readdirSync`/`statSync` in try/catch, skips failing entries, and resolves `baseDir` via an IPC call to main (or an Electron `app.getAppPath`-based resolver that survives bundling)
<!-- SECTION:DESCRIPTION:END -->
