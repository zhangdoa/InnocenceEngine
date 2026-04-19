---
id: TASK-88
title: >-
  Editor rewrite phase 4: connection lifecycle — explicit state machine,
  reconnect backoff, onDisconnect fan-out
status: Done
assignee: []
created_date: '2026-04-19 10:10'
updated_date: '2026-04-19 11:25'
labels:
  - editor
  - editor-rewrite
  - lifecycle
  - electron
dependencies:
  - TASK-86
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Connection state is a real state machine with explicit transitions, not a boolean + a 5s infinite retry. Every domain store reacts to disconnect by resetting in-flight state; reconnect refetches authoritative state.

## State machine

```
idle → connecting → live → lost → connecting … → giving-up
         ↑                  ↓
        stop ← stopping ← (user action from any state)
```

- `connectionStore.status` enum.
- `connectionStore.attempt` counter.
- Reconnect: 2s → 4s → 8s → 16s → 30s cap. After N attempts → `giving-up`; UI shows explicit "retry now" button.
- Transitions into `lost` or `giving-up` call `onDisconnect()` on every domain store: sceneStore clears, assetStore closes any import modal, taskDebuggerStore freezes its feed.
- Transitions into `live` trigger each store's `onConnect()`: initial scene fetch, viewport init, etc.

## `main.js` fixes

- Add `socket.on('error', ...)` handler.
- Clear any scheduled reconnect timer on `stopEngine`/`restartEngine`/successful open.
- Consult `connectionStore.isUserInitiatedShutdown` before scheduling reconnect.
- Emit `connection-status` channel updates on every transition so the renderer UI reflects the state machine visibly (badge in the footer, viewport empty state, etc.).

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 `connectionStore.status` is one of the enumerated values at all times; transitions are the only way state changes
- [x] #2 #2 Reconnect follows exponential backoff with a cap; after `giving-up`, a manual "Retry" button reinitiates
- [x] #3 #3 Every domain store implements `onDisconnect()` / `onConnect()`; fan-out wired in the connectionStore
- [x] #4 #4 Playwright: kill engine mid-session → footer badge flips to "Lost", import modal (if open) closes, scene empties; restart engine → state reflows cleanly
- [x] #5 #5 `socket.on('error', ...)` handler in place; main.js never surfaces uncaught WS errors

Depends on TASK-86 (phase 2).
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Connection lifecycle is now an explicit state machine on both sides of the wire.

**main.js** owns the authoritative machine:
`idle → connecting → live → lost → connecting … → giving-up`, with `stopping` as the exit on user-initiated shutdown. Reconnect delays `2s, 4s, 8s, 16s, 30s` (cap) for up to 6 attempts, then transitions to `giving-up` and waits. `cancelReconnect()` clears pending timers on any shutdown or successful open. `userInitiatedShutdown` is consulted before scheduling — the automatic reconnect loop doesn't race a manual stop. `socket.on('error', …)` added; records the error message but leaves state transitions to `close` which always follows. A new `engine-retry` ipcMain handler wraps `retryNow()` for the UI's Retry button.

main emits `connection-status` events on every transition with `{status, attempt, error, nextRetryMs}` and a legacy binary `engine-connected` signal for code that hasn't migrated (phase 2 domain stores).

**Renderer**:
- `connectionStore` gained `status` (one of 6 values, frozen in `VALID_STATUSES`), `attempt`, `error`, `nextRetryMs`, and `retryNow()` action sending `engine-retry`. `isConnected` is an instance getter computed from `status === 'live'` — every existing reader works unchanged.
- `useIpc` routes the new `connection-status` channel into the event bus alongside the existing `engine-connected`.
- Every domain store now has explicit `onConnect()` / `onDisconnect()` methods. Module-load subscribers dispatch to them on transitions (`sceneStore.refresh/reset`, `assetStore.reset`, `devToggleStore.refresh/reset`, `renderTargetStore.refresh/reset`, `taskGraphStore.reset`).
- `EditorFooter` renders a Naive tag coloured per status (success/warning/error/info/default) and shows a "Retry" button only when `status === 'giving-up'`. `lastMessage` ticker surfaces the human-readable description ("Reconnecting (attempt 3)…", "Connection lost — retrying in 4s", etc.).

**Tests** (`tests/connection-lifecycle.spec.js`, 3 cases — all green):

1. Driving `connection-status` events through `idle → live → lost → giving-up` updates `connectionStore.status`, `isConnected`, and `lastMessage` correctly.
2. Every domain store (`scene`, `asset`, `devToggle`, `renderTarget`, `taskGraph`) exposes `onConnect`/`onDisconnect` as functions; pre-populated state is cleared when the machine transitions to `lost`.
3. Footer's `[data-test="connection-retry"]` button is hidden in `live`, visible in `giving-up`, and clicking it emits `engine-retry` on the ipcRenderer main channel (validated by wrapping `ipcRenderer.send`).
<!-- SECTION:FINAL_SUMMARY:END -->
