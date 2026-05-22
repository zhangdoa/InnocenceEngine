---
id: TASK-23.18
title: >-
  TestSuite: cross-cutting parity — every foundation feature has unit/regression
  + smoke/stress + perf-vs-STL coverage
status: To Do
assignee: []
created_date: '2026-05-22 07:35'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 18000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Cross-cutting subtask. Each foundation subtask under TASK-23 lists its own test ACs, but this subtask is the **roll-up gate**: before TASK-23 closes, the TestSuite must have, for every kept foundation feature, the three test classes the user named:

1. **Unit / regression test** — correctness, edge cases, API contract.
2. **Smoke / stress test** — high-volume, concurrent if applicable, leak-free.
3. **Perf-vs-STL comparison test** — where the engine type has an STL analogue (Allocator vs std::allocator, Array vs std::vector, Queue vs std::queue, HashMap vs std::unordered_map). Recorded numbers; not gated on being faster, but recorded so we know the cost.

Existing TestSuite structure (verify before designing):
- `Source/TestSuite/UnitTests/` — has ArrayTests.cpp, AtomicTests.cpp, RingBufferTests.cpp, FixedSizeStringTests.cpp.
- `Source/TestSuite/StressTests/` — has ConcurrencyStress.cpp.
- No perf-vs-STL subdir exists today. Decide: new `Source/TestSuite/PerfTests/` directory, or extend StressTests.

This subtask's "Done" criterion is that the rolled-up coverage table (header column = feature; row column = unit/stress/perf) has every cell either filled or explicitly marked N/A with a reason (e.g. "Atomic has no STL analogue").

References:
- Source/TestSuite/UnitTests/
- Source/TestSuite/StressTests/
- Each TASK-23.N subtask's own test ACs
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Coverage matrix written in closure note: rows = each kept foundation feature (Allocator, Array, Queue, HashMap, RingBuffer, DoubleBuffer if kept, Atomic if kept, AtomicObject if kept, Handle if kept, FixedSizeString, ObjectPool, Memory). Columns = unit/regression, smoke/stress, perf-vs-STL.
- [ ] #2 Every cell is either: (a) test file path + test name, or (b) explicit N/A + reason.
- [ ] #3 Perf-vs-STL tests record numbers in stdout (engine vs STL nanoseconds per op for representative workloads). Numbers archived in closure note.
- [ ] #4 TestSuite directory structure documented: where unit tests live, where stress tests live, where perf tests live.
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
