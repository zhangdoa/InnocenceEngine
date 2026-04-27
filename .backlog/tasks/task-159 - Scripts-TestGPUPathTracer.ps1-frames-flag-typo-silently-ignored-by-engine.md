---
id: TASK-159
title: 'Scripts/TestGPUPathTracer.ps1: -frames flag typo (silently ignored by engine)'
status: To Do
assignee: []
created_date: '2026-04-27 15:14'
labels:
  - infrastructure
  - testing
  - bug
dependencies: []
references:
  - Scripts/TestGPUPathTracer.ps1
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Spotted while fixing TASK-158 in `Scripts/TestGIScene.ps1`. The sibling script `Scripts/TestGPUPathTracer.ps1` has the same `-frames N` typo: the engine flag is `-total_frames` (parsed in `Source/Engine/Engine.cpp`), so the param is silently ignored and the engine never auto-terminates within the script's window.

Loglevel side is fine (it already uses `-loglevel 0`, which is below Success and lets the grep markers fire), so this task is just the flag rename.

Out of scope from TASK-158 to keep that CL single-purpose; filed as a follow-up.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Fix `-frames N` -> `-total_frames N` in `Scripts/TestGPUPathTracer.ps1`
- [ ] #2 Self-test: run the script against the current engine, assert PASS
- [ ] #3 Add the same sync-warning comment block as TestGIScene.ps1 explaining why the loglevel + flag knobs must stay aligned with the engine
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
