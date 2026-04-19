---
id: TASK-107
title: 'Restore the basic animation system (skeletal rig, skin, blend tree)'
status: To Do
assignee: []
created_date: '2026-04-19 19:18'
labels:
  - rendering
  - animation
  - regression
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The engine previously had a basic animation pipeline — skeletal rig load, skinned-mesh draw call, and a simple blend/playback system — but it's currently dormant. `AnimationPass`, `AnimationSimulationService`, and `AnimationResourceService` exist but are commented out of the rendering client's active Setup graph. Bring the pipeline back so a rigged character (gltf / fbx import with bones) can load, skin on the GPU, and play an authored clip at runtime.

## Scope

- Reactivate `AnimationPass` registration in `ExampleRenderingClient::Setup` (currently a commented-out line).
- Verify / re-land the rig load path in `AnimationResourceService` — bone hierarchy, inverse-bind matrices, clip keyframe storage.
- Reactivate per-frame simulation in `AnimationSimulationService` — sample clip, compose bone world matrices, upload the skinning palette to a constant or structured buffer.
- Wire the GPU skinning in `AnimationPass` — vertex shader reads the bone palette and weights; output world-space deformed vertices for the opaque draw.
- A reference asset (simple rigged character or cube with a sine-wave skeleton) that exercises the pipeline end-to-end.
- Integration test / screenshot showing the asset animating across frames.

## Out of scope (separate tasks)

- IK, physics-based animation, state machines beyond a single clip.
- Morph targets / blend shapes (add later; track as a follow-up once rig skinning works).

## Why

Animation is a baseline expectation for any scene-capable engine. The current "static geometry only" state is an orthogonal regression during the ECS overhaul — worth paying back before shipping any narrative / gameplay demo. Also unlocks future work: IK, ragdoll, state machines, in-editor animation preview.

## Pointers

- Last-known-good animation code lives in git history; grep for `AnimationPass::Setup` in older commits on `ecs-overhaul`.
- `RenderingClient::Setup` in `ExampleRenderingClient.cpp` has the commented-out registration line at approximately `// AnimationPass::Get().Setup();`.
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
