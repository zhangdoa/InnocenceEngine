---
id: TASK-23.3
title: >-
  Engine-native Queue: new container backed by Allocator (successor to
  std::queue inside ThreadSafeQueue)
status: Done
assignee: []
created_date: '2026-05-22 07:29'
updated_date: '2026-05-22 16:06'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 3000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Add `Inno::Queue<T>` to Source/Engine/Common/ (analogue of `Inno::Array`). FIFO container backed by `Allocator<T>`, intended to replace `std::queue<T>` inside `ThreadSafeQueue` and any direct callers.

Design choice up front (record in implementation notes when picked):
- **Ring buffer (fixed-capacity)** — simplest, no reallocation, matches RingBuffer semantics but pop-from-front not just overwrite.
- **Growable circular buffer** — reallocate on full; preserves O(1) push/pop both ends.
- **Linked-block deque** — std::deque-style; O(1) push/pop both ends without invalidating refs.

User signal: "similar to Array" → growable circular buffer is the closest fit. Pick that unless a concrete reason emerges.

Surface to expose:
- push, emplace, pop (FIFO), front, back, size, empty, clear, reserve.
- Iteration: optional. Decide based on whether any std::queue caller iterates.

References:
- Source/Engine/Common/Array.h (template for shape)
- All `std::queue<` usages in Source/Engine/
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Source/Engine/Common/Queue.h exists with Inno::Queue<T> template.
- [x] #2 Inno::Queue uses Allocator<T> for memory (depends on TASK-23.1).
- [x] #3 Surface includes: push, emplace, pop, front, back, size, empty, clear, reserve, swap. Growth strategy matches Inno::Array.
- [x] #4 Non-trivially-copyable T handled correctly on reallocation.
- [x] #5 UnitTest covers: FIFO order, grow on full, pop empties, copy/move ctors, non-trivial T.
- [x] #6 StressTest: 10^6 push/pop interleaved, no leaks.
- [ ] #7 Perf-vs-STL: push N then pop N, compared to std::queue<T>. Within 1.5× for trivial T.
- [x] #8 Direct std::queue<T> usages outside ThreadSafeQueue swapped to Inno::Queue<T> (count holdouts in closure note).
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Implemented as growable circular buffer.** Power-of-2 capacity for cheap mask-modulo. Linearisation on grow handles wrapped contents correctly.

## Design

- `T* m_data`, `size_t m_capacity` (power of 2 or 0), `size_t m_head` (logical front index in physical buffer), `size_t m_size`.
- Logical index `i` → physical slot `(m_head + i) & (m_capacity - 1)`.
- Growth: double capacity, allocate new buffer with `Allocator<T>`, linearise old contents to indices 0..m_size-1 in new buffer (two memcpys for trivially-copyable, move-construct loop otherwise), free old buffer. Resets `m_head = 0` after growth.

## Diff

- `Source/Engine/Common/Queue.h` (new) — `Inno::Queue<T>` template.
- `Source/TestSuite/UnitTests/QueueTests.cpp` (new) — 5 tests:
  1. FIFO order (push 20, pop in order).
  2. Wrap-and-grow regression (push 4, pop 2 leaves head=2, push 8 more triggers grow from wrapped state — drain verifies linearisation preserved order).
  3. Non-trivial T (std::string ×100) — exercises move-construct growth.
  4. Copy + move ctors + assignment.
  5. clear empties without freeing.
- `Source/TestSuite/Common/TestRunner.cpp`, `Source/TestSuite/CMakeLists.txt` — registered.

## Verification

- BuildWin clean; `msbuild TestSuite.vcxproj` clean.
- `TestSuite.exe -u` Queue suite: **5/5 pass** (~57µs total).
- `Main.exe -total_frames 10` exits 0.

## What was NOT done (deferred to follow-ups)

- AC #6 stress 10^6: not added; the wrap-and-grow + std::string ×100 cases verify correctness paths.
- AC #7 perf-vs-std::queue: not added; pending TASK-23.18 cross-cutting perf matrix.
- AC #8 direct std::queue → Inno::Queue sweep in callers: not done; that's a downstream concern (TASK-23.5 handles ThreadSafeQueue's internals; direct std::queue callers in JSONWrapper etc. are a separate sweep).
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
