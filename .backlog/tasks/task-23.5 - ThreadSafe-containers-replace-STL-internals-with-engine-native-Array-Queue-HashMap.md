---
id: TASK-23.5
title: >-
  ThreadSafe* containers: replace STL internals with engine-native Array / Queue
  / HashMap
status: To Do
assignee: []
created_date: '2026-05-22 07:29'
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
- [ ] #1 ThreadSafeVector<T> wraps Inno::Array<T> internally.
- [ ] #2 ThreadSafeQueue<T> wraps Inno::Queue<T> internally.
- [ ] #3 ThreadSafeUnorderedMap<K,T> wraps Inno::HashMap<K,T> internally.
- [ ] #4 ThreadSafeVector::size() is const.
- [ ] #5 ThreadSafeVector::eraseByIndex bug fixed (was calling non-existent std::vector::erase(size_t)).
- [ ] #6 getRawData() return-reference-after-lock-release pattern decided (removed, snapshot-copy, or documented).
- [ ] #7 All consumer services compile and pass their existing tests after migration.
- [ ] #8 Unit + concurrent stress test for each wrapper exists: parallel readers + writers, no data race under TSan / equivalent (or Helgrind on Linux).
- [ ] #9 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0 after migration.
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
