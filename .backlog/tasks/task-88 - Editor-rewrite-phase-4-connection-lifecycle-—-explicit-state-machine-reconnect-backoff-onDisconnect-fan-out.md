---
id: TASK-88
title: >-
  Editor rewrite phase 4: connection lifecycle — explicit state machine,
  reconnect backoff, onDisconnect fan-out
status: To Do
assignee: []
created_date: '2026-04-19 10:10'
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

- [ ] #1 `connectionStore.status` is one of the enumerated values at all times; transitions are the only way state changes
- [ ] #2 Reconnect follows exponential backoff with a cap; after `giving-up`, a manual "Retry" button reinitiates
- [ ] #3 Every domain store implements `onDisconnect()` / `onConnect()`; fan-out wired in the connectionStore
- [ ] #4 Playwright: kill engine mid-session → footer badge flips to "Lost", import modal (if open) closes, scene empties; restart engine → state reflows cleanly
- [ ] #5 `socket.on('error', ...)` handler in place; main.js never surfaces uncaught WS errors

Depends on TASK-86 (phase 2).
<!-- SECTION:DESCRIPTION:END -->
