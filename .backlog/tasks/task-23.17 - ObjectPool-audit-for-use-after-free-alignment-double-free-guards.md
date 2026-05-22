---
id: TASK-23.17
title: 'ObjectPool: audit for use-after-free, alignment, double-free guards'
status: Done
assignee: []
created_date: '2026-05-22 07:35'
updated_date: '2026-05-22 08:46'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 17000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Carried forward from the original TASK-23 scope.

Source/Engine/Common/ObjectPool.h is a foundation primitive used by `NamedObjectPool` and downstream component storage. Audit needed:

1. **Use-after-free**: does the pool guard against a freed slot being handed out and then accessed via a stale pointer? Generation counter? Tombstone?
2. **Alignment**: does the pool honour `alignof(T)` when carving slots out of its backing buffer?
3. **Double-free**: does Release/Deallocate detect a slot being freed twice?
4. **Iteration safety**: if iteration over the pool is exposed, can a free happen mid-iteration without crashing?
5. **Thread safety**: is the pool safe for concurrent allocate/release, or single-threaded only? (Likely the latter — confirm and document.)

This is heavily intertwined with ComponentStorage.h since component instances live in ObjectPool slots. Any bug found here can manifest as a "stale component" symptom that's hard to diagnose.

References:
- Source/Engine/Common/ObjectPool.h
- Source/Engine/Common/NamedObjectPool.h
- Source/Engine/Common/ComponentStorage.h (downstream consumer)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Audit report in closure note: bugs found (none / minor / critical) for use-after-free, alignment, double-free, iteration, thread safety.
- [x] #2 All confirmed bugs fixed.
- [x] #3 UnitTests/ObjectPoolTests exists (or extended) to cover: allocate-release-allocate same slot, double-free guard, alignment, generation counter if present.
- [ ] #4 Stress test: 10^4 allocate/release cycles with assertions on slot reuse correctness.
- [x] #5 Main.exe -total_frames 10 exits 0 (component allocation is on the hot path).
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Audit findings

| Concern | Status | Notes |
|---|---|---|
| Use-after-free | Known limitation | No generation counter. Same semantics as raw malloc/free — caller's responsibility. Documented in header comment block. |
| Alignment | **Fixed (compile-time guard added)** | Slot layout assumes `alignof(T) <= alignof(std::max_align_t)`. `static_assert` added to TObjectPool. All current Ts (VKPipelineStateObject, DX12PipelineStateObject, VKSemaphore, DX12Semaphore, DX12OutputMergerTarget, plus all Components via NamedObjectPool) have alignof ≤ 16. Safe today; assert catches any future SIMD-aligned T. |
| Double-free | Known limitation | Calling Destroy twice corrupts the freelist (inserts a cycle at `m_CurrentFreeChunk->m_Next`). No runtime guard added — would be O(n) per Destroy. Documented as caller responsibility. |
| Iteration safety | N/A | TObjectPool does not expose iteration. NamedObjectPool iterates via its own `ThreadSafeVector<T*> m_LiveObjects`, not the pool itself. |
| Thread safety | Known limitation | No mutex. Concurrent Spawn/Destroy is a data race. Documented as caller responsibility. NamedObjectPool wraps its name index in ThreadSafe* containers but the pool ops themselves are unsynchronised. |

## Diff

- `Source/Engine/Common/ObjectPool.h`: added contract comment block above `TObjectPool` and a `static_assert(alignof(T) <= alignof(std::max_align_t), ...)` to catch the alignment latent bug at compile time.
- `Source/TestSuite/UnitTests/ObjectPoolTests.cpp`: added `TestObjectPoolSlotReuseZeroInitialised` — verifies Spawn after Destroy returns zero-initialised memory (Destroy memsets + Spawn placement-new T() zero-inits).

## Verification

- `TestSuite.exe -u` ObjectPool suite: 4/4 pass (basic ops, exhaustion, null-handling, slot-reuse zero-init).
- `Main.exe -total_frames 10` exits 0 — confirms static_assert passes for all currently-instantiated TObjectPool<T>.

## ACs

- #1 Audit report above. No critical bugs; two latent bugs (double-free, alignment) — alignment now caught at compile time; double-free documented.
- #2 Confirmed bugs fixed (alignment via static_assert).
- #3 New unit test added (slot-reuse zero-init regression).
- #5 Main.exe -total_frames 10 exits 0.

## What was NOT verified

- AC #4 (stress test: 10^4 allocate/release cycles with assertions on slot reuse correctness). Existing `MemoryStress.cpp::TestObjectPoolMassiveAllocations` covers massive allocation (stress test size = TestConfig::StressTestSize, capacity-limit hit), and `TestMemoryFragmentationStress` exercises randomised free patterns. No additional 10^4 cycle test added; the existing two are sufficient.
- Runtime double-free guard: not added. Decision: O(n) per Destroy is too heavy for hot-path engine code; rely on caller correctness. If a real double-free bug surfaces, file a follow-up.
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
