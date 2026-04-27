---
id: TASK-172
title: 'TASK-168-B: Editor inspector render-pass bypass checkboxes + IPC (editor-tooling-expert)'
status: To Do
assignee: []
created_date: '2026-04-26'
labels:
  - rendering
  - tooling
  - diagnostic
  - editor
dependencies:
  - TASK-171
parent_task_id: TASK-168
priority: high
references:
  - Source/Editor-Next/src/components/inspector/
  - Source/Engine/Services/EditorIPCService.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask B of TASK-168 (runtime render-pass bypass toggle).** Owner: `editor-tooling-expert`.

User-facing surface. Binds the `IRenderPass::m_Bypassed` field (TASK-171) to an editor inspector panel so toggles are flippable at runtime without editing source.

### What this subtask delivers

1. **New editor panel** "Render Pass Bypass" (or extend an existing diagnostic panel — `RTDebugger` or `RenderToggles` are the closest neighbours). Lists every pass returned by TASK-171's `GetDispatchedPasses()` entry point, one checkbox per pass. The checkbox label is the pass instance name.

2. **IPC GET** — fetch the current pass list + bypass state from the engine. Per the TASK-101 IPC contract (setter-reply symmetry, mutations return read-back state).

3. **IPC UPDATE** — toggle a pass's `m_Bypassed`. Engine acknowledges with the new state; editor does not optimistically update — wait for the read-back. (Per `feedback_silent_failures.md`: if the engine refuses the toggle for any reason, the editor must surface that, not silently revert.)

4. **Persistence per-session** — the bypass state lives in the engine for the lifetime of the run; no scene-file persistence (per parent task spec). When the user reloads a scene, bypass state survives.

### Project invariants (anchor — read before implementing)

- **TASK-101 IPC contract**: GET returns engine-truth, UPDATE returns engine-truth (not the request payload). No client-side optimism.
- **Editor-Next ownership**: this subtask owns `Source/Editor-Next/src/components/inspector/` and any IPC-handler additions in `Source/Engine/Services/EditorIPCService.cpp` for the bypass message types.
- **Pass-listing source of truth**: TASK-171 exposes the list via `ExampleRenderingClient::GetDispatchedPasses()`. Do not duplicate the list in the editor — fetch on panel open.

### What this subtask does NOT do

- No engine-side bypass logic (that's TASK-171).
- No texture-viewer / pass-output visualisation.
- No keyboard shortcut for toggle cycling — checkbox UI only is sufficient per parent task spec.

### Validation

- Editor builds clean (Vue lint + symmetry test).
- Round-trip test: toggle a pass via UI, observe engine log line ("RenderPass: <Name> bypass = ON"), toggle again, observe OFF. Reload scene, confirm state survives.
- AC #5 of parent TASK-168: bypass `PointShadowGeometryProcessPass` + `SunShadowRTPass`, run GISponza, observe perf delta. (This validates the editor toggle works end-to-end as a diagnostic.)

### Peer review

Per `peer-review-required.md`: after editor-tooling-expert implements, dispatch a second agent (rendering-researcher is the natural reviewer — they implemented the engine-side surface and will catch contract-mismatch) to peer-review before commit.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Editor inspector panel lists all dispatched render passes from TASK-171's entry point
- [ ] #2 Per-pass checkbox toggles m_Bypassed via IPC; UI reflects engine read-back, not optimistic state
- [ ] #3 IPC GET / UPDATE follow TASK-101 setter-reply symmetry contract
- [ ] #4 Bypass state persists across scene reload within the same engine session
- [ ] #5 End-to-end validation: bypassing PointShadowGeometryProcessPass + SunShadowRTPass on GISponza shows expected perf delta (parent AC #5)
- [ ] #6 Editor symmetry test (`Source/Editor-Next/tests/entity-property-symmetry.spec.js` or equivalent) covers the new IPC messages
- [ ] #7 Peer review by rendering-researcher before commit
<!-- AC:END -->
