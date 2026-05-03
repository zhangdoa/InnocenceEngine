---
id: TASK-215
title: >-
  ImGuiWrapper "Load scene" button uses sync Load (engine-invariants.md
  violation)
status: To Do
assignee:
  - low-level-expert
created_date: '2026-05-03 08:24'
labels:
  - bug
  - engine-invariant
dependencies: []
references:
  - 'Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:293'
  - .claude/state/engine-invariants.md
  - Source/Engine/Services/SceneService.h
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Violation

`Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:293` (the ImGui "Load scene" button callback) calls:

```cpp
g_Engine->Get<SceneService>()->Load(scene_filePath, false);
```

This is a UI-thread caller passing `AsyncLoad=false` — exactly the failure shape the engine invariant warns against.

## Invariant (verbatim from `.claude/state/engine-invariants.md:7-11`)

> `SceneService::Load()` calls that originate outside the render thread (HIDService button callbacks, any other main-thread context) must pass `AsyncLoad=true`. `LoadSync()` from a non-render-thread caller races the render thread's `IGraphicsService::Update()` and corrupts the DX12 resource state tracker, manifesting as a `tracker=UAV / D3D12-actual=SRV` barrier mismatch on the Final Blend Pass Result texture (e.g. after pressing R to reload a scene).
>
> `LoadAsync()` sets `m_prepareForLoadingScene=true`; `SceneService::Update()` on the render thread picks it up the next frame and calls `LoadSync()` from the safe context. The auto-test path (loading from `World::Update()` inside the render thread callback) may remain synchronous because it already runs on the render thread.

## Precedent

Commit `f4e0252a` fixed the same shape in `Source/Engine/Common/World.inl` (the active failure point — latent race surfaced by TASK-213 CL 5's wider LoadSync window). The test-expert peer review of that fix audited remaining `Load()` call sites and flagged ImGuiWrapper.cpp:293 as the same shape, but only triggers on user UI-button reload — not on the auto-test path, so latent until exercised.

## Owner

`low-level-expert` per fallback rule in dispatch brief: `editor-tooling-expert` scope is `Source/Editor-Next` (Vue / Electron / TS), not the C++ runtime ImGui wrapper.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 ImGuiWrapper.cpp:293 changed to pass AsyncLoad=true (matching the f4e0252a precedent in World.inl).
- [ ] #2 Audit all other Load() call sites in Source/Engine/ThirdParty/ImGuiWrapper/ — any UI-thread caller passing AsyncLoad=false is corrected; any intentional sync caller (render-thread context) is documented in-place.
- [ ] #3 Optional: extend .claude/state/engine-invariants.md to enumerate all known SceneService::Load() call sites and their thread context, so the next audit is a one-liner grep against the enumerated list.
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
