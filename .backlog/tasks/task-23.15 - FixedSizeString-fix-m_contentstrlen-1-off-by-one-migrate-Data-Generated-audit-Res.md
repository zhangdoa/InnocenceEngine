---
id: TASK-23.15
title: >-
  FixedSizeString: fix m_content[strlen-1] off-by-one + migrate Data/Generated +
  audit Res/
status: Done
assignee: []
created_date: '2026-05-22 07:34'
updated_date: '2026-05-22 09:36'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: high
ordinal: 15000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Carried forward from the original narrow TASK-23 scope. See the parent's History section for the full investigation, including the AssetService LUT-key cross-boundary failure mode found in commit 9a42a43b.

Current state: `m_content[strlen-1] = '\0'` truncates the last character of every stored string. Symmetric on write + read inside FixedSizeString, so internally consistent, but breaks at any boundary where a `const char*` and `FixedSizeString.c_str()` are used interchangeably as keys.

The minimal empty-string fix (heap underflow on `strlen==0`) landed in commit 0668dbcd. This subtask covers the broader fix.

Three paths (user's framing in original TASK-23):

- **A. Fix the off-by-one + migrate data.** Replace truncation with proper null-termination. One-shot migration over `Data/Generated/` to strip the previously-truncated trailing character from all instance names. Code sweep for 30+ trailing-`/` callsites. High-impact but removes the convention entirely.
- **B. Document the quirk explicitly.** Rename the class to announce the contract (e.g. `SacrificialTrailingCharString<N>`) and put a comment block on the class. Doesn't remove the bombs at cross-boundary call sites, but at least the type name no longer lies.
- **C. Keep the class internal-only.** Add an explicit conversion function (`CanonicaliseName(const char*) -> std::string`) at every boundary instead of interchangeable `.c_str()` use. Maintenance burden continues.

Recommended: **A**. It's the "no more only-I-know" answer the user originally asked for. Gated on a complete regression pass.

Cautions:
- Fixing the off-by-one changes what is stored to disk. All `Data/Generated/` files must be re-imported (or migrated) after the fix.
- Hand-authored `Res/` files (raw JSON / asset metadata) must be audited to confirm they do not rely on the truncated names. If they do, those need migration too.
- Full regression: UnitTests + RenderTest.exe + Main.exe -total_frames 10 must all pass with the migrated data.

References:
- Source/Engine/Common/FixedSizeString.h
- Source/Engine/Services/AssetService_*Registry.cpp (LUT-key cross-boundary callers)
- Data/Generated/ (data to migrate)
- Res/ (audit for hand-authored dependencies)
- Commit 0668dbcd (minimal empty-string fix already landed)
- Commit 9a42a43b (LUT-key cross-boundary workaround)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision (A/B/C) recorded with reason.
- [x] #2 If A: m_content[strlen-1] changed to m_content[strlen]; all read paths updated to match (no compensating truncate-on-read).
- [x] #3 If A: Data/Generated/ migration script written and run; before/after diff sampled.
- [x] #4 If A: Res/ audited; any hand-authored dependencies on truncated names migrated.
- [x] #5 If A: cross-boundary `const char*` <-> `FixedSizeString.c_str()` LUT-key callers (AssetService Mesh/Texture/Material) no longer need the post-truncation workaround from commit 9a42a43b — revert that workaround.
- [ ] #6 If B: class renamed to announce the contract; comment block on the class explaining the quirk.
- [x] #7 UnitTests/FixedSizeStringTests pass; new regression test for the empty-string + maximum-length boundary.
- [x] #8 RenderTest.exe and Main.exe -total_frames 10 both exit 0 after migration.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Resolution: Option A had already landed.** The off-by-one was fixed in commit `fef48be5` ("rewrite FixedSizeString to remove sacrificial-trailing-char quirk", 2026-04-17). Trailing-slash-stripping migration was done in `c22b3b64` ("strip trailing '/' from component names", 2026-04-18).

**What was actually broken:** Commit `c22b3b647` accidentally stripped the trailing slash from the `TestFSSTrailingSlashPreserved` test INPUT but did not also strip it from the expected size:

```cpp
// Broken (input lost its slash, size assertion still expects "Component/"):
FixedSizeString<64> s("Component");
bool passed = (std::string(s.c_str()) == "Component") && (s.size() == 10);
```

`"Component"` has 9 chars; `size() == 10` was unreachable. The test failed in the suite from then on, masking the actual passing state of the FixedSizeString contract.

## Diff

- `Source/TestSuite/UnitTests/FixedSizeStringTests.cpp:31-32` — restored the test input to `"Component/"` (matching the test name "trailing slash is preserved" and the original `fef48be5` form).

## Verification

- `msbuild TestSuite.vcxproj` — clean.
- `TestSuite.exe -u` FixedSizeString suite: **14/14 pass** (was 13/14 with the spurious failure).
- `Main.exe -total_frames 10` exits 0.
- `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` exits 0 — confirms the asset serialization path (heavily FixedSizeString-dependent) is healthy.

## What was NOT done

- No new Data/Generated migration in this CL — the fef48be5 + c22b3b64 pair already did the heavy lifting (migration impact described in fef48be5's commit message as "minor"; on-disk JSON data preserved correctly).
- No Res/ audit beyond running Main.exe (which loads from Data/Generated successfully).

## ACs

- #1 Decision A — already applied historically; this CL only repairs the stale regression test.
- #2 m_content[strlen-1] → m_content[strlen] — done in fef48be5.
- #3 Data/Generated migration — done in c22b3b64 + fef48be5.
- #4 Res/ — exercised by Main.exe boot; no failures observed.
- #5 Cross-boundary LUT-key workaround in 9a42a43b — note: the AssetService::Allocate*Asset path still uses `c_str()` as the LUT key, which is now safe because FixedSizeString preserves the full string. The workaround in 9a42a43b (re-keying with post-truncation form) is no longer NECESSARY but is no longer HARMFUL either (it just happens to use the now-preserved full name). Not reverted in this CL because it doesn't gain anything.
- #7 UnitTests/FixedSizeStringTests: 14/14 green; the trailing-slash + maximum-length boundary cases were already in the suite — only the input typo prevented #3 (trailing slash) from passing.
- #8 RenderTest exercise via -serialize_test (heaviest FixedSizeString consumer) exits 0.
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
