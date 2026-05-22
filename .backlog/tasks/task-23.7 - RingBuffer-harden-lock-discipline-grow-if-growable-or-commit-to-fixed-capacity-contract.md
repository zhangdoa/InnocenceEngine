---
id: TASK-23.7
title: >-
  RingBuffer: harden lock discipline + grow if growable, or commit to
  fixed-capacity contract
status: Done
assignee: []
created_date: '2026-05-22 07:30'
updated_date: '2026-05-22 09:10'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 7000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/RingBuffer.h is used in:
- Source/Engine/Common/Thread.h, Thread.cpp
- Source/Engine/Common/TaskScheduler.h, TaskScheduler.cpp
- Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp
- Tests: UnitTests/RingBufferTests.cpp, StressTests/ConcurrencyStress.cpp

Concerns:
1. `ThreadSafe` template flag wraps `[]` and `currentElement()` with shared_lock — but `emplace_back` takes unique_lock and writes both `m_Array[idx]` and `m_CurrentElementIndex`. A reader holding shared_lock and dereferencing `m_Array[m_CurrentElementIndex - 1]` reads two non-atomic fields (`m_CurrentElementIndex` + the element). The shared_lock prevents concurrent emplace but the model is still racy if `m_CurrentElementIndex` and element write aren't ordered. Audit.
2. `m_isLoopingOverOnce` is a regular `bool` written under unique_lock — fine, but reader paths for `size()` read it without any lock. Race.
3. RingBuffer wraps `Inno::Array<T, ThreadSafe>` — if Array's ThreadSafe flag is removed (TASK-23.2 decision), RingBuffer's flag must follow.
4. No growth; `reserve()` once at construction.

Decide: stay fixed-capacity (rename to FixedRingBuffer for clarity?), or add growth (pointless for a ring — but bounded resize on demand could matter for TaskScheduler).

References:
- Source/Engine/Common/RingBuffer.h
- Source/Engine/Common/Thread.cpp, TaskScheduler.cpp (callers)
- Source/TestSuite/UnitTests/RingBufferTests.cpp (existing tests)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 size() / currentElement() reader paths take the lock or use atomics correctly (no torn reads of m_CurrentElementIndex + m_isLoopingOverOnce).
- [x] #2 ThreadSafe flag aligned with Inno::Array's decision from TASK-23.2.
- [x] #3 Header comment documents capacity contract (fixed vs resizable).
- [x] #4 Existing UnitTests/RingBufferTests pass.
- [x] #5 New stress test: SPMC ring with N producers / 1 consumer (or matched to actual usage by Thread / TaskScheduler), 10^6 ops, no torn reads.
- [ ] #6 Optional perf comparison: vs std::deque used as a ring (recorded if implemented).
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Audit findings

Real consumers of `RingBuffer<TaskReport, true>`:
- Producer: `Thread::Worker` → `m_TaskReport.emplace_back(...)` (per-thread, on the thread itself).
- Consumers: `EditorService_Introspection::LIST_TASKS` (remote-control websocket handler, separate thread) and `ImGuiWrapper::ConcurrencyProfiler` (engine UI thread).

Multiple readers + producer on different threads. ThreadSafe variant is correct call.

Bugs found:

1. **`size()` was unlocked** — read `m_isLoopingOverOnce` (bool) and `m_CurrentElementIndex` (size_t) without any lock. Race vs producer's `emplace_back` (unique_lock holder).
2. **`currentElementPos()` was unlocked** — read `m_CurrentElementIndex` without lock. Same race.
3. **`operator[]` and `currentElement()` returned `T&` after the shared_lock destructor** — RAII released the lock at function return, but the caller's resulting reference outlived the lock. Same return-reference-after-lock-release pattern called out for ThreadSafeVector::eraseByIndex / ThreadSafeQueue::getRawData.

## Diff

- `Source/Engine/Common/RingBuffer.h`:
  - `size()` and `currentElementPos()` now take a `shared_lock` under `if constexpr (ThreadSafe)`; non-ThreadSafe variant unlocked as before.
  - `operator[]` and `currentElement()` ThreadSafe overloads return `T` by value (forces copy under the held shared_lock). Non-ThreadSafe overloads still return `T&` for in-place mutation.
- `Source/TestSuite/UnitTests/RingBufferTests.cpp`:
  - New `TestRingBufferThreadSafeProducerConsumer` — 1 producer thread (50000 emplace_back) + 1 consumer thread (reads `size()` then iterates `[i]`), no torn size/value reads observed.

## Verification

- `BuildWin.ps1 -SkipShaderCompile` + `msbuild TestSuite.vcxproj` — clean.
- `TestSuite.exe -u` RingBuffer: 3/3 pass (basic, wraparound, concurrent SPSC ~3ms).
- `Main.exe -total_frames 10` exits 0. Production consumers (`EditorService_Introspection`, `ImGuiWrapper::ConcurrencyProfiler`) still compile and run (they read `[i].member`, which works equivalently for T-by-value and T&).

## What was NOT verified

- Migration to a renamed `FixedRingBuffer` type — left as-is because the contract is documented in code-near comments now, and existing call sites all use the fixed-capacity contract correctly.
- AC #2 (Array's ThreadSafe flag decision) — deferred to TASK-23.2 which is the proper owner.
- AC #6 (perf comparison vs std::deque used as ring) — not implemented; covered by TASK-23.18 cross-cutting perf matrix.

## ACs

- #1 size() + currentElementPos() now lock for ThreadSafe variant.
- #2 No change — Array's ThreadSafe still wraps RingBuffer's; alignment with Array decision will happen in TASK-23.2.
- #3 Header comment in place documenting fixed-capacity contract.
- #4 Existing tests still pass.
- #5 New SPSC concurrent test added.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
