---
id: TASK-92
title: 'TaskDebuggerPanel UX: collapse tasks, per-thread workload as a foldable widget'
status: Done
assignee:
  - editor-tooling-expert
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

- [x] #1 Panel renders a compact per-thread workload row by default (~24px tall per thread)
- [x] #2 Clicking a thread row expands to show the full per-task detail
- [x] #3 Expanded/collapsed state persists across editor reload
- [x] #4 Workload metric visible per thread (cycles consumed or % active) — picked deliberately, not inherited from the raw report dump
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — `npm run build` clean (vite v5.4.21, 4157 modules, 9.10s)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — `tests/task-debugger.spec.js` (live engine, 1 passed in 5.0s); `tests/theme-reactivity.spec.js`, `tests/ipc-contract.spec.js`, `tests/scene-vertical.spec.js` (mock, 9 passed in 26.2s) — uiStore is shared so I re-ran the broad mock suite to catch shape regressions
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — `tests/task-debugger-collapse.spec.js`, 3 tests, all passed in 19.0s. These are mock-based by necessity: the engine returns its real worker-thread count (host-dependent), so the assertion "N rows for N worker threads" cannot be deterministic without a synthesized payload. The live-engine spec `task-debugger.spec.js` continues to validate the live wiring end-to-end.
- [x] #4 Self-authored mock-based tests are not the sole validation — `task-debugger.spec.js` was re-run live against Main.exe and asserts the panel renders post-redesign with real worker threads (see #2)
- [x] #5 User-observable outcome verified — screenshot under `Build/captures/task-92/panel-expanded.png` shows 4 collapsed rows with workload bars + Thread 1 expanded showing per-task duration bars; DOM snapshots under the same dir
- [x] #6 Final summary lists what was NOT verified — see Implementation Notes
<!-- DOD:END -->

## Implementation Notes

**Workload metric chosen: total cycles (sum of per-task durations in the buffered slice), in ms.** Justification: `% active` would require dividing total task time by a wall-clock window (`max(finishTime) - min(startTime)`), but the ring buffer can hold reports that span seconds (idle thread) or microseconds (busy thread), so the denominator changes meaning across rows and conflates "no tasks scheduled" with "tasks finished fast." Cycles is unit-stable and directly answers "what is this thread doing right now": idle threads register near-zero, saturated threads dominate the bar. The bar scales against the busiest thread per render so idle rows visibly empty out.

**Persistence shape**: `uiStore.taskDebuggerExpanded` is a `{ "<index>": true }` map (absent key = collapsed; that's the default per the brief). Persisted under `editor-next.taskDebugger.expanded`; readback at module load is defensive (corrupt JSON → empty map, not a thrown init).

**Drive-by correctness fix in the panel I rewrote**: the old `ticksToMs = ticks/10000` assumed 100ns Win32 QPC ticks, but `Engine/Common/Thread.cpp` stamps reports with `Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond)` — confirmed in `Timer.cpp`. Corrected divisor to `1000`. Pre-existing displayed durations were 10× too small; new totals (e.g. 0.33 ms in the screenshot) reflect the actual µs→ms conversion.

**Files changed**:
- `Source/Editor-Next/src/components/TaskDebuggerPanel.vue` — full rewrite: collapsed rows + foldable per-task detail, chevron, scaled workload bar, total ms, fixed µs→ms conversion.
- `Source/Editor-Next/src/store/uiStore.js` — `taskDebuggerExpanded` map + getter/setter/toggle, hydrate on init, persist via deep watcher.
- `Source/Editor-Next/tests/task-debugger-collapse.spec.js` — new spec (3 tests).
- `Source/Editor-Next/tests/task-debugger-snapshot.spec.js` — one-shot capture helper that produces `Build/captures/task-92/{collapsed-default.html,thread-1-expanded.html,panel-expanded.png}`.

**Not verified**:
- Behaviour with N=0 worker threads (engine always has at least one; not exercised in either test).
- The unrelated pre-existing renderer warning `Cannot set properties of undefined (setting 'threads')` from `taskGraphStore.js` line 33's `TASK_GRAPH_FRAME` handler — observed on live-engine connect both before and after my change. Out of scope for TASK-92; left for a separate task if it ever stops being benign.
- Visual behaviour at the 280px column width specifically — the screenshot was captured at default ~480px panel width. Grid template is `14px 80px 1fr 72px` so the workload bar shrinks gracefully, but a narrow-column spec wasn't authored.
- `editor.spec.js` (full hierarchy smoke test) is currently failing at HEAD on the `Main Camera/` entity assertion — confirmed pre-existing by re-running on `git stash`'d tree. Not introduced by this change.
