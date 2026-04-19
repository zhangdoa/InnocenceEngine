---
id: TASK-80
title: >-
  TaskScheduler self-submission deadlock — sub-tasks picking the caller thread
  spin forever
status: To Do
assignee: []
created_date: '2026-04-19 09:44'
labels:
  - scheduler
  - concurrency
  - correctness
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

`TaskScheduler::Submit` → `Thread::AddTask` needs the target thread's state to be Idle (or Waiting, if frozen) to CAS it into Busy and push to `m_TaskList`. The CAS is there to prevent racing the Worker's iteration of `m_TaskList`.

When a task running on worker W calls Submit, and `GenerateThreadIndex` randomly picks W itself (or picks a thread whose owner is also spinning on the same issue), `AddTask` spins forever: W is held in Busy by the outer task, and the outer task is blocked inside `AddTask` waiting for W to become Idle. Deadlock.

This surfaced during TASK-70 Axis 2 when the -bake file loop submitted its outer ImportSync as a scheduler task, and ProcessAssimpScene inside that task submitted mesh/material sub-tasks — some of which hit the same worker. Worked around in TASK-70 by moving the outer bake loop to `std::thread` so the orchestrator is off the worker pool, but the scheduler itself still has this contract gap.

## Why this is structural, not a bug patch

The scheduler's public API doesn't announce "Submit must not be called from inside a running task," and there's nothing that enforces it. Future nested-submission call sites (path tracing, any fan-out from a worker) will hit the same trap. The fix belongs at the scheduler layer, not at each caller.

## Options

1. **Snapshot `m_TaskList` before iteration.** Worker copies the list locally, then iterates the snapshot. AddTask pushes directly to the real list under a mutex (no CAS dance). Straightforward; small per-iteration copy cost.
2. **Lock-free MPMC queue** for the Worker's task input. More complex but higher throughput; probably overkill for current load.
3. **Thread-local current-worker-index** + GenerateThreadIndex excludes it. Avoids the exact self-submission case but doesn't help when two mutually-blocked workers happen to pick each other.

Option 1 is the smallest structural change that actually makes nested Submit safe across the board.

## Acceptance Criteria

- [ ] #1 A task running on worker W can call `TaskScheduler::Submit(...)` and the submitted task runs (regardless of which thread index Submit picks), without any "Adding task … is taking longer than expected" warnings
- [ ] #2 Worker's iteration of its task list is safe against concurrent AddTask pushes (no iterator invalidation)
- [ ] #3 Add a stress test: a task that spawns N sub-tasks and Waits on all of them, run from inside a worker — must complete without the timeout warnings TASK-70 originally tripped
- [ ] #4 TASK-70's `-bake` loop can revert to TaskScheduler::Submit for the outer file tasks (confirm no regression in wall-clock)
<!-- SECTION:DESCRIPTION:END -->
