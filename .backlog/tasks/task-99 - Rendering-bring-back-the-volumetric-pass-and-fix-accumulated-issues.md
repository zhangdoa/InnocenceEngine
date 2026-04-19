---
id: TASK-99
title: 'Rendering: bring back the volumetric pass and fix accumulated issues'
status: To Do
assignee: []
created_date: '2026-04-19 18:11'
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

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
