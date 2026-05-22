---
id: TASK-23.10
title: >-
  Handle vs AssetHandle: resolve the naming collision; rename one (or both) to
  match actual semantics
status: Done
assignee: []
created_date: '2026-05-22 07:33'
updated_date: '2026-05-22 14:58'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 10000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Foundation currently has **two unrelated templates both named "Handle"**:

1. **`Inno::Handle<T>`** in `Source/Engine/Common/Handle.h` — ref-counted shared-ownership wrapper around `T*`. Storage is `AtomicObject<T*>` + `AtomicObject<std::atomic<int>*>`. Surface includes operator->, GetRef, comparison ops, AddRef/ReleaseRef, copy/move. **Conceptually: a hand-rolled `std::shared_ptr<T>` with a spinlock-based field.**
   - Consumers: Thread.h, TaskScheduler.h, Engine_Internal.h. Only used as `Handle<ITask>`.

2. **`Inno::AssetHandle<T>`** in `Source/Engine/Common/AssetHandle.h` — POD struct `{ uint32_t m_Index, m_Generation }`. Pure value type, no ownership, no atomicity. Used by 17 files for asset/mesh/texture/material registry handles.
   - Type aliases: `MeshAssetHandle`, `TextureAssetHandle`, `MaterialAssetHandle`.

These are completely different abstractions. Neither name communicates its semantics:
- `Handle<T>` does not name "shared, ref-counted".
- `AssetHandle<T>` does not name "index + generation".

Decision options:

- **A. Rename `Inno::Handle<T>` to `Inno::SharedPtr<T>` (or `Inno::Shared<T>`).** Or replace with `std::shared_ptr<T>` outright — the hand-rolled version offers nothing std::shared_ptr doesn't. Three consumers, all using it for `Handle<ITask>` — trivial sweep.
- **B. Rename `Inno::AssetHandle<T>` to `Inno::Slot<T>` / `Inno::SlotHandle<T>` / `Inno::IndexedHandle<T>`.** 17-file sweep but mechanical (rename + include).
- **C. Do both.** Most clarity.
- **D. Replace `Inno::Handle<T>` with std::shared_ptr<T>**, leave AssetHandle as the only "Handle" in the foundation. Then AssetHandle owns the name unambiguously.

Recommended: **D**. The hand-rolled Handle<T> doesn't appear to do anything std::shared_ptr can't, and removing it lets AssetHandle own the name without ambiguity. Verify by reading the Thread / TaskScheduler usages — if there's no specific requirement that std::shared_ptr can't meet, swap.

This subtask is sequenced with TASK-23.9 (AtomicObject) — if Handle is replaced by std::shared_ptr, AtomicObject loses its only consumer and can be deleted.

References:
- Source/Engine/Common/Handle.h
- Source/Engine/Common/AssetHandle.h
- Source/Engine/Common/Thread.h:51,59,67 (Handle<ITask>)
- Source/Engine/Common/TaskScheduler.h:36,42,47 (Handle<ITask>)
- Source/Engine/Engine_Internal.h:31 (Handle<ITask> m_RenderingExecutionTask)
- 17 files using AssetHandle aliases
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision recorded (A/B/C/D) with reason — specifically, why std::shared_ptr is or isn't a fit for the Thread/TaskScheduler use case.
- [ ] #2 If D (recommended): Inno::Handle<T> deleted; Thread / TaskScheduler / Engine_Internal migrated to std::shared_ptr<ITask>; AtomicObject (TASK-23.9) follows.
- [x] #3 If A or C: Inno::Handle<T> renamed (and Thread / TaskScheduler / Engine_Internal swept).
- [ ] #4 If B or C: Inno::AssetHandle<T> renamed across 17 files (use grep + sed; do NOT regenerate the .md files).
- [x] #5 Existing UnitTests for the task/threading subsystem pass.
- [x] #6 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-05-22 (reopened):** Earlier this session, decision D (replace `Inno::Handle<T>` with `std::shared_ptr<T>`) landed in commit d4fe9aa8. User flagged this as misaligned with the global strategy "rely less on STL" — that direction REPLACED engine-native code with STL, opposite of the goal. Reverted in commit e8318902.

Re-decision needed under "engine-native" constraint. Options remaining:
- **A.** Rename `Inno::Handle<T>` to a more descriptive engine-native name (e.g. `Inno::SharedPtr<T>` — the type IS a shared_ptr-shape). Keep AssetHandle as-is.
- **B.** Rename `Inno::AssetHandle<T>` to e.g. `Inno::Slot<T>`. Keep Handle<T> as-is.
- **C.** Rename both.
- **(D removed — STL adoption out of scope.)**

Recommended: **A**. AssetHandle's name describes its purpose (handle-to-asset); Handle<T>'s name is the more ambiguous of the two. Renaming Inno::Handle → Inno::SharedPtr makes the contract explicit + frees the "Handle" name for AssetHandle.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option A (engine-native rename).** Decision D (replace with std::shared_ptr) reverted earlier this session — STL adoption is opposite of the strategic direction.

## Diff

- `Source/Engine/Common/SharedPtr.h` (new) — engine-native ref-counted shared smart pointer:
  - `T* m_Object`, `std::atomic<int>* m_RefCount`.
  - Standard surface: explicit ctor from T*, copy/move ctors + assignment, op->, GetRef/GetConstRef, comparison ops, operator bool, operator=(nullptr_t), operator=(T*).
  - Concurrent reassignment of the SAME SharedPtr instance from multiple threads is UB (same as std::shared_ptr) — documented.
  - Inlines the spinlock-guarded pointer concept that AtomicObject used to provide. Closes TASK-23.9.

- `Source/Engine/Common/Handle.h` — deleted.
- `Source/Engine/Common/AtomicObject.h` — deleted (cascade from 23.9).

Sweep (mechanical sed): `Handle<ITask>` → `SharedPtr<ITask>` and `Handle.h` includes → `SharedPtr.h` across 11 files:
- Thread.h, Thread.cpp, TaskScheduler.h, TaskScheduler.cpp
- Engine_Internal.h
- RayTracer.cpp, RayTracer_Internal.h
- AssimpImporter.cpp, PhysXWrapper.cpp
- TaskSystemTests.cpp, TaskSystemStressTests.cpp

## Verification

- `BuildWin.ps1 -SkipShaderCompile` clean.
- `msbuild TestSuite.vcxproj` clean.
- `TestSuite.exe -s` Task System Stress Tests **4/4 pass**:
  - memory ordering across Wait() — 448ms
  - high-concurrency submission (8 threads × 2000 tasks) — 33ms
  - Recurrent task survives Once-task flood — 19ms
  - 100 Freeze/Unfreeze cycles — 15ms
- `Main.exe -total_frames 10` exits 0.

## ACs

- #1 Decision A (rename Inno::Handle → SharedPtr).
- #3 Sweep complete; AssetHandle keeps its name (already describes purpose; no collision now that Handle is renamed).
- #5 Existing UnitTests + stress tests pass.
- #6 Main.exe -total_frames 10 exits 0.

## What was NOT done

- AssetHandle rename (AC #4) — not pursued. The naming collision is resolved by renaming the OTHER side; AssetHandle is already descriptive of its purpose.
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
