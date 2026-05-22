---
id: TASK-23.2
title: >-
  Array: promote to growable vector-like with reallocation; replace std::vector
  / std::array engine-wide
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 2000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Today `Inno::Array<T, ThreadSafe>` (Source/Engine/Common/Array.h) is **fixed-capacity-after-reserve**: `reserve()` can only be called once (`assert(!m_Initialized)`), `emplace_back` asserts on overflow. It is a managed slab, not a vector. User wants it to behave like std::vector — automatic growth on push, capacity / size separation, iterator semantics, swap, shrink_to_fit, etc. — so it can become the default engine-side sequence container.

Scope:

1. Implement growth strategy (geometric, e.g. 1.5× or 2×) — reallocate on capacity exhaustion, memcpy/move-construct elements, deallocate old buffer.
2. Distinguish `T` trivially_copyable (memcpy) from non-trivial (move-construct loop). The current memcpy in copy-ctor / op= is undefined for non-trivial T.
3. Add the std::vector-like surface: `push_back`, `emplace_back` (variadic), `pop_back`, `resize(n)`, `resize(n, T)`, `shrink_to_fit`, `clear` (already exists), `at(i)` (bounds-checked), `data()`, `front`, `back`, `swap`, iterator/const_iterator typedefs.
4. Decide: keep `ThreadSafe` template flag? The flag wraps `[]` with `shared_lock` but doesn't lock during reallocation, so it's currently unsafe under concurrent push. Either fix or remove the flag and split into `Array<T>` + `ConcurrentArray<T>`.
5. Use `Allocator<T>` (TASK-23.1 prerequisite) — do NOT call `Memory::Allocate` directly anymore.
6. Sweep engine code: replace `std::vector<T>` with `Inno::Array<T>` where reasonable. Hot spots: services, components, render queue. Keep std::vector at interop boundaries (e.g. external libs).

Test coverage is the heaviest part of this subtask — see AC.

References:
- Source/Engine/Common/Array.h
- All `std::vector<` usages in Source/Engine/
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Array grows automatically on push_back / emplace_back; no assert on exceeding initial reserve().
- [ ] #2 Reallocation correctly handles non-trivially-copyable T (move-construct, not memcpy).
- [ ] #3 Iterator stability semantics documented and tested (invalidate on reallocation, stable for [pos++]).
- [ ] #4 std::vector-like surface implemented: push_back, emplace_back(Args&&...), pop_back, resize(n), resize(n, T), shrink_to_fit, at(i), data(), front, back, swap, iterators.
- [ ] #5 ThreadSafe template flag decision applied (removed or fixed to lock during reallocation).
- [ ] #6 Array uses Allocator<T> for memory (depends on TASK-23.1).
- [ ] #7 UnitTests/ArrayTests covers: construct, copy, move, push grow, resize up/down, pop, shrink, iterator invalidation, at() bounds, exception/assert behaviour, non-trivial T (e.g. std::string).
- [ ] #8 StressTest: 10^6 push/pop mix without leak; concurrent reader/writer if ThreadSafe variant survives.
- [ ] #9 Perf-vs-STL: push_back N=10^5, random-access, iteration recorded vs std::vector — Array within 1.5× of STL for trivial T.
- [ ] #10 std::vector usages in Source/Engine/ replaced with Inno::Array where boundary doesn't force STL (count the remaining holdouts in the closure note).
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
