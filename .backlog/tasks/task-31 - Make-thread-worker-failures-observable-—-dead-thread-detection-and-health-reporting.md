---
id: TASK-31
title: >-
  Make thread worker failures observable — dead thread detection and health
  reporting
status: Done
assignee: []
created_date: '2026-04-13 18:13'
updated_date: '2026-04-17 04:45'
labels:
  - architecture
  - reliability
  - threading
  - observability
dependencies: []
references:
  - Source/Engine/Common/Thread.cpp
  - Source/Engine/Common/Thread.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Before the try-catch added in c52cbed4, an unhandled C++ exception in Thread::Worker would propagate up and kill the worker thread silently. The engine would then stall or produce incorrect output with no log, no error, and no indication that a worker thread had died. The try-catch is a minimum fix; it does not address observability of thread health over time.

**Structural weakness:** The thread pool has no mechanism to detect dead or degraded workers and no way to surface their status to the engine or operator. A thread that exits unexpectedly is simply gone — tasks assigned to it will never execute, and nothing alerts the frame loop that capacity has been lost.

**Target improvement:**
- Track each Thread's health state (alive / exception-failed / released) and expose it via an engine stats query
- In TaskScheduler or its equivalent, detect when all available threads are in a failed state and log a fatal error rather than hanging indefinitely
- Consider restarting worker threads on exception (with a backoff and max-restart limit) so a single bad task doesn't permanently reduce thread pool capacity
- Expose thread health in any future debug/profiling overlay
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):**

- Added `State::Failed` enum value. Transitioning to `Failed` means the Worker body itself (not a per-task execution) raised an unhandled exception; the thread exits its loop, logs at Error, and GetState() now visibly reports the condition.
- Wrapped the entire `Thread::Worker` body in an outer try/catch. Per-task `try/catch` inside the task loop was already present (commit c52cbed4); the outer catch covers anything escaping the task iteration itself (m_TaskList iteration, state machine, allocators).
- Added `std::atomic<uint64_t> m_CaughtExceptionCount`. The per-task catch blocks now increment it and include the cumulative count in their log message, so a noisy task lineage becomes visible in the logs without any tooling.
- Exposed `Thread::GetCaughtExceptionCount() const` for a future `TaskScheduler` health probe or debug overlay to consume.

Scope deliberately does not include thread restart on exception or auto-escalation to fatal when all workers are Failed — those need a TaskScheduler-level owner that knows the worker count and restart policy, which is bigger than this CL. Logging state + cumulative exception count is the observability layer that unblocks those future decisions.

**Validation:** Build clean. RenderTest exit 0. Integration run on the 10-frame UnitTest→GISponza auto-test finishes with 0 `cumulative exceptions` and 0 `died from` lines, confirming the healthy path is unchanged.
<!-- SECTION:NOTES:END -->
