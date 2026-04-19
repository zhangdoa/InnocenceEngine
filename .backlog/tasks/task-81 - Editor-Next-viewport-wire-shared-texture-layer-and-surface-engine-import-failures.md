---
id: TASK-81
title: >-
  Editor-Next viewport: wire shared-texture layer and surface engine-import
  failures
status: To Do
assignee: []
created_date: '2026-04-19 09:56'
labels:
  - editor
  - viewport
  - ipc
  - robustness
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`src/components/ViewportPanel.vue` is a placeholder today: it renders a canvas and a "DX12 SHARED TEXTURE MODE / RESOLUTION: 1280x720" decoration but doesn't actually bind the shared texture or respond to resize. Meanwhile `main.js` (Electron main) imports the engine's shared texture via Electron's sharedTexture API and calls `sendSharedTexture` on the renderer frame, with a two-shape fallback and a `console.error` if both paths fail — but the renderer gets no signal.

Result today: if the shared-texture import fails in prod, `isConnected` flips true, the "Waiting for Engine" overlay disappears, and nothing renders — the user sees a blank pane with no explanation. If import succeeds, the decoration lies ("1280x720" is hardcoded regardless of the real back-buffer size).

## Scope

1. **Wire the viewport layer.** Whichever of `sendSharedTexture` succeeded in `main.js`, forward a `VIEWPORT_READY { width, height }` IPC to the renderer. `ViewportPanel` listens, stores dims in a ref, and binds/resizes the canvas accordingly.
2. **Forward failures.** If both shared-texture shapes fail in `main.js`, send `VIEWPORT_FAILED { reason }` to the renderer. `ViewportPanel` shows an explicit error empty-state (Naive `NEmpty` with a message) instead of a blank black box.
3. **Handle resize.** Listen for dockview `onDidLayoutChange` or a resize observer on the canvas element, send `VIEWPORT_RESIZE { width, height }` to main, and the engine swap chain follows.
4. **Update the decoration.** Replace the hardcoded "1280x720" with the real dims from step 1; or drop the decoration if it's not load-bearing.

## Acceptance Criteria

- [ ] #1 On successful shared-texture bind, viewport receives `VIEWPORT_READY` and renders engine output; resolution readout matches real dims
- [ ] #2 On shared-texture bind failure, viewport shows an explicit error state (not a blank black box)
- [ ] #3 Resizing the dock panel propagates to the engine's back-buffer (feature-gated on TASK-73 if needed)
- [ ] #4 Playwright smoke: viewport panel is present and reports either ready or failed within 5s of engine start
<!-- SECTION:DESCRIPTION:END -->
