---
id: TASK-224
title: BuildWin.ps1 LASTEXITCODE check brittle when all shaders are skipped
status: To Do
assignee: []
created_date: '2026-05-14 02:55'
labels:
  - build
  - scripts
  - bug
  - tooling
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

`Scripts/BuildWin.ps1`'s post-pre-step exit-code check is brittle. When all shaders are already up-to-date and `Invoke-HlslToDxil` in `Lib/Compile-HLSL.psm1` skips every shader (no `dxc.exe` spawn), `$LASTEXITCODE` is never assigned by the pre-step. If the outer shell entered the script with `$LASTEXITCODE = $null` or a non-zero value from a prior command, the `if ($LASTEXITCODE -ne 0)` check trips spuriously and aborts the build.

Pre-existing — same shape in the pre-TASK-212 `HLSL2DXIL_NoPause.ps1` invocation.

## Discovered

Surfaced during TASK-212 Phase 2 (HLSL pair consolidation, commit `62b85b19`, 2026-05-14). Agent flagged it as "surfaced finding, not folded in" per `surface-dont-chase`.

## Workaround already in place

`Scripts/Tests/Test-BuildWinDxilPrestep.ps1` explicitly sets `$global:LASTEXITCODE = 0` before invoking BuildWin to prevent the false-positive. This works around the symptom in the test driver but doesn't fix the root cause.

## Fix shape

Options:
1. Initialize `$LASTEXITCODE = 0` at the top of `BuildWin.ps1` (matches the test-driver's workaround pattern).
2. Initialize before the pre-step call specifically — narrows the scope.
3. Use a different exit-code propagation mechanism: capture the pre-step's exit via a return value, not `$LASTEXITCODE`.

Pick during dispatch. (1) or (2) is the minimal fix; (3) is the structurally cleaner shape.

## Acceptance criteria

- [ ] #1 `BuildWin.ps1` invoked from a fresh shell with `$LASTEXITCODE` unset or non-zero proceeds correctly when all shaders are up-to-date
- [ ] #2 Pre-step failure still aborts the build (the load-bearing check stays load-bearing)
- [ ] #3 `Test-BuildWinDxilPrestep.ps1`'s `$global:LASTEXITCODE = 0` workaround can be removed (or its purpose re-justified)

## Why low priority

Doesn't trip in normal `BuildWin.ps1` invocations (the outer shell usually inherits a clean state). The workaround in the test driver covers the one observed failure path.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
