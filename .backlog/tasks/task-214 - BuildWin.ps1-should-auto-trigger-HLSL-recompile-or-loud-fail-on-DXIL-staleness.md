---
id: TASK-214
title: BuildWin.ps1 should auto-trigger HLSL recompile or loud-fail on DXIL staleness
status: To Do
assignee:
  - ci-build-expert
created_date: '2026-05-02 18:58'
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
- [ ] #1 Owner picks (a) or (b): (a) BuildWin.ps1 auto-triggers HLSL2DXIL_NoPause.ps1 if any HLSL source has changed since the last DXIL compile.
- [ ] #2 (b) OR BuildWin.ps1 loud-fails (with explicit "DXIL stale, run HLSL2DXIL_NoPause.ps1" message) if DXIL is older than any HLSL source.
- [ ] #3 Whichever path is chosen, document the rationale in Scripts/README.md (create if missing).
- [ ] #4 Verify the chosen behavior reproduces the TASK-213 CL C scenario: edit an HLSL source, run BuildWin.ps1, confirm either auto-recompile or loud-fail occurs (not silent stale-DXIL reuse).
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
