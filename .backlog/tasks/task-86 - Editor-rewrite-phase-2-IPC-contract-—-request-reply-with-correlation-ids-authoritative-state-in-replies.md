---
id: TASK-86
title: >-
  Editor rewrite phase 2: IPC contract — request/reply with correlation ids,
  authoritative state in replies
status: To Do
assignee: []
created_date: '2026-04-19 10:10'
labels:
  - editor
  - editor-rewrite
  - ipc
dependencies:
  - TASK-85
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

Replace the current "send an IPC and schedule `GET_SCENE` 50ms later" pattern with a proper request/reply contract. Authoritative state ships in the reply; UI never guesses what the server committed.

## Protocol

Every message is one of three shapes:

```
REQUEST  { id, type, payload }            // client → engine
REPLY    { id, status, result|error }     // engine → client (paired by id)
EVENT    { type, payload }                // engine → client (out-of-band, no id)
```

- `id` is a monotonic client-generated integer.
- `REPLY.status` is `"ok"` or `"err"`.
- `REPLY.result` carries authoritative state for mutation requests (e.g. `CREATE_ENTITY` reply returns the updated scene tree or at least the new entity plus its id; `UPDATE_ENTITY_PROPERTY` reply returns the committed value after any engine-side clamping/normalization).
- `EVENT` is for things the engine pushes without a client prompt (import progress, task-graph frames, viewport resize ack).

## Client side

- `useIpc.js` becomes: `request(type, payload) → Promise<result>` + `on(eventType, handler) → unsubscribe`. That's the whole API.
- A single in-flight map `{ id → resolve/reject }` times out after N seconds; rejects with a specific error code.
- No `ipcRenderer.send(..., { type })` scattered around components. All go through `request` / `on`.
- HMR-safe: the composable tears down its listeners in `onBeforeUnmount` via named handler refs, not `removeAllListeners`.

## Engine side

- `EditorService` dispatcher already exists (TASK-62 AC #8). Extend it so every handler returns a `REPLY.result` payload matching the contract, and mutation handlers explicitly include post-commit state in the reply.
- Add a reply pump that writes `{ id, status, result }` onto the same WebSocket.
- Out-of-band pushes go through an `event(...)` helper so they never collide with the reply channel.

## Acceptance Criteria

- [ ] #1 Every client → engine message uses `request()` or `on()`; zero direct `ipcRenderer.send(..., { type })` calls remain in `src/`
- [ ] #2 Engine dispatcher returns authoritative result payloads for all mutation handlers; replies wrap them in the `{ id, status, result }` envelope
- [ ] #3 Request timeout path tested: disconnect engine mid-request → promise rejects with `TIMEOUT` error within the configured window, no orphaned entries in the in-flight map
- [ ] #4 Events and replies travel on separate envelope types; no handler ambiguity
- [ ] #5 IPC typings (TS types or JSDoc) live in one place and are imported by both the client composable and any server-side message-shape tests

Depends on TASK-85 (phase 1).
<!-- SECTION:DESCRIPTION:END -->
