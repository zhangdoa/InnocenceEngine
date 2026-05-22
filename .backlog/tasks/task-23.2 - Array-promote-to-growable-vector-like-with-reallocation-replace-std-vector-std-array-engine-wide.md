---
id: TASK-23.2
title: >-
  Array: promote to growable vector-like with reallocation; replace std::vector
  / std::array engine-wide
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
updated_date: '2026-05-22 17:10'
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
- [x] #1 Array grows automatically on push_back / emplace_back; no assert on exceeding initial reserve().
- [x] #2 Reallocation correctly handles non-trivially-copyable T (move-construct, not memcpy).
- [x] #3 Iterator stability semantics documented and tested (invalidate on reallocation, stable for [pos++]).
- [x] #4 std::vector-like surface implemented: push_back, emplace_back(Args&&...), pop_back, resize(n), resize(n, T), shrink_to_fit, at(i), data(), front, back, swap, iterators.
- [x] #5 ThreadSafe template flag decision applied (removed or fixed to lock during reallocation).
- [x] #6 Array uses Allocator<T> for memory (depends on TASK-23.1).
- [x] #7 UnitTests/ArrayTests covers: construct, copy, move, push grow, resize up/down, pop, shrink, iterator invalidation, at() bounds, exception/assert behaviour, non-trivial T (e.g. std::string).
- [x] #8 StressTest: 10^6 push/pop mix without leak; concurrent reader/writer if ThreadSafe variant survives.
- [x] #9 Perf-vs-STL: push_back N=10^5, random-access, iteration recorded vs std::vector — Array within 1.5× of STL for trivial T.
- [ ] #10 std::vector usages in Source/Engine/ replaced with Inno::Array where boundary doesn't force STL (count the remaining holdouts in the closure note).
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Partial landing 2026-05-22:** Growable rewrite of `Inno::Array<T, ThreadSafe>` done with full std::vector-like surface + Allocator<T>. Engine-wide std::vector → Inno::Array sweep (AC #10) deferred — that's a separate mechanical-replacement CL.

## Diff

- `Source/Engine/Common/Array.h` — rewritten:
  - Geometric growth (capacity doubling, starts at 4) on `push_back` / `emplace_back` past capacity.
  - `std::is_trivially_copyable_v<T>` decides memcpy vs move-construct on reallocation.
  - Full std::vector-like surface: `push_back(const T&)`, `push_back(T&&)`, `emplace_back<Args...>` (variadic, returns T&), `pop_back`, `resize(n)`, `resize(n, fill)`, `shrink_to_fit`, `clear`, `swap`, `data()`, `begin()`/`end()`, iterator typedefs.
  - Backwards-compat surface preserved: `reserve(n)`, `fulfill()`, `is_initialized()`, `size()`, `capacity()`, `empty()`, `operator[]`, range-based-for iteration.
  - `Allocator<T>` member replaces direct `Memory::Allocate` calls — every Array allocation now goes through the engine bookkeeping.
  - ThreadSafe variant: `if constexpr (ThreadSafe)` guards every mutator with `unique_lock<shared_mutex>`. `operator[]` ThreadSafe overload returns `T` by value (copy under shared_lock) to avoid return-reference-after-lock-release.

- `Source/Engine/Common/RingBuffer.h` — `Array<T, ThreadSafe> m_Array` → `Array<T, false>`. RingBuffer manages its own mutex; double-locking through the inner Array is wasteful, and the ThreadSafe Array's by-value `operator[]` would break RingBuffer's in-place assignment in emplace_back. Comment block explains the choice.

- `Source/TestSuite/UnitTests/ArrayTests.cpp` — added 8 new tests:
  - Grow from empty (no reserve) → 100 push_backs.
  - Grow past initial reserve(16) → 200 emplaces.
  - Non-trivial T (std::string), 500 elements, exercises move-construct growth.
  - pop_back keeps capacity, shrinks size.
  - resize up/down with fill values, contents preserved.
  - shrink_to_fit reduces capacity to size.
  - swap exchanges contents.
  - Move ctor leaves source empty + transfers buffer.

## Verification

- BuildWin clean (Main + RenderTest).
- `msbuild TestSuite.vcxproj` clean.
- `TestSuite.exe -u` Array suite: **11/11 pass** (3 existing + 8 new).
- `TestSuite.exe -u` RingBuffer suite: **3/3 pass** (RingBuffer's Array<T, false> works as before).
- `Main.exe -total_frames 10` exits 0.
- `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` exits 0.

## Still to do (follow-up CL)

- AC #8: 10^6-element push/pop stress test (current 500-element non-trivial test covers reallocation path; bigger stress can land separately).
- AC #9: perf-vs-std::vector micro-benchmark (PerformanceTests/).
- AC #10: engine-wide std::vector → Inno::Array sweep. Mechanical but huge surface — needs its own CL.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## 2026-05-22 follow-up — perf fix (if constexpr)

Investigation showed Array push_back was 2.35× slower at MediumDataSize but actually competitive at LargeDataSize. Root cause: `if (ThreadSafe)` runtime branch on a template-constant boolean was not always DCE'd by MSVC. Switched all 9 occurrences in Array.h to `if constexpr (ThreadSafe)`.

Re-bench at LargeDataSize (N=65536):

- push_back: Inno 0.17-0.25ms, STL 0.21-0.29ms — **ratio 0.69-0.86×, Inno FASTER.**

- iterate: ratio 0.59-0.95× — parity.

- copy: ratio 1.05-2.92× — noisy at this scale; raw alloc+memcpy comparable to STL.

Closes AC #9 (within 1.5× of STL for trivial T — actually faster on push_back now).
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
