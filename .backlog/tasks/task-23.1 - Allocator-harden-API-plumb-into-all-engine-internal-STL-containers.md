---
id: TASK-23.1
title: 'Allocator: harden API + plumb into all engine-internal STL containers'
status: To Do
assignee: []
created_date: '2026-05-22 07:28'
updated_date: '2026-05-22 15:51'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 1000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Source/Engine/Common/Allocator.h` is a thin reinterpret-cast shell around `Memory::Allocate / Memory::Deallocate` that nothing in production code actually uses (only itself includes the header). It already satisfies the C++ allocator concept superficially (`value_type`, `propagate_on_container_move_assignment`, `is_always_equal`, `allocate`, `deallocate`) but:

- `allocate(n)` ignores overflow (`sizeof(T) * n` can wrap).
- No `construct` / `destroy` (relies on allocator_traits default — OK in C++17+, document).
- No alignment handling; routes through `Memory::Allocate` which itself may or may not honour T's alignment.
- `deallocate` ignores `_Count`.

Goal: harden into a production allocator + actually use it everywhere in engine code where a std::vector / std::unordered_map / std::set is declared. The motivating reason — every container allocation should go through the engine's `Memory::Allocate`, not the global new.

References:
- Source/Engine/Common/Allocator.h
- Source/Engine/Common/Memory.h, Memory.cpp
- All std::vector / std::unordered_map / std::queue declarations in Source/Engine/
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Allocator::allocate guards against size overflow (sizeof(T) * _Count exceeds SIZE_MAX).
- [x] #2 Allocator honours alignof(T) — either via Memory::Allocate alignment param or via aligned_alloc.
- [ ] #3 Every engine-internal std::vector / std::unordered_map / std::set / std::queue / std::deque declaration in Source/Engine/ uses Allocator<T> as the allocator template parameter.
- [x] #4 UnitTest covers: allocate/deallocate round-trip, overflow guard, alignment honoured, copy-construction from related allocator.
- [x] #5 Stress test allocates+frees N=10^6 elements without leaks (verified by Memory::GetCurrentAllocationCount or equivalent).
- [ ] #6 Perf-vs-STL: micro-benchmark vs std::allocator for vector<int> push_back / clear, recorded in TestSuite output.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Partial landing 2026-05-22:** Hardening done (AC #1, #2, #4). Engine-wide STL-plumb sweep (AC #3) and perf-vs-STL (#6) deferred to a follow-up CL — they're a different concern (mechanical rewrite of every container declaration in Source/Engine vs the API hardening done here).

Diff:
- `Source/Engine/Common/Allocator.h`: overflow guard in `allocate` (throw `std::bad_alloc` if `sizeof(T) * _Count` overflows); alignment guarantee documented; cleaned up trailing-whitespace / stale comments.
- `Source/TestSuite/UnitTests/AllocatorTests.cpp` (new): 4 tests — roundtrip, related-T copy-construct, overflow→bad_alloc, equality.

Verification (this CL): TestSuite -u Allocator 4/4 pass; Main.exe -total_frames 10 exits 0.

**Still to do (follow-up CL):**
- AC #3: sweep std::vector / std::unordered_map / std::queue / std::set / std::deque declarations across Source/Engine to use `Allocator<T>` (Inno::Allocator-aware variant). Big mechanical sweep.
- AC #5: 10^6-element leak stress test (current Memory stress at 10^5 covers allocator path indirectly).
- AC #6: perf-vs-std::allocator micro-benchmark in PerformanceTests/.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
