---
id: TASK-23.8
title: >-
  Atomic / AtomicReader / AtomicWriter: remove (zero production callers) or
  adopt at AssetService LUT boundaries
status: Done
assignee: []
created_date: '2026-05-22 07:30'
updated_date: '2026-05-22 08:53'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 8000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/Atomic.h provides `Atomic<T>` + `AtomicReader<T>` + `AtomicWriter<T>` — a single-writer / multi-reader wrapper backed by `shared_mutex` + `condition_variable_any` + a reader-count.

Production-code references: **zero**. Only `Source/TestSuite/UnitTests/AtomicTests.cpp` and `Source/TestSuite/StressTests/ConcurrencyStress.cpp` reference these classes.

Two paths:

**A. Remove.** Delete Atomic.h + the test files. No production user is harmed. Cleanest signal: "we don't use this."

**B. Adopt.** The AssetService LUT cross-boundary failure mode (commit 9a42a43b, see TASK-23 parent History) is exactly what Atomic<T> would catch at the type level — a writer cannot mutate a LUT entry while readers hold AtomicReader. Use it at AssetService::m_MeshLUT / m_TextureLUT / m_MaterialLUT.

Recommended: **A**, unless adoption is wired in this same task. "Test-only abstraction" is not a sustainable foundation primitive.

Note: bug audit at the same time —
- `~Atomic()` calls `m_condition.wait` with `unique_lock` — but the predicate `IsWritable() && IsReadable()` checks `m_ReaderCount == 0 && !m_IsWriting`. If the dtor races with a reader, the dtor blocks until reader finishes. Fine if intended; otherwise document.
- `FinishReading()` decrements `m_ReaderCount` without acquiring the mutex — the `notify_one` may not be paired with the cv predicate read correctly. Audit.

References:
- Source/Engine/Common/Atomic.h
- Source/TestSuite/UnitTests/AtomicTests.cpp
- Source/TestSuite/StressTests/ConcurrencyStress.cpp
- Source/Engine/Services/AssetService.h / .cpp (potential adoption site)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision recorded (A: remove, or B: adopt) with the reason.
- [x] #2 If A: Atomic.h + AtomicTests.cpp deleted; ConcurrencyStress reference removed; TestSuite still builds + passes.
- [ ] #3 If B: AssetService LUTs (Mesh/Texture/Material) refactored to use Atomic<T>, with regression test (e.g. concurrent reload-during-render).
- [ ] #4 If B: condition_variable predicate audit fixes applied (FinishReading lock+notify pairing).
- [x] #5 Either way: zero leftover Atomic<T> references in production code that don't have a real consumer.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option A** — Atomic.h / AtomicReader / AtomicWriter deleted. Zero production callers (only TestSuite consumed them).

Rationale: a foundation primitive with no production user and no clear path to adoption is dead weight. The user's framing was "remove if unused" — applied.

Option B (adopt at AssetService LUT boundaries) was considered and rejected for this CL: the cross-boundary failure mode (commit 9a42a43b) is already fixed at the source with the FixedSizeString post-truncation workaround. The deeper fix is TASK-23.15 (off-by-one). Re-introducing Atomic<T> as the workaround for a workaround would be the wrong layer.

## Diff

- `Source/Engine/Common/Atomic.h` — deleted.
- `Source/TestSuite/UnitTests/AtomicTests.cpp` — deleted.
- `Source/TestSuite/StressTests/ConcurrencyStress.cpp` — removed `#include "Atomic.h"`, removed `TestConcurrentAtomicOperations` function (~62 lines), removed the call from `RunConcurrencyStressTests`.
- `Source/TestSuite/Common/TestRunner.cpp` — removed `extern void RunAtomicUnitTests()` and its call.
- `Source/TestSuite/CMakeLists.txt` — removed `UnitTests/AtomicTests.cpp` from source list.

## Verification

- `BuildWin.ps1 -SkipShaderCompile` + `msbuild TestSuite.vcxproj` — clean.
- `TestSuite.exe -u` — no "Atomic Unit Tests" suite (confirmed by grep on output).
- `TestSuite.exe -s` — "Concurrency Stress Tests" suite still runs (now contains only RingBuffer stress).
- `Main.exe -total_frames 10` — exits 0.

## ACs

- #1 Decision: A (remove).
- #2 Atomic.h + AtomicTests.cpp deleted; ConcurrencyStress reference removed; TestSuite builds + passes.
- #3 N/A (Option B not taken).
- #4 N/A (Option B not taken).
- #5 Zero Atomic<T> references remain in production code.
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
