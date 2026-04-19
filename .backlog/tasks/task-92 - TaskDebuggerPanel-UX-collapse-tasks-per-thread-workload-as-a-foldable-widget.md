---
id: TASK-92
title: 'TaskDebuggerPanel UX: collapse tasks, per-thread workload as a foldable widget'
status: To Do
assignee: []
created_date: '2026-04-19 16:26'
labels:
  - editor
  - task-debugger
  - ux
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Current panel renders every task report inline — the per-task bars dominate the layout and in the narrow left-column slot (280px) they become unreadable. At-a-glance what users want is thread workload; per-task detail is drill-down.

## Proposed shape

- **Default**: one row per worker thread showing its workload (e.g. total cycles consumed, active vs idle ratio, last-N-frame sum bar). No per-task bars visible.
- **Expand**: row folds out into a detail widget showing the task list with their duration bars, same info the current panel shows.
- Collapsed state is the default and remembered in `uiStore` so reloads don't lose the user's drill-down state.

## Acceptance Criteria

- [ ] #1 Panel renders a compact per-thread workload row by default (~24px tall per thread)
- [ ] #2 Clicking a thread row expands to show the full per-task detail
- [ ] #3 Expanded/collapsed state persists across editor reload
- [ ] #4 Workload metric visible per thread (cycles consumed or % active) — picked deliberately, not inherited from the raw report dump
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
