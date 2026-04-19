---
id: TASK-104
title: >-
  R&D: volumetric cloud rendering (cumulus / cirrus / altocumulus coverage
  model)
status: To Do
assignee: []
created_date: '2026-04-19 18:39'
labels:
  - rendering
  - sky
  - volumetric
  - research
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Sky today is cloud-free. Add a volumetric cloud rendering pass with enough expressiveness to cover the common types — cumulus, cirrus, altocumulus, overcast stratus — controlled by a small set of coverage/density/altitude parameters.

## Approach candidates

- **Ray-marched 3D noise field** (Schneider/Horizon Zero Dawn style). Classic choice: low-freq Worley base × high-freq erosion, beer-powder attenuation, two-lobe phase. Good quality, heavy GPU cost.
- **Procedural 2D impostor** for cheap reference / mobile fallback. Less physical; useful as a tier.
- **Hybrid layered volumetric** — different altitude bands use different noise parameters. Preferable if TASK-99 volumetric pass is already paying the ray-march cost.

## Scope

- Authored or procedural shape/erosion noise textures (offline bake via a compute shader or tooling).
- Parameter set: coverage, type (cumulus/stratus/cirrus mix), base altitude, thickness, wind vector, density scale.
- Integration with `SkyPass` (clouds composite over sun/stars, below aurora/meteors), `VolumetricPass` (TASK-99 — shared ray-march kernel where possible), and the path tracer (importance-sampled along view ray).
- Temporal reprojection to reduce per-frame cost; spatially amortize the expensive march.

## Why

Sunny-day + clear sky is visually flat and punts on the hardest interaction in atmospheric rendering: clouds scatter sun/moon light, cast godrays, and dominate brightness variance. Ties TASK-99 (volumetric) and TASK-103 (night sky) together.

## Deliverables

- Dev-toggle-switchable cloud pass.
- RenderDoc captures across the full parameter sweep (clear → scattered → overcast).
- Cost budget measured; quality-tier options documented if the full ray-march is disproportionate.
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
