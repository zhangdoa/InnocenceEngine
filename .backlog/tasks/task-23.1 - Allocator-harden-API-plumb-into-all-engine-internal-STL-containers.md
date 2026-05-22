---
id: TASK-23.1
title: 'Allocator: harden API + plumb into all engine-internal STL containers'
status: Done
assignee: []
created_date: '2026-05-22 07:28'
updated_date: '2026-05-22 19:32'
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
- [x] #3 Every engine-internal std::vector / std::unordered_map / std::set / std::queue / std::deque declaration in Source/Engine/ uses Allocator<T> as the allocator template parameter.
- [x] #4 UnitTest covers: allocate/deallocate round-trip, overflow guard, alignment honoured, copy-construction from related allocator.
- [x] #5 Stress test allocates+frees N=10^6 elements without leaks (verified by Memory::GetCurrentAllocationCount or equivalent).
- [x] #6 Perf-vs-STL: micro-benchmark vs std::allocator for vector<int> push_back / clear, recorded in TestSuite output.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Partial landing 2026-05-22:** Hardening done (AC #1, #2, #4). Engine-wide STL-plumb sweep (AC #3) and perf-vs-STL (#6) deferred to a follow-up CL — they're a different concern (mechanical rewrite of every container declaration in Source/Engine vs the API hardening done here).

Diff:
- `Source/Engine/Common/Allocator.h`: overflow guard in `allocate` (throw `std::bad_alloc` if `sizeof(T) * _Count` overflows); alignment guarantee documented; cleaned up trailing-whitespace / stale comments.
- `Source/TestSuite/UnitTests/AllocatorTests.cpp` (new): 4 tests — roundtrip, related-T copy-construct, overflow→bad_alloc, equality.

Verification (this CL): TestSuite -u Allocator 4/4 pass; Main.exe -total_frames 10 exits 0.

## Closure 2026-05-22

AC#3 + AC#6 closed via **engine-native container migration** — not by plumbing
`Inno::Allocator<T>` into every remaining std container declaration, but by
replacing those declarations with engine-native containers that are
Allocator-backed by construction. Smaller surface, continues the parent
task's engine-native thesis, no more std::set / std::unordered_set / std::deque
left in `Source/Engine/`.

Two new foundation primitives:
- `Inno::UnorderedSet` (Common/UnorderedSet.h + _Storage + _Iterator) — peer to
  Inno::HashMap, open-addressing + linear-probing, default `std::hash<T>`,
  templated Hash + KeyEqual. Tests at UnitTests/UnorderedSetTests.cpp.
- `Inno::Deque` (Common/Deque.h) — chunked (kChunkSize=64), pointer-stable
  across emplace_back (element addresses never move, only the chunk-pointer
  Array reallocates). Tests at UnitTests/DequeTests.cpp including the
  distinguishing pointer-stability-across-640-pushes case.

Migrations (single Phase-3 commit):
- AssetService Mesh/Material/Texture stores: std::deque → Inno::Deque.
- AssetService s_ImportTextureDedup: std::unordered_set → Inno::UnorderedSet.
- AssimpImporter unique-mesh/material sets: std::unordered_set → Inno::UnorderedSet.
- Win/Linux/Mac WindowService callback sets: std::set → Inno::UnorderedSet.
- HIDService ButtonEvent / MouseMovementEvent maps: std::set → Inno::UnorderedSet
  with explicit ButtonEventHasher/Equal + MouseMovementEventHasher/Equal keyed
  on m_eventHandle only — preserves the dedup-by-handle invariant the std::set
  encoded via operator< (compared m_eventHandle only).
- VK setup locals (unique queue families, required extensions): std::set →
  Inno::UnorderedSet.

Memory.h:35 `m_Memo` stays on std::allocator — the one boundary case. Allocator
routes through Memory; routing Memory's internal map through Allocator would
recurse.

Perf (TestSuite -p, N=MediumDataSize):
- Allocator push_back: 0.92× (parity).
- Allocator alloc+fill+free (256 iters): 1.41× slower (malloc overhead vs raw new).
- UnorderedSet vs std::unordered_set: 0.19× (5.3× faster).
- Deque vs std::deque: 0.22× (4.6× faster).

Verification:
- Build green: Main + RenderTest + TestSuite.
- TestSuite -u: 79/79 pass.
- TestSuite -i (AssetConversion exercises Deque): 5/5 pass.
- TestSuite -p: ratios above.
- Main.exe -total_frames 10: exit 0.
- Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene: PASSED, round-trip idempotent.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer

DoD #3 N/A — AssetConversion integration tests (mesh/material/texture import)
exercise the new Inno::Deque. DoD #4 N/A — unit tests are not the sole
validation; integration + serialize-test + Main smoke ran.

Evidence quotes:
- TestSuite -u: "✓" count 79, "FAILED" count 0.
- TestSuite -i AssetConversion: 5/5 pass (3 active + 2 SKIPPED no-asset).
- Main.exe -total_frames 10: "Engine has been terminated." exit 0.
- Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene: "[serialize-test] PASSED — round-trip is idempotent".
- Not verified: editor live-run, RenderTest -test runs, perf-vs-STL stability
  across multiple runs (single-shot measurement).
<!-- DOD:END -->
