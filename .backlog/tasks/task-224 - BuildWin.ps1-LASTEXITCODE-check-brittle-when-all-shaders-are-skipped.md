---
id: TASK-224
title: BuildWin.ps1 LASTEXITCODE check brittle when all shaders are skipped
status: Done
assignee: []
created_date: '2026-05-14 02:55'
updated_date: '2026-05-14 09:10'
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

- [x] #1 `BuildWin.ps1` invoked from a fresh shell with `$LASTEXITCODE` unset or non-zero proceeds correctly when all shaders are up-to-date
- [x] #2 Pre-step failure still aborts the build (the load-bearing check stays load-bearing)
- [x] #3 `Test-BuildWinDxilPrestep.ps1`'s `$global:LASTEXITCODE = 0` workaround can be removed (or its purpose re-justified)

## Why low priority

Doesn't trip in normal `BuildWin.ps1` invocations (the outer shell usually inherits a clean state). The workaround in the test driver covers the one observed failure path.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

### 2026-05-14 — closure

Picked option (2) from the fix shapes — narrow init `$global:LASTEXITCODE = 0` immediately before `& HLSL2DXIL.ps1 -NoPause` at `Scripts/BuildWin.ps1:102`, with a 3-line WHY comment at `Scripts/BuildWin.ps1:99-101`. Option (1) (top-of-script init) was rejected as broader than needed; option (3) (HLSL2DXIL.ps1 returns a value) was rejected as a cross-script contract change for a low-priority issue.

Verification:
- `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` PASS — bumped-source case still triggers compile, DXIL `LastWriteTimeUtc` advances, source mtime restored. Recompile path unaffected.
- Brittle-case repro: fresh `powershell.exe -NoProfile`, salted `$LASTEXITCODE = 42` via `cmd /c exit 42`, then post-fix pre-step block invoked with all shaders up-to-date. `$LASTEXITCODE = 0` after the call. Gate at `Scripts/BuildWin.ps1:104` does not trip. Repro script not committed (`Build/` gitignored).

AC #3 disposition: re-justified, not removed. Test driver's `$global:LASTEXITCODE = 0` at `Scripts/Tests/Test-BuildWinDxilPrestep.ps1:102` is independently needed for `Set-StrictMode -Version Latest` on-read protection — existing inline comment at `Scripts/Tests/Test-BuildWinDxilPrestep.ps1:97-101` already documents this. Different concern from the BuildWin brittleness.

Not verified: a full `Scripts/BuildWin.ps1` invocation chained through to msbuild post-fix. The brittle-case symptom is fully captured by the pre-step block alone (lines 95-105); msbuild is a separate code path downstream of the gate and untouched by this change.

### Review (ci-build-impl, 2026-05-14)

**Verdict: PASS.**

Fix shape #2 (narrow init right before the pre-step call) picked, applied at `Scripts/BuildWin.ps1:102` with a 3-line WHY comment at `Scripts/BuildWin.ps1:99-101`.

**AC walkthrough:**

- **AC #1 — fresh-shell with salted `$LASTEXITCODE`**: `Scripts/BuildWin.ps1:102` writes 0 unconditionally before the call. On the all-skipped path, no native process runs inside `Invoke-HlslToDxil`, so `$LASTEXITCODE` retains the init. Implementer's `cmd /c exit 42` repro confirmed. Holds.
- **AC #2 — pre-step failure still aborts**: walked the two failure modes.
  - dxc compile error: `Scripts/HLSL2DXIL.ps1:24` sets `$ErrorActionPreference='Stop'`; the module throws, propagates as a terminating error through `& $preStep` under `Scripts/BuildWin.ps1:43`'s own `Stop` preference. Script terminates before reaching the `$LASTEXITCODE` gate. Build aborts.
  - Hypothetical dxc non-zero-without-throw: dxc's exit overwrites the init 0; the gate at `Scripts/BuildWin.ps1:104` trips with the real exit code. Build aborts.
  - Holds.
- **AC #3 — test-driver workaround**: `Scripts/Tests/Test-BuildWinDxilPrestep.ps1:102` keeps `$global:LASTEXITCODE = 0`; the inline justification at `Scripts/Tests/Test-BuildWinDxilPrestep.ps1:97-101` cites Set-StrictMode-on-read protection. Independent of BuildWin's bug — the driver runs under `Set-StrictMode -Version Latest` (`Scripts/Tests/Test-BuildWinDxilPrestep.ps1:32`), BuildWin does not. Re-justification stands.

**Unintended-interaction sweep:**

- `-SkipShaderCompile` branch (`Scripts/BuildWin.ps1:95-96`): untouched, no interaction.
- `-BuildWithNRD` reconfigure (`Scripts/BuildWin.ps1:58-70`): captures `$cmakeExit = $LASTEXITCODE` immediately, before the new init runs. No interaction.
- msbuild path (`Scripts/BuildWin.ps1:110-122`): `Invoke-MsBuild`'s `return $LASTEXITCODE` (`Scripts/BuildWin.ps1:84`) is captured per-call. Init is overwritten by the first msbuild invocation. No interaction.
- clangd post-step (`Scripts/BuildWin.ps1:131-155`): gates on `$exitCode`, not `$LASTEXITCODE`. No interaction.
- Test driver's static wiring check regex `'HLSL2DXIL\.ps1.*-NoPause'` (`Scripts/Tests/Test-BuildWinDxilPrestep.ps1:60`): still matches `Scripts/BuildWin.ps1:103`. No interaction.

**Comment hygiene (`comment-discipline`):** the 3-line comment at `Scripts/BuildWin.ps1:99-101` documents WHY (`$LASTEXITCODE` is whatever the outer shell entered with on the all-skipped path) — a non-obvious, shell-state-dependent failure mode. Concise, no restatement of the code. Passes.

**Scripts/CLAUDE.md PowerShell 5.1 conventions:** the new strings are pure ASCII; only `$global:` scope is used in an assignment (not a double-quoted interpolation). Compliant.

**Findings: none.** No ADVISORY items.

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — N/A for PowerShell scripts; static wiring check (phase 0 of `Test-BuildWinDxilPrestep.ps1`) confirms `BuildWin.ps1` still parses and contains the expected `HLSL2DXIL.ps1 -NoPause` invocation pattern.
- [x] #2 `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` re-run post-fix — PASS (1/1). Recompile path: `BRDFLUTPass.comp` source bumped to 2026-05-14 09:04:26, DXIL `LastWriteTimeUtc` advanced 2026-05-10 02:54:02 → 2026-05-14 07:04:27, source mtime restored.
- [x] #3 Pre-existing integration test (`Test-BuildWinDxilPrestep.ps1`) covers the changed area. New test not written — see #5 for the brittle-case-specific repro.
- [x] #4 Validation is real-process: `Test-BuildWinDxilPrestep.ps1` invokes real `HLSL2DXIL.ps1` + `dxc.exe` + real DXIL files. The brittle-case repro invokes the actual `HLSL2DXIL.ps1` script in a fresh `powershell.exe` process. No mock-based tests.
- [x] #5 Terminal transcript captured: fresh `powershell.exe -NoProfile`, `cmd /c exit 42` salts `$LASTEXITCODE` to 42, post-fix pre-step block clears it to 0 before the call, all-skipped pre-step leaves it at 0, gate check passes.
- [x] #6 Not verified: full `BuildWin.ps1 → msbuild` end-to-end chain post-fix. The brittle-case symptom is fully captured by the pre-step block alone (lines 95-105); msbuild is downstream of the gate and untouched. Stated honestly above.
<!-- DOD:END -->
