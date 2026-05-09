---
id: TASK-99
title: 'Rendering: bring back the volumetric pass and fix accumulated issues'
status: Done
assignee: []
created_date: '2026-04-19 18:11'
updated_date: '2026-05-09'
labels:
  - rendering
  - regression
  - volumetric
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The volumetric pass (fog / god-rays / participating media) was disabled during the ECS overhaul and hasn't been re-enabled. Re-land it, then triage and fix the issues that have surfaced since it was last healthy.

## Scope

- **Re-enable** — flip the pass back into the active render graph (`RenderingClient` registration), wire its inputs (depth, shadow map, sky) and outputs (scattering / extinction).
- **Fix integration issues** — expect fallout from intervening ECS / resource changes: stale descriptor bindings, changed constant-buffer layouts, altered barrier expectations, new async scene-load paths. Work through each as a sub-issue.
- **Re-validate against current lighting model** — the scene lighting path has evolved (path tracer, radiance cache, per-frame light list). Volumetric has to compose correctly with all of them — no double-counting direct light, correct shadow sampling, matches the reference exposure.
- **Performance** — measure cost on GISponza @ 1080p. If the cost is disproportionate, document why and propose a quality tier.

## Why

The engine has a sky and directional sun; dusty Sponza interiors with god-rays is a canonical lighting showcase. Losing volumetrics during the overhaul was always a "temporarily" — time to pay that back.

## Pointers

- Last-known-good volumetric code lives in the git history — start by surfacing what was removed and comparing to current pass lifecycle.
- The current render graph is built by `RenderingClient::Setup`; check how adjacent effects (bloom, tonemap) re-wired after the overhaul.

## Deliverables

- Volumetric renders correctly on GISponza (RenderDoc capture before/after re-enable).
- No validation errors under `-gpu_validation`.
- Test tier 2 (Main.exe integration) stays green.
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Obsolete under PT-primary direction (2026-04-30) + reframed as PT-side medium integration (TASK-99.1)

### Why the original framing fails

Task brief assumed *"the old commented one almost works"* — re-enable + triage fallout. Investigation (2026-05-09 dispatch, code-impl agent) found the opposite:

- All four sub-pass bodies (`froxelization`, `irraidanceInjection`, `rayMarching`, `visualization`) in `VolumetricPass_ExecuteCommands.cpp` are stubs — `BindGPUResource` / `Dispatch` / `CommandListBegin/End` blocks fully commented out.
- ExecuteCommands waits on render-pass components that are never recorded.
- TODO at `VolumetricPass_ExecuteCommands.cpp:60,117` — `// TODO: Implement per-pass dispatch params buffer for VolumetricPass`. Shaders read `dispatchParams[6]/[7]` slots that don't exist.
- Shaders live at `Source/Shaders/HLSL/WIP/volumetric*` — never on the active shader path since the rename at `a511d710`.
- No "last known good" exists in `git log --all`. Commit `99227710` (June 2025) deleted the previously-commented `Initialize/ExecuteCommands/Terminate` registration calls; they were already commented before deletion.

### Why direction-obsolescence on top of that

Task filed 2026-04-19. Project pivoted to PT-primary 2026-04-30 (`.claude/state/project-direction.md`). The existing volumetric design is a **rasterizer-side** subsystem — froxel + ray-march sourcing irradiance from the raster light list, composing into the lit raster buffer. Under PT-primary, participating-media scattering structurally belongs in the path tracer's medium integration, not a parallel raster subsystem maintained in lockstep. Same logic that closed TASK-153.

### Resolution

- **TASK-99 → Done (obsolete-under-PT-primary).** AC #1 / #2 / #3 unticked — they were not done; they target a subsystem that is no longer load-bearing. No integration tests run, no RenderDoc captures produced, no `-gpu_validation` exercise. Honestly: nothing was verified because nothing was implemented.
- **Existing raster volumetric files retained as dead code for now.** `Source/ExampleProject/RenderingClient/VolumetricPass.{h,cpp}`, `VolumetricPass_Setup.cpp`, `VolumetricPass_Internal.h`, `VolumetricPass_ExecuteCommands.cpp`, `Source/Shaders/HLSL/WIP/volumetric*` — never registered, never compiled into the active graph. Removal is a separate cleanup CL (or absorbs into TASK-99.1's scope when PT medium integration lands).
- **Follow-up filed: TASK-99.1** — PT-side medium integration. Spec at `docs/superpowers/specs/2026-05-09-pt-media-single-scattering-design.md`. Replaces this raster-side feature.

### Cross-references

- TASK-77 — PT-primary direction parent.
- TASK-99.1 — successor (PT-side participating media).
- TASK-153, TASK-137 — sibling closures on 2026-04-30 batch (rasterizer-trick subsystems demoted under PT-primary).
- Commit `99227710` — June 2025 deletion of registration calls.
- Commit `a511d710` — shader move into `WIP/`.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
