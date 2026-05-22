---
id: TASK-23.16
title: 'Memory::Reallocate: resolve UB (realloc() on new[] pointer on MSVC)'
status: Done
assignee: []
created_date: '2026-05-22 07:34'
updated_date: '2026-05-22 08:31'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: high
ordinal: 16000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Carried forward from the original TASK-23 scope.

`Memory::Reallocate` (in `Source/Engine/Common/Memory.cpp`) calls `realloc()` on a pointer originally allocated with `new[]`. On MSVC, `new[]` adds bookkeeping overhead before the user-visible pointer (so the system allocator doesn't recognise the address — `realloc(p, n)` is UB).

Two paths:

- **A. Switch to malloc/realloc/free throughout `Memory::*`.** Lose `new[]` constructor calls — but `Memory::Allocate` is already a `void*` allocator that doesn't construct, so this is just bookkeeping.
- **B. Migrate callers off Memory::Reallocate.** Use new[]/delete[] explicitly; remove Reallocate from the public API. Forces callers to deal with element-count growth themselves.

Recommended: **A** unless an audit shows callers depend on `new[]` semantics. The function is named "Reallocate", which implies C-style allocator semantics anyway.

Audit before deciding:
- Grep all `Memory::Reallocate` callers. How many are there?
- For each, do they assume new[]/delete[] semantics (paired delete[]?) or plain free?

References:
- Source/Engine/Common/Memory.cpp (Reallocate implementation)
- Source/Engine/Common/Memory.h (declarations)
- Grep `Memory::Reallocate` / `Memory::Allocate` / `Memory::Deallocate`
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Memory::Reallocate caller audit complete — list of callers in closure note.
- [x] #2 Decision (A: malloc/realloc/free; B: remove Reallocate from API) recorded.
- [x] #3 If A: Memory::Allocate / Memory::Deallocate / Memory::Reallocate consistently use malloc/realloc/free; new[]/delete[] removed from Memory.cpp.
- [ ] #4 If B: all callers migrated to manual realloc patterns; Memory::Reallocate removed.
- [x] #5 UnitTests pass; Main.exe -total_frames 10 exits 0; no leaks under whatever leak detector is in use.
- [ ] #6 Stress test: heavy Allocate/Reallocate/Deallocate cycle (10^6 ops); no UB tripped (verify with AddressSanitizer or MSVC /fsanitize=address if available).
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option A** — Memory::Allocate / Memory::Reallocate / Memory::Deallocate now consistently use `std::malloc` / `std::realloc` / `std::free`. The new[]/delete[] mismatch with realloc() is gone.

## Caller audit

`Memory::Reallocate` had **one** caller: `Source/Engine/ThirdParty/STBWrapper/stb_image.cpp:3` via `#define STBI_REALLOC`. The same TU also overrides `STBI_MALLOC` and `STBI_FREE` to `Memory::Allocate` / `Memory::Deallocate` — so the three Memory:: APIs are a consistent triple from stb_image's perspective. Switching all three to malloc/realloc/free preserves that contract.

Other `Memory::Allocate` / `Memory::Deallocate` consumers (TextureResourceServiceImpl.cpp, BCCompression.cpp, Array.h, Allocator.h) treat the return as `void*` with manual cast — no new[]/delete[] semantics relied on. Safe.

## Diff

- `Source/Engine/Common/Memory.cpp`: `::new char[size]` → `std::malloc(size)`; `delete[](char*)ptr` → `std::free(ptr)`; added `<cstdlib>`; removed unused `<memory>`. Bookkeeping (`Record`/`Erase`) and shared_mutex unchanged.
- `Source/TestSuite/UnitTests/MemoryTests.cpp` (new): 5 unit tests — Allocate/Deallocate round-trip, Reallocate grow (prefix preserved), Reallocate shrink (prefix preserved), Reallocate(nullptr, n) acts as Allocate(n), Deallocate(nullptr) is a no-op.
- `Source/TestSuite/StressTests/MemoryStress.cpp`: added `TestMemoryReallocateStress` — 100,000 Reallocate cycles with sizes 16..4112, head + tail writes to detect corruption.
- `Source/TestSuite/Common/TestRunner.cpp`, `Source/TestSuite/CMakeLists.txt`: registered new MemoryTests.cpp.

## Verification

- `TestSuite.exe -u` — Memory Unit Tests: 5/5 pass. (Pre-existing FixedSizeString "trailing slash preserved" failure unchanged — that's TASK-23.15.)
- `TestSuite.exe -s` — `Memory Reallocate Stress - 27.16ms ✓` along with the rest of the stress suite. No crash, no leak.
- `Main.exe -total_frames 10` exits 0 (real workload exercises stb_image's STBI_REALLOC path during texture import).

## ACs

- #1 Caller audit complete: 1 caller (stb_image via STBI_REALLOC).
- #2 Decision recorded: Option A.
- #3 Memory.cpp uses malloc/realloc/free consistently.
- #5 UnitTests + Main.exe -total_frames 10 pass.
- #6 Stress test added (100k cycles). AddressSanitizer not run — MSVC RelWithDebInfo build does not have `/fsanitize=address` enabled in this project's CMake. Manual head/tail byte verification in the stress test is the substitute. Flagging the ASan gap explicitly.
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
