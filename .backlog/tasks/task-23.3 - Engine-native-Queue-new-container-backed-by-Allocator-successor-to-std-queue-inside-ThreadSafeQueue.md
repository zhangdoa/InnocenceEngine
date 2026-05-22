---
id: TASK-23.3
title: >-
  Engine-native Queue: new container backed by Allocator (successor to
  std::queue inside ThreadSafeQueue)
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
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
- [ ] #1 Source/Engine/Common/Queue.h exists with Inno::Queue<T> template.
- [ ] #2 Inno::Queue uses Allocator<T> for memory (depends on TASK-23.1).
- [ ] #3 Surface includes: push, emplace, pop, front, back, size, empty, clear, reserve, swap. Growth strategy matches Inno::Array.
- [ ] #4 Non-trivially-copyable T handled correctly on reallocation.
- [ ] #5 UnitTest covers: FIFO order, grow on full, pop empties, copy/move ctors, non-trivial T.
- [ ] #6 StressTest: 10^6 push/pop interleaved, no leaks.
- [ ] #7 Perf-vs-STL: push N then pop N, compared to std::queue<T>. Within 1.5× for trivial T.
- [ ] #8 Direct std::queue<T> usages outside ThreadSafeQueue swapped to Inno::Queue<T> (count holdouts in closure note).
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
