---
id: TASK-112
title: >-
  Replace _Exit() in serialize-test with clean engine shutdown + exit-code
  propagation
status: To Do
assignee: []
created_date: '2026-04-20 10:21'
labels: []
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

`RunSerializeTest` currently terminates via `_Exit(0/1)` (bypasses all C++ destructors and CRT cleanup). This was necessary because `std::exit()` crashes with `STATUS_STACK_BUFFER_OVERRUN` when called from inside the scene-loaded callback with D3D12 GPU threads still running.

## Structural weakness

`_Exit()` is a workaround, not a fix. Consequences:
- D3D12 resources are not cleanly released (no device teardown, no debug layer drain)
- If a crash occurs between `SceneService::Save` and `CompareAndRestore`, the data files are left in modified state with no restore
- There is no way to add cleanup logic after the test exits (e.g., GPU validation results)

## Proper fix

1. Store the test result in `InitConfig::serializeTestResult` (new int field, default 0)
2. Call `g_Engine->Get<IWindowService>()->Terminate()` to stop the main loop cleanly
3. In `WinMain.cpp`, after `m_pEngine->Terminate()`, check `initConfig.serializeTest[0] != 0` and return `initConfig.serializeTestResult`

This mirrors how `totalFrames` + `HasGPUError()` already drives the frame-run exit code.

## Files

- `Source/ExampleProject/LogicClient/World.inl` — `RunSerializeTest()`
- `Source/Engine/Platform/WinMain/WinMain.cpp` — add result check after `Terminate()`
- `Source/Engine/Engine.h` / `InitConfig` — add `int serializeTestResult = 0` field
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
