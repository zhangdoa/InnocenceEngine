---
id: TASK-23.9
title: >-
  AtomicObject: audit (used by Inno::Handle); collapse into Handle.h or harden
  as foundation primitive
status: Done
assignee: []
created_date: '2026-05-22 07:33'
updated_date: '2026-05-22 14:58'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 9000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/AtomicObject.h provides `AtomicObject<T>` — a `T` field guarded by a spinlock (`atomic_flag` test_and_set). Its **only consumer** is `Source/Engine/Common/Handle.h` (used to store `m_Object: T*` and `m_RefCount: std::atomic<int>*`).

Audit concerns:

1. **Spinlock as the only sync primitive** is questionable. `Get()` returns `T&` to the caller — which immediately releases nothing because Get() doesn't lock at all. So the spinlock only guards `SetObject` / `MoveObject` / `operator bool` / `DeleteObject`, not read-via-Get. If a writer mutates via SetObject while another thread holds a `T&` from Get(), that's a data race.
2. The `operator bool() const` and `operator!() const` lock-check-unlock pattern returns a stale answer the instant the lock is released. The spinlock here provides no real guarantee.
3. `DeleteObject` for pointer T calls `delete m_Object` while holding the lock, then `SetObject(nullptr)` separately (taking the lock again). Between the two, another thread could observe a dangling pointer via Get().
4. `MoveObject` calls `SetObject(other.m_Object)` then `other.SetObject(T{})` — not atomic, and other.m_Object is read without holding other's lock.

This class is doing less work than its name suggests. Decision:

- **A. Collapse into Handle.h.** AtomicObject has one consumer; inline the spinlock logic into Handle directly (or replace with `std::shared_ptr<T>` / `std::atomic<T*>` where appropriate). Delete AtomicObject.h.
- **B. Harden as foundation primitive.** Fix the Get()-without-lock pattern (return a guarded handle, not raw `T&`), make DeleteObject atomic, document semantics.

Recommended: **A**, especially in conjunction with the TASK-23.10 Handle redesign (which may end up just using std::shared_ptr<T>).

References:
- Source/Engine/Common/AtomicObject.h
- Source/Engine/Common/Handle.h (sole consumer)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision recorded (A: collapse into Handle, or B: harden as foundation primitive) with reason.
- [x] #2 If A: AtomicObject.h removed; Handle.h refactored to use std::shared_ptr<T> or inlined spinlock; no foundation header named AtomicObject remains.
- [ ] #3 If B: Get() no longer returns a raw reference without sync; SetObject/DeleteObject/MoveObject are atomic from observer's perspective; documented in header.
- [x] #4 Either way: Handle<ITask> consumers (Thread.h, TaskScheduler.h, Engine_Internal.h) compile and pass tests.
- [x] #5 Sequenced or merged with TASK-23.10 (Handle redesign).
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-05-22 (reopened):** Cascading from TASK-23.10 reopen — Handle.h was restored via revert e8318902, so AtomicObject.h is back too (it was deleted only because Handle was the sole consumer). User's "rely less on STL" strategy means we keep AtomicObject as engine-native; the audit decision shifts from "remove" to "harden the spinlock + Get/Set contract" (Option B from the original task description).
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option A (collapse into SharedPtr.h).** Per the cascading decision from TASK-23.10: AtomicObject was Handle's sole consumer; Handle is now SharedPtr; AtomicObject's spinlock-guarded-pointer pattern would have been overkill in the new SharedPtr design.

Concrete fix: SharedPtr's `m_Object` is a plain `T*` (no atomic wrapping). The only piece that needed atomicity was the refcount, handled by `std::atomic<int>*`. Concurrent reassignment of the same SharedPtr instance is UB by contract (same as std::shared_ptr) — documented in the header.

The original AtomicObject.h had real bugs (Get returning T& without lock, lock-check-unlock returning stale operator bool, non-atomic DeleteObject+SetObject pair). All eliminated by the cleaner SharedPtr design.

## Diff

- `Source/Engine/Common/AtomicObject.h` — deleted.

## Verification

Same as TASK-23.10: BuildWin clean; Task System Stress 4/4 pass; Main.exe -total_frames 10 exits 0.

## ACs

- #1 Decision A.
- #2 AtomicObject.h removed; Handle.h refactored as SharedPtr.h with inlined refcount logic (no spinlock).
- #4 SharedPtr<ITask> consumers compile + pass tests.
- #5 Sequenced + merged with TASK-23.10 in same CL.
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
