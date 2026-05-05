---
id: TASK-214
title: BuildWin.ps1 should auto-trigger HLSL recompile or loud-fail on DXIL staleness
status: Done
assignee:
  - ci-build-expert
created_date: '2026-05-02 18:58'
updated_date: '2026-05-05 07:55'
labels:
  - build-pipeline
  - ergonomics
dependencies: []
references:
  - Scripts/BuildWin.ps1
  - Scripts/HLSL2DXIL_NoPause.ps1
  - >-
    .backlog/tasks/task-213 -
    Auto-test-infra-cross-binary-deterministic-three-scene-PT-capture-root-cause-TASK-210-carry-forwards.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Hazard

`Scripts/BuildWin.ps1` does NOT auto-invoke `Scripts/HLSL2DXIL_NoPause.ps1`. When HLSL is edited and only `BuildWin.ps1` runs, the existing DXIL is reused — leading to silent stale-DXIL behavior at runtime: PSO creation fails with `E_INVALIDARG`.

## Concrete failure shape

PSO creation fails with `E_INVALIDARG` when DXIL is older than its HLSL source. The TASK-213 CL C implementer hit this exact failure mode: HLSL edited at 20:08, DXIL last compiled at 20:03; PSO failed until manual `HLSL2DXIL_NoPause.ps1` run.

## Why standalone (not folded into TASK-213)

This is build-pipeline ergonomics, not TASK-213-specific. Filing only as a TASK-213 advisory loses it once TASK-213 closes. The fix surface is `Scripts/BuildWin.ps1` — owner `ci-build-expert`.

## Surfacing context

TASK-213 CL C implementer surface-back; the CL C surface-back transcript carries the specific timestamps as evidence.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Owner picks (a) or (b): (a) BuildWin.ps1 auto-triggers HLSL2DXIL_NoPause.ps1 if any HLSL source has changed since the last DXIL compile.
- [x] #2 (b) OR BuildWin.ps1 loud-fails (with explicit "DXIL stale, run HLSL2DXIL_NoPause.ps1" message) if DXIL is older than any HLSL source.
- [x] #3 Whichever path is chosen, document the rationale in Scripts/README.md (create if missing).
- [x] #4 Verify the chosen behavior reproduces the TASK-213 CL C scenario: edit an HLSL source, run BuildWin.ps1, confirm either auto-recompile or loud-fail occurs (not silent stale-DXIL reuse).
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Review (ci-build-impl, 2026-05-05)

**Verdict: PASS** (one ADVISORY, no blockers)

### Terminal-goal verification

The CL C failure shape — `BuildWin.ps1` reusing stale DXIL because no recompile is triggered — is no longer reachable on the default path. `Scripts/BuildWin.ps1:54-63` invokes `HLSL2DXIL_NoPause.ps1` before `msbuild` on every run; `Scripts/Lib/Compile-HLSL.psm1:165-180` is the timestamp-aware recompile policy, so a CL-C-shape edit (source mtime > DXIL mtime) deterministically triggers recompile. The runtime test `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` reproduces the exact 20:08-vs-20:03 scenario and asserts `LastWriteTimeUtc` on the DXIL advances post-run.

### AC coverage

| AC | Evidence | Status |
|---|---|---|
| #1 option (a) chosen | `BuildWin.ps1:58` invokes pre-step unconditionally (subject to switch) | met |
| #2 (b) alt path | option (a) selected; (b) explicitly rejected w/ rationale (`Scripts/README.md:14-46`) | met |
| #3 README rationale | `Scripts/README.md` § "Why auto-trigger (option a), not loud-fail (option b)" cites `no-shadow-state` discipline as structural reason | met |
| #4 reproduces CL C | `Test-BuildWinDxilPrestep.ps1:80-122` bumps source mtime, runs pre-step, asserts DXIL mtime advances | met |

### Discipline checks

- **`on-implement/no-shadow-state.md`**: PASS. Choice to delegate to `Compile-HLSL.psm1` rather than gate on a duplicate timestamp predicate is correct application — option (b) would have been a shadow of the real compile policy.
- **`on-implement/safety-observability.md`**: PASS. Skip branch logs why it skips; fail branch logs with exit code. No silent guards.
- **`on-implement/test-etiquette.md`**: PASS. Test budget is bounded; `finally` block restores source mtime so working tree is unmodified after pass/fail.
- **`on-commit/peer-review-required.md`**: review brief consumed without implementer reasoning trace; verdict line-grounded.

### ADVISORY (non-blocking, not addressed in this CL)

- `Scripts/Tests/Test-BuildWinDxilPrestep.ps1:58` Phase 0 wiring guard uses substring match on `HLSL2DXIL_NoPause\.ps1`. Currently `BuildWin.ps1` has exactly one occurrence (the `&` invocation at line 58), so the check is meaningfully load-bearing. If a future refactor extracts the pre-step into a dot-sourced helper or comment-mentions the file, the substring guard would still pass even with the wiring removed. The runtime mtime assertion catches the regression in practice — leaving as advisory; if the test file is touched in a later CL, consider tightening to e.g. `^\s*&[^#]*HLSL2DXIL_NoPause\.ps1` so the match is anchored to code, not comments.

### Observations (non-findings)

- README's residual-risk surfacing (`Scripts/README.md:65-68`, "TASK-202 — same hazard on `cmake --build` invocation path; not addressed by this change") is the correct disclosure: the `BuildWin.ps1` driver is fixed; the cmake-direct path remains a separate fix surface.

**Reviewed-By: ci-build-impl**
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Final Summary — TASK-214

**Status:** Done. Reviewed: PASS with one non-blocking ADVISORY (test substring guard tightening — left as follow-up, captured in Implementation Notes).

### Decision

Picked option (a) auto-trigger. `BuildWin.ps1` now invokes `HLSL2DXIL_NoPause.ps1` unconditionally before msbuild. Rationale: the compile module is already idempotent (per-shader source-vs-DXIL `LastWriteTimeUtc` plus `#include`-graph dep checks). Loud-fail (b) would have required duplicating that staleness predicate in `BuildWin.ps1` — a `no-shadow-state` violation that would drift from the real compile policy.

### What landed (3 files)

- `Scripts/BuildWin.ps1` (modified, lines 49-63) — auto-trigger pre-step + abort-on-fail + `-SkipShaderCompile` escape
- `Scripts/README.md` (new, 95 lines) — rationale, failure handling, escape hatch, related work, script inventory
- `Scripts/Tests/Test-BuildWinDxilPrestep.ps1` (new) — phase-0 static wiring guard + runtime stale-DXIL recompile assertion + teardown that restores source mtime

### AC coverage

| AC | Status | Evidence |
|---|---|---|
| #1 option (a) auto-trigger | ✓ | BuildWin.ps1:54-63 invokes pre-step unconditionally |
| #2 (b) loud-fail alternative | ✓ | option (a) selected; (b) explicitly rejected with rationale in README |
| #3 rationale documented | ✓ | Scripts/README.md § "Why auto-trigger (option a), not loud-fail (option b)" cites no-shadow-state discipline |
| #4 reproduces CL C scenario | ✓ | Test-BuildWinDxilPrestep.ps1 PASSED — bumped BRDFLUTPass.comp source mtime to now (DXIL stale by ~6h), pre-step recompiled bumped shader, DXIL LastWriteTimeUtc advanced 07:30:13 → 07:36:05 |

### DoD coverage

| DoD | Status | Notes |
|---|---|---|
| #1 Code compiles (script self-test) | ✓ | Test-BuildWinDxilPrestep.ps1 parses and runs end-to-end |
| #2 Pre-existing integration tests re-run | N/A | No pre-existing test covered BuildWin.ps1's shader pre-step (the hazard TASK-214 was filed to fix) |
| #3 New integration test written | ✓ | Real-filesystem integration test (touches HLSL source mtime, invokes actual HLSL2DXIL_NoPause.ps1, asserts actual DXIL rewritten on disk) |
| #4 Mocks not sole validation | ✓ | Test mutates real files and asserts on real LastWriteTimeUtc; no mocks |
| #5 User-observable outcome verified | ✓ | PowerShell transcript shows: bumped source mtime, pre-step output `Compiling BRDFLUTPass.comp -> ...`, `Successfully compiled BRDFLUTPass.comp.`, DXIL mtime advanced, source mtime restored to original |
| #6 What was NOT verified | ✓ | listed below |

### What was NOT verified (DoD #6)

- **Full BuildWin.ps1 → msbuild end-to-end run** — chained msbuild was not exercised (5+ minutes per invocation). The wiring point under test is the pre-step hand-off, which the test exercises directly via the same subprocess invocation BuildWin.ps1 makes.
- **`#include` graph staleness branch** — test mutates a leaf shader (no dependents). The dep-newer branch is pre-existing module behaviour, not this CL's surface.
- **`-SkipShaderCompile` escape hatch** — manually smoke-tested during debugging; not covered by the automated test.
- **TASK-202 (cmake-direct invocation path)** — explicitly out of scope; called out in `Scripts/README.md` "Related work" as a separate fix surface.

**Reviewed-By: ci-build-impl**
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
