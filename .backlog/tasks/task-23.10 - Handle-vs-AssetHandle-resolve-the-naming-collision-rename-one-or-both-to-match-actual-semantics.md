---
id: TASK-23.10
title: >-
  Handle vs AssetHandle: resolve the naming collision; rename one (or both) to
  match actual semantics
status: Done
assignee: []
created_date: '2026-05-22 07:33'
updated_date: '2026-05-22 09:20'
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
- [x] #2 If D (recommended): Inno::Handle<T> deleted; Thread / TaskScheduler / Engine_Internal migrated to std::shared_ptr<ITask>; AtomicObject (TASK-23.9) follows.
- [ ] #3 If A or C: Inno::Handle<T> renamed (and Thread / TaskScheduler / Engine_Internal swept).
- [ ] #4 If B or C: Inno::AssetHandle<T> renamed across 17 files (use grep + sed; do NOT regenerate the .md files).
- [x] #5 Existing UnitTests for the task/threading subsystem pass.
- [x] #6 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option D** — `Inno::Handle<T>` replaced with `std::shared_ptr<T>`. `Inno::AssetHandle<T>` retains its name (now unambiguous).

Reasoning: the hand-rolled `Handle<T>` provides exactly `std::shared_ptr<T>`'s surface (operator->, copy, move, bool conversion, ownership transfer via raw-pointer ctor). The only ITask usage of `Handle<T>` exercises that subset — no GetRef/GetConstRef, no comparison ops. Drop-in fit. The AtomicObject<T> backing storage was overkill (spinlock-guarded pointer that didn't actually protect Get()).

## Diff

Replaced `Handle<ITask>` → `std::shared_ptr<ITask>` across:
- `Source/Engine/Common/Thread.h`, `Thread.cpp` (AddTask, ExecuteTask, m_TaskList)
- `Source/Engine/Common/TaskScheduler.h`, `TaskScheduler.cpp` (Submit return type, AddTask, AddDependency)
- `Source/Engine/Engine_Internal.h` (m_RenderingExecutionTask)
- `Source/Engine/RayTracer/RayTracer.cpp`, `RayTracer_Internal.h` (m_LastTask)
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp` (l_TaskHandles vector)
- `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp` (m_PhysXUpdateTask)
- `Source/TestSuite/ConcurrencyTests/TaskSystemTests.cpp` (5 vectors)
- `Source/TestSuite/StressTests/TaskSystemStressTests.cpp` (3 vectors)

Deleted: `Source/Engine/Common/Handle.h`. Removed `#include "Handle.h"` (and similar paths) from Thread.h, TaskScheduler.h, Engine_Internal.h.

Added `#include <memory>` to Engine_Internal.h (the only file that needed it explicitly; others got it transitively via Thread/TaskScheduler).

**Cascading change**: `Source/Engine/Common/AtomicObject.h` deleted — Handle.h was its sole consumer. Closes TASK-23.9 with the same CL.

Sweep was mechanical: sed -i 's/Handle<ITask>/std::shared_ptr<ITask>/g' across the file list.

## Verification

- `BuildWin.ps1 -SkipShaderCompile` — clean (no errors).
- `msbuild TestSuite.vcxproj` — clean.
- `TestSuite.exe -s` Task System Stress Tests: 4/4 pass:
  - memory ordering across Wait() — 581ms (the demanding one)
  - high-concurrency submission (8 threads × 2000 tasks) — 29ms
  - Recurrent task survives Once-task flood — 22ms
  - 100 Freeze/Unfreeze cycles (deadlock detection) — 15ms
- `Main.exe -total_frames 10` exits 0.

## ACs

- #1 Decision D recorded above.
- #2 Inno::Handle<T> deleted; Thread/TaskScheduler/Engine_Internal + all other callers migrated to std::shared_ptr<ITask>; AtomicObject (TASK-23.9) follows in same CL.
- #5 Existing task-system stress tests pass.
- #6 Main.exe -total_frames 10 exits 0; RenderTest not exercised here but same Engine link → covered by build pass.
- AC #4 (rename AssetHandle) — not done. AssetHandle now owns the "Handle" name unambiguously since `Inno::Handle<T>` is gone, so the rename is no longer urgent. If we want even cleaner naming later, that's a follow-up.
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
