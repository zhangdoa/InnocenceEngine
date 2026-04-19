---
id: TASK-82
title: >-
  Editor-Next: replace scene-mutation setTimeouts with a request/reply IPC
  convention
status: Done
assignee: []
created_date: '2026-04-19 09:56'
updated_date: '2026-04-19 10:11'
labels:
  - editor
  - ipc
  - robustness
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`src/store/sceneStore.js` lines ~50/60/66 schedule `GET_SCENE` 50ms after every create/delete/rename mutation to refresh the tree. On slow or busy engines this fires before the mutation commits, and rapid operations (create three entities in a row) stomp each other. Same symptom class in `panelStore.resetLayout` which closes panels then immediately re-adds them, racing dockview's async layout updates.

The underlying smell: every mutation site invents its own "send and hope" sequence with magic timeouts. The engine IPC dispatcher is a request/reply-capable router (TASK-62 AC #8 made it first-class); the editor just isn't using it like one.

## Scope

1. Define a convention: every mutation-class IPC (`CREATE_ENTITY`, `DELETE_ENTITY`, `RENAME_ENTITY`, `UPDATE_ENTITY_PROPERTY`) gets a correlation id and returns a `*_OK` or `*_ERR` reply. Alternatively, every mutation triggers an engine-pushed `SCENE_DATA` after it commits (no correlation id needed, but requires a single reply channel).
2. Delete the `setTimeout(..., 50)` refresh calls from `sceneStore.js`; refresh on the engine's committed reply / push.
3. Audit `panelStore.resetLayout` — either call the dockview api's `clear()` then re-add, or await layout changes before re-adding.

## Why not "just increase the timeout"

A hardcoded delay tuned for the fastest machine still races on the slowest; tuning for the slowest machine makes the UI feel mushy everywhere. Request/reply is the right primitive — mutation → commit → single source of truth → UI updates.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 No `setTimeout` calls on the mutation path in `sceneStore.js`; scene refresh happens on engine reply/push
- [ ] #2 #2 Rapid consecutive mutations (create three entities back-to-back) all show up in the outliner deterministically; no lost refreshes
- [ ] #3 #3 `panelStore.resetLayout` no longer races with dockview's internal layout updates
- [ ] #4 #4 Playwright regression: create-3-entities-in-200ms test asserts all three are visible in the hierarchy
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Superseded by TASK-86 (editor rewrite phase 2: IPC contract) and TASK-87 (scene vertical). The setTimeout-based refresh pattern is designed out of the new request/reply contract rather than patched.
<!-- SECTION:FINAL_SUMMARY:END -->
