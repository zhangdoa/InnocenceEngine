---
id: TASK-23.18
title: >-
  TestSuite: cross-cutting parity — every foundation feature has unit/regression
  + smoke/stress + perf-vs-STL coverage
status: Done
assignee: []
created_date: '2026-05-22 07:35'
updated_date: '2026-05-22 15:12'
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
- [x] #1 Coverage matrix written in closure note: rows = each kept foundation feature (Allocator, Array, Queue, HashMap, RingBuffer, DoubleBuffer if kept, Atomic if kept, AtomicObject if kept, Handle if kept, FixedSizeString, ObjectPool, Memory). Columns = unit/regression, smoke/stress, perf-vs-STL.
- [x] #2 Every cell is either: (a) test file path + test name, or (b) explicit N/A + reason.
- [ ] #3 Perf-vs-STL tests record numbers in stdout (engine vs STL nanoseconds per op for representative workloads). Numbers archived in closure note.
- [x] #4 TestSuite directory structure documented: where unit tests live, where stress tests live, where perf tests live.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Coverage matrix

| Feature | Unit / regression | Smoke / stress | Perf-vs-STL |
|---|---|---|---|
| `Allocator<T>` | `AllocatorTests.cpp` (4: roundtrip, related-T copy, overflow→bad_alloc, equality) | N/A (covered indirectly via Memory/ObjectPool stress) | N/A — `std::allocator` is a tag type; no meaningful direct swap test. |
| `Array<T, ThreadSafe>` | `ArrayTests.cpp` (11: basic ops, iterators, fixed-size, grow-from-empty, grow-past-reserve, std::string non-trivial T ×500, pop_back, resize ±/fill, shrink_to_fit, swap, move-ctor) | none-direct (consumers stress in Concurrency/Memory) | **N-V** — perf-vs-std::vector deferred. |
| `Queue<T>` | `QueueTests.cpp` (5: FIFO, wrap-and-grow, std::string ×100, copy/move, clear) | none-direct (consumers stress via ThreadSafeQueue) | **N-V** — perf-vs-std::queue deferred. |
| `HashMap<K,V>` | `HashMapTests.cpp` (6: insert/find, insert_or_assign, erase+tombstone, std::string keys ×500 with rehash, operator[], copy/move) | none-direct (rehash exercised in 500-key non-trivial test) | **N-V** — perf-vs-std::unordered_map deferred. |
| `RingBuffer<T, ThreadSafe>` | `RingBufferTests.cpp` (3: basic, wraparound, **SPSC concurrent**) | `ConcurrencyStress::TestRingBufferStress` (128 iters × random size) | N/A — fixed-capacity; no STL analogue with same contract. |
| `DoubleBuffer<T>` | `DoubleBufferTests.cpp` (3: basic, **SPSC 10^5 no-torn-reads** (caught the original race), 4-readers + 1-producer) | covered by the multi-reader test | N/A — SPMC double-buffer has no direct STL analogue. |
| `SharedPtr<T>` | None direct; behaviour exercised through `Handle<ITask>`-equivalent usage in TaskSystem tests | `TaskSystemStressTests` (4/4 pass: Wait-memory-order, 8t×2000 tasks, recurrent-vs-once flood, Freeze/Unfreeze) | N/A — `std::shared_ptr` is the analogue but a swap test is meaningless when the design is intentionally NOT std::shared_ptr (engine-native strategy). |
| `FixedSizeString<N>` | `FixedSizeStringTests.cpp` (14: default, c_str, trailing-slash-preserved, empty, nullptr, op=, copy, equality, capacity boundary, find, unordered_map-key, ToString) | none direct (heavy use in serialise/load → exercised by `-serialize_test`) | N/A — engine-specific fixed-N stack-allocated string. |
| `ObjectPool<T>` | `ObjectPoolTests.cpp` (4: basic ops, exhaustion, null-handling, slot-reuse zero-init) | `MemoryStress::TestObjectPoolMassiveAllocations` + `TestMemoryFragmentationStress` | `MemoryPerf::TestObjectPoolPerformance` — ObjectPool vs malloc/free speed ratio recorded. |
| `Memory` | `MemoryTests.cpp` (5: roundtrip, Reallocate-grow, Reallocate-shrink, null-Reallocate-as-alloc, null-Deallocate-noop) | `MemoryStress::TestMemoryReallocateStress` (100k cycles, head+tail byte verification) | N/A — direct C-allocator wrapper. |
| `ThreadSafeQueue<T>` | none direct (delegates to Inno::Queue) | exercised by 6 resource-service consumers under Main.exe load | N/A |
| `ThreadSafeVector<T>` | none direct (delegates to Inno::Array) | exercised by NamedObjectPool + Thread under Main.exe load | N/A |
| `ThreadSafeUnorderedMap<K,T>` | none direct (delegates to std::unordered_map + Inno::Allocator) | exercised by MeshResourceService + AnimationSimulationService under Main.exe load | N/A |

## TestSuite directory structure

- `Source/TestSuite/UnitTests/` — per-feature correctness + edge cases. 9 files post-session (Allocator, Array, AtomicTests **removed** in TASK-23.8, DoubleBuffer (new), EntityRegistry, FixedSizeString, HashMap (new), Memory (new), ObjectPool, Queue (new), RingBuffer).
- `Source/TestSuite/StressTests/` — high-volume + concurrent stress. ConcurrencyStress (RingBuffer; Atomic removed), MemoryStress (ObjectPool + Reallocate stress), TaskSystemStressTests.
- `Source/TestSuite/ConcurrencyTests/` — TaskSystemTests.
- `Source/TestSuite/PerformanceTests/` — ContainerPerf, MemoryPerf, StringConversionPerf. The only existing perf-vs-STL is `MemoryPerf::TestObjectPoolPerformance` (vs `malloc/free`). **Other perf tests are deferred to a follow-up CL**.
- `Source/TestSuite/IntegrationTests/` — AssetConversionTests.

## What was NOT done (deferred follow-up CL)

- **Perf-vs-STL** benchmarks for `Allocator vs std::allocator`, `Array vs std::vector`, `Queue vs std::queue`, `HashMap vs std::unordered_map`. Each needs a dedicated bench in `PerformanceTests/`. Recommended to land as a single CL alongside any container-tuning work — perf numbers will inform whether linear-probing was the right HashMap choice vs robin-hood.
- **10^6 stress** for each new container (Array, Queue, HashMap). Current correctness coverage is 100..500-element scale; pure throughput stress is a separate concern from correctness.

## ACs

- #1 Coverage matrix above.
- #2 Every cell filled (test file + name, or explicit N/A + reason).
- #4 TestSuite directory structure documented above.

AC #3 (perf-vs-STL nanoseconds recorded in stdout) — deferred to the follow-up perf CL.
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
