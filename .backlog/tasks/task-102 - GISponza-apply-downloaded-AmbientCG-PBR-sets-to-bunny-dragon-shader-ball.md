---
id: TASK-102
title: 'GISponza: apply downloaded AmbientCG PBR sets to bunny / dragon / shader ball'
status: To Do
assignee: []
created_date: '2026-04-19 18:36'
labels:
  - rendering
  - assets
  - sponza
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The ExampleRenderingClient already bootstraps four AmbientCG PBR sets at startup (`Concrete007`, `Ground037`, `Metal032`, `Tiles074` — see `ExampleRenderingClient::Setup`). These are loaded into the asset cache but none of GISponza's hero props actually reference them. Bunny, dragon, and the shader-ball should each be tagged to a distinct PBR set so the scene exercises real authored PBR materials, not the default diffuse fallback.

## Scope

- Pick a PBR set per prop (suggestion: dragon → Metal032, bunny → Tiles074, shader-ball → Concrete007 — but judgement call at implementation time).
- Wire the texture slots (Color / NormalGL / Metalness / Roughness) into each prop's MaterialComponent JSON under `Data/ExampleProject/Components/`.
- Confirm the `ImportTexture` bootstrap produces the TextureComponent JSONs the scene will reference (it's idempotent, so rerunning `Main.exe` once should populate them).
- Before / after RenderDoc capture on GISponza frame 8 to confirm the textures actually land.

## Why

The downloaded PBR sets are dead weight until a scene references them. Tagging the three hero props gives the rasterized and path-traced pipelines a non-trivial material to chew on and makes BRDF regressions immediately visible.

## Out of scope

- Adding more PBR sets beyond the existing four.
- Cloth curtains (covered by TASK-98).
- Sponza architecture materials (those already have their own authored materials).
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
