---
id: TASK-23.15
title: >-
  FixedSizeString: fix m_content[strlen-1] off-by-one + migrate Data/Generated +
  audit Res/
status: To Do
assignee: []
created_date: '2026-05-22 07:34'
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
- [ ] #1 Decision (A/B/C) recorded with reason.
- [ ] #2 If A: m_content[strlen-1] changed to m_content[strlen]; all read paths updated to match (no compensating truncate-on-read).
- [ ] #3 If A: Data/Generated/ migration script written and run; before/after diff sampled.
- [ ] #4 If A: Res/ audited; any hand-authored dependencies on truncated names migrated.
- [ ] #5 If A: cross-boundary `const char*` <-> `FixedSizeString.c_str()` LUT-key callers (AssetService Mesh/Texture/Material) no longer need the post-truncation workaround from commit 9a42a43b — revert that workaround.
- [ ] #6 If B: class renamed to announce the contract; comment block on the class explaining the quirk.
- [ ] #7 UnitTests/FixedSizeStringTests pass; new regression test for the empty-string + maximum-length boundary.
- [ ] #8 RenderTest.exe and Main.exe -total_frames 10 both exit 0 after migration.
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
