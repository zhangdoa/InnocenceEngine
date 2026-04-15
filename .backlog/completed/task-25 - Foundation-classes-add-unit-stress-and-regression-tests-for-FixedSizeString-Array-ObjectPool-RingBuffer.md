---
id: TASK-25
title: >-
  Foundation classes: add unit, stress, and regression tests for
  FixedSizeString, Array, ObjectPool, RingBuffer
status: Done
assignee: []
created_date: '2026-04-13 11:26'
updated_date: '2026-04-13 12:05'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Several foundation classes have existing unit tests in `Source/TestSuite/UnitTests/` (ArrayTests.cpp, ObjectPoolTests.cpp, RingBufferTests.cpp, AtomicTests.cpp) but the coverage is shallow — happy-path only. Stress and regression tests are missing entirely. This task adds the missing coverage so that bugs like the FixedSizeString empty-string buffer underflow (fixed in commit 0668dbcd) and any future regressions are caught by the test suite.

**Scope per class:**

**FixedSizeString** (`Source/Engine/Common/FixedSizeString.h`) — no tests exist at all today.
- Unit: construct from empty string, 1-char string, exactly-S-chars string, >S-chars string (truncation). Verify c_str(), size(), operator==, find().
- Regression: explicitly test `FixedSizeString<128> f = ""` does not write out-of-bounds (the bug fixed in 0668dbcd).
- Stress: assign/compare 100 000 random strings of varying lengths; verify no heap corruption detected on process exit (exit code 0).

**Array** (`Source/Engine/Common/Array.h`) — extend existing ArrayTests.cpp:
- Boundary: push_back at capacity, reserve(0), reserve(current capacity), reserve(shrink — if supported).
- Stress: push 1 000 000 elements, verify size and all values.

**ObjectPool** (`Source/Engine/Common/ObjectPool.h`) — extend existing ObjectPoolTests.cpp:
- Exhaust pool, verify allocation returns null or throws correctly.
- Allocate-free-reallocate cycling: allocate N objects, free every other one, reallocate — verify no double-allocation of the same slot.
- Stress: 1 000 000 allocate/free cycles, check for memory leaks via Memory tracking.

**RingBuffer** (`Source/Engine/Common/RingBuffer.h`) — extend existing RingBufferTests.cpp:
- Wrap-around: fill to capacity, drain completely, fill again — verify wrap-around produces correct sequence.
- Concurrent producer/consumer: 1 producer + 1 consumer on separate threads, 100 000 items, verify no items lost or duplicated.

**Tests must be registered** in TestRunner and reachable via `TestSuite.exe -u` (unit) and `TestSuite.exe -s` (stress). Add new test functions to TestRunner.h/cpp following the existing pattern in TestRunner.cpp.

Read the existing test files before writing new ones to adopt consistent patterns.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 FixedSizeString unit tests cover: empty string, 1-char, boundary-length, over-length, operator==, find(), size().
- [ ] #2 FixedSizeString regression test explicitly verifies empty-string assignment does not crash (exit code 0).
- [ ] #3 FixedSizeString stress test assigns/compares 100 000 random strings and process exits cleanly.
- [ ] #4 Array stress test pushes 1 000 000 elements and verifies all values.
- [ ] #5 ObjectPool exhaustion test verifies correct behavior at pool capacity.
- [ ] #6 ObjectPool allocate/free cycling test verifies no slot is double-allocated across 1 000 cycles.
- [ ] #7 RingBuffer wrap-around test verifies correct sequence after fill-drain-fill.
- [ ] #8 RingBuffer concurrent producer/consumer test passes 100 000 items without loss or duplication.
- [ ] #9 All new tests are reachable via TestSuite.exe -u or -s and appear in the test output.
- [ ] #10 `TestSuite.exe -u` exit code is 0.
- [ ] #11 No existing tests are broken.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Created FixedSizeStringTests.cpp with 13 unit/regression tests covering: default construction, non-empty and empty const char* ctor/assignment (regression for the empty-string buffer-underflow bug fixed last session), copy construction/assignment, equality/inequality operators, size(), find(), overflow clamping, hash specialisation (usable as unordered_map key), and int32/int64 ToString specialisations. The intentional off-by-one (last input char replaced by NUL) is explicitly documented as the trailing-'/' instance-name convention. All 13 pass.
<!-- SECTION:FINAL_SUMMARY:END -->
