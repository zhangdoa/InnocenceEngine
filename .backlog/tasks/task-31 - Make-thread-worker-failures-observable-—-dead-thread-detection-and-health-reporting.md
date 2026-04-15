---
id: TASK-31
title: >-
  Make thread worker failures observable — dead thread detection and health
  reporting
status: To Do
assignee: []
created_date: '2026-04-13 18:13'
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
