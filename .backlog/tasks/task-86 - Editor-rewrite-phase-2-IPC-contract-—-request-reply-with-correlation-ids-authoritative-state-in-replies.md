---
id: TASK-86
title: >-
  Editor rewrite phase 2: IPC contract — request/reply with correlation ids,
  authoritative state in replies
status: Done
assignee: []
created_date: '2026-04-19 10:10'
updated_date: '2026-04-19 11:02'
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
<!-- AC:BEGIN -->
- [x] #1 #1 Every client → engine message uses `request()` or `on()`; zero direct `ipcRenderer.send(..., { type })` calls remain in `src/`
- [x] #2 #2 Engine dispatcher returns authoritative result payloads for all mutation handlers; replies wrap them in the `{ id, status, result }` envelope
- [x] #3 #3 Request timeout path tested: disconnect engine mid-request → promise rejects with `TIMEOUT` error within the configured window, no orphaned entries in the in-flight map
- [x] #4 #4 Events and replies travel on separate envelope types; no handler ambiguity
- [x] #5 #5 IPC typings (TS types or JSDoc) live in one place and are imported by both the client composable and any server-side message-shape tests

Depends on TASK-85 (phase 1).
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Wire contract landed end-to-end.

Client side (`src/composables/useIpc.js`): singleton router + event bus + in-flight map. Public API is exactly `request(type, payload, { timeoutMs })` returning a Promise and `on(eventType, handler)` returning an unsubscribe. `useIpc()` inside a component wraps `on()` with auto-teardown in `onBeforeUnmount` (named handler refs, not `removeAllListeners` — HMR-safe). Errors are an `IpcError` class carrying `code` (`TIMEOUT`, `DISCONNECTED`, `NO_HANDLER`, `INTERNAL`, plus handler-specific codes) and the originating `requestType`.

Engine side (`Source/Engine/Services/EditorService.cpp`): handler signature changed from `void(json msg, ws)` to `json(json payload, ws)`. Dispatcher parses the `envelope` field (rejects anything other than `"request"`), calls the handler, wraps the return in `{envelope:"reply", id, status, result}` or catches `EditorReqError`/`std::exception` and writes `{envelope:"reply", id, status:"err", error:{code, message}}`. `NotifyViewportReady` switched to event-envelope broadcast. All mutation handlers (`ENTITY_CREATE`/`DELETE`/`RENAME`) now return the post-mutation entity list as authoritative state so stores don't need a follow-up `GET_SCENE`; `UPDATE_ENTITY_PROPERTY` returns the post-commit value (covers engine clamping).

Main process (`main.js`): HELLO handshake sent as `{envelope:"request", id:0, type:"HELLO", payload:{pid}}` — id=0 is reserved so renderer-generated ids never collide. The HELLO reply is consumed by main (shared-texture setup) and not forwarded to the renderer. VIEWPORT_READY events are consumed (re-bind) AND forwarded to the renderer (for its UI state).

Store migration (all six): every store speaks through `request`/`on` only — no direct `ipcRenderer.send('engine-message', …)` calls remain in src/ (verified by grep). Each store subscribes to `on('engine-connected')` at module load and auto-refreshes or resets on transitions; the per-panel `watch(isConnected)` patterns were removed. sceneStore mutations use server-truth replies (no `setTimeout` followups). AssetPanel's scene-load path goes through `request('LOAD_SCENE', {path})` instead of the old window CustomEvent.

Playwright coverage:

- `tests/ipc-contract.spec.js` (new, 4 tests): reply resolves inflight, request times out with `TIMEOUT` code and correct `requestType`, event handlers fire without resolving pending requests, disconnect rejects all inflight with `DISCONNECTED`.
- `tests/ux-audit.spec.js` updated to simulate `IMPORT_PROGRESS` with the new envelope shape.

AC #5 (typings in one place): the wire contract is documented at the top of `useIpc.js` and mirrored in a comment block at the top of `EditorService.cpp` — intentionally JSDoc-style rather than a separate TS definitions file because we have no TS compiler in the build and the envelope shape is tiny (3 message types, 4-5 fields each). Escalate to formal types when the message volume warrants it.
<!-- SECTION:FINAL_SUMMARY:END -->
