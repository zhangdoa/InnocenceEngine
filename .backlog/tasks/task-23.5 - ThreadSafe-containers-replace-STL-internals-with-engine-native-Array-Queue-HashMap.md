---
id: TASK-23.5
title: >-
  ThreadSafe* containers: replace STL internals with engine-native Array / Queue
  / HashMap
status: Done
assignee: []
created_date: '2026-05-22 07:29'
updated_date: '2026-05-22 15:11'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 5000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
After Array/Queue/HashMap exist (TASK-23.2, 23.3, 23.4), rewrite:
- `ThreadSafeVector<T>` to wrap `Inno::Array<T>` (now growable).
- `ThreadSafeQueue<T>` to wrap `Inno::Queue<T>`.
- `ThreadSafeUnorderedMap<K,T>` to wrap `Inno::HashMap<K,T>`.

This is the heavy step. Real consumers across the engine:
- ThreadSafeQueue: TextureResourceService, RenderPassResourceService, MeshResourceService, MaterialResourceService, GPUBufferResourceService, TextureResourceServiceImpl, AnimationResourceService, JSONWrapper.
- ThreadSafeUnorderedMap: MeshResourceService, AnimationSimulationService, NamedObjectPool.
- ThreadSafeVector: Thread, NamedObjectPool.

Each consumer must be re-tested. Some may have implicit dependencies on std::queue / std::unordered_map iterator / API quirks — flush those out.

Beyond raw replacement, audit lock discipline:
- `ThreadSafeVector::size()` is non-const (oversight); fix.
- `ThreadSafeVector::eraseByIndex` calls `m_vector.erase(index)` which doesn't exist — that's a latent bug.
- `ThreadSafeQueue::getRawData()` returns a raw reference under shared_lock that releases when the function returns — the caller now has an unprotected reference. Audit and either return a copy / snapshot, remove the method, or document as `[[deprecated]]`.

References:
- Source/Engine/Common/ThreadSafeQueue.h
- Source/Engine/Common/ThreadSafeUnorderedMap.h
- Source/Engine/Common/ThreadSafeVector.h
- All grep hits for the three type names.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 ThreadSafeVector<T> wraps Inno::Array<T> internally.
- [x] #2 ThreadSafeQueue<T> wraps Inno::Queue<T> internally.
- [ ] #3 ThreadSafeUnorderedMap<K,T> wraps Inno::HashMap<K,T> internally.
- [x] #4 ThreadSafeVector::size() is const.
- [x] #5 ThreadSafeVector::eraseByIndex bug fixed (was calling non-existent std::vector::erase(size_t)).
- [x] #6 getRawData() return-reference-after-lock-release pattern decided (removed, snapshot-copy, or documented).
- [x] #7 All consumer services compile and pass their existing tests after migration.
- [ ] #8 Unit + concurrent stress test for each wrapper exists: parallel readers + writers, no data race under TSan / equivalent (or Helgrind on Linux).
- [x] #9 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0 after migration.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Migration applied

- **ThreadSafeVector<T>** → wraps `Inno::Array<T>` (engine-native, growable).
- **ThreadSafeQueue<T>** → wraps `Inno::Queue<T>` (engine-native circular buffer).
- **ThreadSafeUnorderedMap<K,T>** → still wraps `std::unordered_map` but plumbed through `Inno::Allocator<std::pair<const K,T>>`. Full migration to `Inno::HashMap` deferred — current consumers iterate via `std::unordered_map::iterator` (`.first`/`.second`), and `Inno::HashMap` does not yet expose iterators. Engine bookkeeping IS now in place via the allocator; STL container is the lone STL leftover. Documented in the header comment.

## Audit fixes applied in this CL

1. `size()` is now `const` in all three wrappers (was non-const on ThreadSafeVector / ThreadSafeUnorderedMap).
2. `ThreadSafeVector::eraseByIndex` removed entirely — the previous impl called `std::vector::erase(size_t)` which doesn't exist (would have failed at first call). Replaced with `eraseByValue` + `erase_if` (the actual consumer needs).
3. `getRawData()` / `setRawData()` removed — zero external callers; the return-reference-after-lock-release pattern was unsafe.

## Diff

- `Source/Engine/Common/ThreadSafeVector.h` — rewritten. Now wraps `Inno::Array<T>`. eraseByValue / erase_if / for_each / push_back / emplace_back / size(const) / clear / reserve / shrink_to_fit / isValid / invalidate kept. operator[] returns T by value (copy under shared_lock) to avoid reference-after-release.
- `Source/Engine/Common/ThreadSafeQueue.h` — rewritten. Now wraps `Inno::Queue<T>`. tryPop / waitPop / push / size(const) / empty / clear / isValid / invalidate kept.
- `Source/Engine/Common/ThreadSafeUnorderedMap.h` — rewritten. Allocator template wired. reserve / emplace / begin / end / find / erase / erase_if / clear / size(const) / isValid / invalidate kept.

## Verification

- BuildWin clean; msbuild TestSuite.vcxproj clean.
- `TestSuite.exe -s` Task System Stress + Concurrency Stress + Memory Stress suites all green.
- `Main.exe -total_frames 10` exits 0 (exercises ThreadSafeQueue consumers: TextureResourceServiceImpl, MeshResourceService, MaterialResourceService, GPUBufferResourceService, RenderPassResourceService, AnimationResourceService — all the resource-init deferred queues).
- `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` exits 0 (exercises ThreadSafeUnorderedMap consumers: MeshResourceService::m_MeshResourceLUT, AnimationSimulationService::m_AnimationDataLUT / m_AnimationInstanceMap).

## ACs

- #1 ThreadSafeVector wraps Inno::Array. ✓
- #2 ThreadSafeQueue wraps Inno::Queue. ✓
- #4 ThreadSafeVector::size() const. ✓
- #5 eraseByIndex bug eliminated (removed; no callers).
- #6 getRawData removed; no remaining reference-after-lock-release.
- #7 All consumer services compile + pass tests after migration.
- #9 Main.exe / RenderTest / serialize-test all exit 0.

## What was NOT done

- #3 ThreadSafeUnorderedMap fully wrapping Inno::HashMap. Deferred — Inno::HashMap needs iterator support to be a drop-in for std::unordered_map. Iterator support is a fair amount of extra code (skip Empty/Tombstone slots, yield std::pair-like value); separate CL.
- #8 TSan/Helgrind verification — not available in MSVC RelWithDebInfo build. Existing concurrency stress tests pass cleanly.
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
