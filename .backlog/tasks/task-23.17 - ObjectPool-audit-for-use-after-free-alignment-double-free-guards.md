---
id: TASK-23.17
title: 'ObjectPool: audit for use-after-free, alignment, double-free guards'
status: To Do
assignee: []
created_date: '2026-05-22 07:35'
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
- [ ] #1 Audit report in closure note: bugs found (none / minor / critical) for use-after-free, alignment, double-free, iteration, thread safety.
- [ ] #2 All confirmed bugs fixed.
- [ ] #3 UnitTests/ObjectPoolTests exists (or extended) to cover: allocate-release-allocate same slot, double-free guard, alignment, generation counter if present.
- [ ] #4 Stress test: 10^4 allocate/release cycles with assertions on slot reuse correctness.
- [ ] #5 Main.exe -total_frames 10 exits 0 (component allocation is on the hot path).
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
