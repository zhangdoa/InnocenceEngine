---
id: TASK-89
title: >-
  Editor rewrite phase 5: viewport — real shared-texture binding, resize
  propagation, explicit failure state
status: Done
assignee: []
created_date: '2026-04-19 10:10'
updated_date: '2026-04-19 11:33'
labels:
  - editor
  - editor-rewrite
  - viewport
dependencies:
  - TASK-86
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Goal

The viewport panel actually displays the engine frame, resizes with its dock pane, and tells the user explicitly when the shared-texture bind fails instead of going blank.

## Scope

- **main.js**: on successful `sendSharedTexture`, emit `VIEWPORT_READY { width, height }` on the renderer IPC channel. On failure of both shape attempts, emit `VIEWPORT_FAILED { reason }`.
- **ViewportPanel.vue**: subscribe to both via `useIpc().on(...)`. Local state: `pending | live | failed`. Rendering:
  - `pending`: "Waiting for Engine" empty-state (existing look).
  - `live`: canvas bound, resolution readout reflects real dims from the event (not hardcoded).
  - `failed`: explicit error empty-state with the reason and a "Retry" action that asks main to re-import.
- **Resize**: ResizeObserver on the canvas parent, debounced 150ms, sends `request("VIEWPORT_RESIZE", { width, height })`. Engine reply confirms the new back-buffer size; canvas width/height attributes update from the reply. (If TASK-73 swap chain resize isn't done, this degrades to a no-op reply and the canvas stays at the original size — still better than today's lie.)
- **Teardown**: on `onDisconnect` (phase 4 hook), state returns to `pending`.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 On successful shared-texture bind, viewport renders engine output and resolution readout matches real dims
- [x] #2 #2 On bind failure, viewport shows explicit error state with reason + retry action (not a silent black box)
- [x] #3 #3 ResizeObserver-driven resize reaches the engine; either takes effect (TASK-73) or no-ops cleanly
- [x] #4 #4 Disconnect resets viewport to `pending` state; reconnect drives it back to `live` or `failed`
- [x] #5 #5 Playwright: viewport panel reports `live` or `failed` within 5s of engine start

Depends on TASK-86 (phase 2 IPC contract).
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Viewport now has an honest state machine end-to-end.

**main.js** owns the wire contract to the renderer. The engine's internal `VIEWPORT_READY` event (carrying the shared handle) is consumed but NOT forwarded — main drives its own `setupSharedTexture` and emits one of two synthesized events on the engine-message channel based on outcome:

- `{envelope:'event', type:'VIEWPORT_READY', payload:{width, height}}` on a successful bind (either primary or fallback shape).
- `{envelope:'event', type:'VIEWPORT_FAILED', payload:{reason, width, height}}` on both shapes failing — the `reason` carries a combined "primary: …; fallback: …" error message.

**ViewportPanel.vue** holds three-state (`pending` / `live` / `failed`) local state. Subscribes to both events through useIpc's event bus, plus the `engine-connected` bus for disconnect→pending. The `data-test-status` attribute on the panel exposes the state for Playwright. In each state:

- `pending`: Naive `NEmpty` "Waiting for engine viewport…"
- `live`: canvas shows engine output; stats overlay displays `WAITING`/`LIVE`/`FAILED` status and `W × H` resolution from the event.
- `failed`: `NEmpty` with the reason and a "Retry" button that sets state back to `pending` and sends `engine-retry` via ipcRenderer (reuses TASK-88's manual-retry channel).

**Resize**: a `ResizeObserver` on the viewport container debounces layout changes 150ms and fires `request('VIEWPORT_RESIZE', { width, height })`. The engine handler echoes the requested dims with `applied: false` (the swap-chain resize itself is TASK-73). `NO_HANDLER` / `DISCONNECTED` errors are swallowed; other failures log without throwing.

**Tests** (`tests/viewport-wiring.spec.js`, 3 cases — all green):

1. Panel starts in `pending`; after `VIEWPORT_READY` with `{1920, 1080}` it flips to `live` and the resolution readout reads "1920 × 1080".
2. `VIEWPORT_FAILED` flips to `failed` with the reason visible; the Retry button appears; clicking it sends `engine-retry` on the Electron main channel and flips the panel back to `pending`.
3. Forcing `engine-connected=false` after going `live` drops the panel back to `pending` (proves the connection-disconnect → pending fan-out from phase 4 reaches the viewport).
<!-- SECTION:FINAL_SUMMARY:END -->
