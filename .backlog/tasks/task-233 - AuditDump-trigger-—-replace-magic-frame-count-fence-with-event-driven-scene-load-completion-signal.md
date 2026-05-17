---
id: TASK-233
title: >-
  AuditDump trigger — replace magic frame-count fence with event-driven
  scene-load completion signal
status: To Do
assignee: []
created_date: '2026-05-17 15:50'
labels:
  - rendering
  - test-infra
  - race-condition
  - followup
dependencies: []
references:
  - >-
    Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp
  - Source/ExampleProject/LogicClient/World.inl
  - Source/Engine/Services/SceneService.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-228 investigation (`216de0e0` was the partial fix). The audit-mode dump fires when `s_AuditFrame == 30` (`Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp:299`), a rendering-execute counter unrelated to the world-update counter `m_AutoFrameCount` at `World.inl:284` that triggers GISponza scene-load at frame 5.

The 5→30 bump bought scene-load latency headroom for current scene complexity, but the underlying gap is the two unrelated counters. If scene-load takes longer in future (more assets, slower bake, slower hardware), the race re-emerges. Magic-number fence is a band-aid; the right shape is event-driven: AuditDump fires on a "scene fully loaded" signal posted by SceneService::Update after the world-update path completes.

Anchor files:
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands.cpp:296-301` (current fence)
- `Source/ExampleProject/LogicClient/World.inl:284` (scene-swap trigger)
- `Source/Engine/Services/SceneService.{h,cpp}` (load completion event)
- `Source/Engine/Services/EditorService.cpp:498` (precedent: WebSocket LOAD_SCENE handler signals on completion — similar pattern)

Pre-existing band-aid; surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 AuditDump trigger replaced with event-driven scene-load completion signal (no magic frame count)
- [ ] #2 Audit autotest reliably captures GISponza on hardware that completes scene-load in <5s and on hardware that takes >30s (worst-case soak)
- [ ] #3 No race window between scene-load trigger and audit capture, regardless of frame-counter values
- [ ] #4 Build green; existing audit captures unchanged byte-shape (modulo nondeterministic ray-bounce noise)
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
