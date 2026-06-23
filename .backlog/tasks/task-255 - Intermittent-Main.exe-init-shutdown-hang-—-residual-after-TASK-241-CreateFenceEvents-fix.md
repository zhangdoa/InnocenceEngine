---
id: TASK-255
title: >-
  Intermittent Main.exe init/shutdown hang — residual after TASK-241
  CreateFenceEvents fix
status: To Do
assignee: []
created_date: '2026-06-23 16:13'
labels:
  - rendering
  - bug
  - dx12
  - blocker
  - follow-up
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Successor to TASK-241. The CreateFenceEvents:106 crash and the pool-exhaustion path (TASK-242) are fixed; the engine now runs Audit/SerializeTest/GIScene to graceful exit 0. A SEPARATE, intermittent init/shutdown hang remains: a run killed mid-frame can leave the virtual GPU / swap-chain in a state that hangs the NEXT launch. Mitigated (not fixed) by Invoke-EngineBounded (Scripts/Lib/Test-Engine.psm1): kill stragglers + 4s settle before launch, WaitForExit(timeout)+Kill so the child is never orphaned. Did NOT manifest in 3 graceful runs on 2026-06-23, so it is not reliably reproducible. Root cause TBD (candidate: GPU/swap-chain teardown ordering on abnormal exit). Out of scope for TASK-241 which was titled for the CreateFenceEvents AV specifically.
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
