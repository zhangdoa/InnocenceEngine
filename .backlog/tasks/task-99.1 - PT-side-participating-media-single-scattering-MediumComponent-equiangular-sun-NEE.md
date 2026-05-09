---
id: TASK-99.1
title: 'PT-side participating media — single-scattering, MediumComponent + equiangular sun NEE'
status: To Do
assignee: []
created_date: '2026-05-09'
labels:
  - rendering
  - path-tracer
  - volumetric
  - participating-media
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
PT-side replacement for TASK-99 (closed obsolete-under-PT-primary 2026-05-09). Adds bounded homogeneous participating media to the GPU path tracer with single-scattering integration. Primary user-visible target: sun god-rays through dust in Sponza interiors.

**Approach** (locked via brainstorming, 2026-05-09):
- Bounded volumes via new `MediumComponent` (entity-AABB-bound, scalar `sigma_t`, RGB `albedo`, scalar HG `g`).
- Equiangular sampling on sun NEE only (PBRT-v4 §11.4); other lights as in-scatter sources deferred.
- Henyey-Greenstein phase function (PBRT-v4 §11.2).
- Output to integrator's emissive radiance slot (bypasses NRD ReBLUR demod, no interaction with TASK-77.4).
- Compile-time `Inno::PT::Media::ENABLED` + runtime `g_DevToggle_PTMedia`.

**Spec:** `docs/superpowers/specs/2026-05-09-pt-media-single-scattering-design.md` — full architecture, data shapes, algorithm, CL split, risk register.

**Status:** parked. Spec written, no implementation queued. Pick up when motivated.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 (visual, blocking) Sun god-rays visible in GISponza with `MediumComponent` placed on the central nave interior; user-direction layer-4 sign-off on a moving-camera capture.
- [ ] #2 (visual) UnitTest + GITestBox each show clean equiangular in-scatter on a defined volume (no NaN, no negative radiance, no banding past acceptable Monte Carlo noise).
- [ ] #3 Compile-time toggle `Inno::PT::Media::ENABLED = false` elides all medium code paths; binary identical to pre-CL-1 in that build mode.
- [ ] #4 Runtime DevToggle `g_DevToggle_PTMedia = false` skips slab test + equiangular dispatch; emissive slot receives zero from media.
- [ ] #5 No interaction with TASK-77.4 NRD denoiser — emissive channel routes around ReBLUR by construction; visual diff with NRD on/off shows expected behavior.
- [ ] #6 No `-gpu_validation` errors on smoke run across all 3 scenes.
- [ ] #7 Peer review on each CL by a fresh impl-stage agent.
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Spec (`docs/superpowers/specs/2026-05-09-pt-media-single-scattering-design.md`) is the load-bearing document. CL split:

| CL | Scope | LoC |
|---|---|---|
| CL-1 | `MediumComponent` + `MediumDataService` + GPU binding + integrator stub returning zero in-scatter. Verify or split the emissive radiance channel as prep. | ~250 |
| CL-2 | `PTMediaIntegrator.hlsli`: slab test, equiangular sampling, Beer-Lambert, HG phase, sun NEE attenuation. | ~200 |
| CL-3 | Test fixtures (UnitTest, GITestBox, GISponza) + RenderDoc captures + density tuning. | ~50 + scene-data |

CL-1 first task: read `Source/Shaders/HLSL/common/PTRaygenIntegrator.hlsl` to confirm the integrator's radiance output channels. If emissive isn't a separate UAV slot, splitting it out is part of CL-1.

**Cross-refs**:
- TASK-99 — predecessor (closed obsolete).
- TASK-77 — PT-primary direction parent.
- TASK-77.4 — concurrent NRD work; emissive channel routing avoids interaction.
- TASK-104 / TASK-105 — sky / cloud / sun R&D, separate shape.
<!-- SECTION:NOTES:END -->
