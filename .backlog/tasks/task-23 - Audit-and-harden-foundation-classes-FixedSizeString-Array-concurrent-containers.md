---
id: TASK-23
title: >-
  Audit and harden foundation classes: FixedSizeString, Array, concurrent
  containers
status: To Do
assignee: []
created_date: '2026-04-13 11:25'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Several foundation classes in Source/Engine/Common/ were written early in the project and have not been systematically reviewed for correctness. A recent bug (FixedSizeString empty-string buffer underflow: `m_content[-1]` write when `strlen(content)==0`) caused heap corruption in a runtime test and went undetected for years.

The goal is a systematic correctness audit of the most critical foundation classes, fixing any confirmed bugs. Candidates:
- **FixedSizeString** (`FixedSizeString.h`): off-by-one in null termination (`m_content[strlen-1]` instead of `m_content[strlen]`) changes the semantics of every stored string — the last character is silently dropped. This survived because the bug is applied symmetrically on both write and read through FixedSizeString, but it means every component name in JSON is silently wrong. The minimal empty-string fix was applied in commit 0668dbcd; the off-by-one for non-empty strings is tracked here.
- **Array** (`Array.h`): check bounds, resize, iterator safety.
- **ObjectPool** (`ObjectPool.h`): check for use-after-free, alignment, double-free guards.
- **ThreadSafeQueue / ThreadSafeVector / ThreadSafeUnorderedMap**: verify lock discipline, correctness of size() vs internal state.
- **Memory** (`Memory.cpp`): `Memory::Reallocate` calls `realloc()` on a pointer allocated with `new[]` — this is UB on MSVC where `new[]` adds overhead before the user pointer. Check all callers.

**Caution for FixedSizeString off-by-one**: fixing `m_content[strlen-1]` → `m_content[strlen]` changes what is stored to disk (component instance names in JSON). All generated asset files (Data/Generated/) would need to be re-imported after the fix. Existing hand-authored Res/ files must be audited to confirm they do not rely on the truncated names. Do NOT fix this without a full regression test pass.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Each audited class has a written list of issues found (none / minor / critical).
- [ ] #2 All confirmed bugs with undefined behaviour or data corruption are fixed.
- [ ] #3 FixedSizeString off-by-one is either fixed with full regression pass, or explicitly documented as a known-safe quirk with a comment explaining why.
- [ ] #4 Memory::Reallocate UB is resolved (either use new[]/delete[] consistently, or switch to malloc/realloc/free).
- [ ] #5 All existing unit tests (UnitTests/) continue to pass after changes.
- [ ] #6 RenderTest.exe exits 0 and Main.exe 10-frame integration test exits 0 after all fixes.
<!-- AC:END -->
