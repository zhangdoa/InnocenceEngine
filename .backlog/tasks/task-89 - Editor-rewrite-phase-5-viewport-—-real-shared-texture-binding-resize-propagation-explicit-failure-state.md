---
id: TASK-89
title: >-
  Editor rewrite phase 5: viewport — real shared-texture binding, resize
  propagation, explicit failure state
status: To Do
assignee: []
created_date: '2026-04-19 10:10'
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

- [ ] #1 On successful shared-texture bind, viewport renders engine output and resolution readout matches real dims
- [ ] #2 On bind failure, viewport shows explicit error state with reason + retry action (not a silent black box)
- [ ] #3 ResizeObserver-driven resize reaches the engine; either takes effect (TASK-73) or no-ops cleanly
- [ ] #4 Disconnect resets viewport to `pending` state; reconnect drives it back to `live` or `failed`
- [ ] #5 Playwright: viewport panel reports `live` or `failed` within 5s of engine start

Depends on TASK-86 (phase 2 IPC contract).
<!-- SECTION:DESCRIPTION:END -->
