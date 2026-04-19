---
id: TASK-103
title: >-
  R&D: realistic night sky simulation (stars, Milky Way, zodiacal light,
  airglow)
status: To Do
assignee: []
created_date: '2026-04-19 18:39'
labels:
  - rendering
  - sky
  - research
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Today the sky is a fixed clear-sunny-day model. Add a night sky path so scenes can render under moonlight/starlight with physically-plausible star distribution, Milky Way density map, and (aspirationally) zodiacal light and airglow.

## Scope

- **Star catalog** — Hipparcos / Yale bright-star catalog (a few thousand stars) as a small vertex buffer; per-star magnitude, spectral type, RA/Dec. Distant-sphere billboard or tiny-quad impostor pipeline.
- **Milky Way density** — equirectangular HDR panorama (e.g. ESO Milky Way), galactic plane aligned via rotation matrix from Galactic to local coords.
- **Airglow / zodiacal light** — additive low-freq terms. Cheap; defer if schedule-bound.
- **Integration with existing sky pass** — `SkyPass` switches on sun elevation: sun above horizon → current sunny-day; below → night model (with dusk blend). Moon contribution handled by TASK-104 (sun/moon).
- **Tonemapping coherence** — night sky has ~10^6× lower luminance than day; auto-exposure / luminance histogram path must adapt without blowing out to black.

## Why

The engine positions itself as a solid rendering testbed; no-night is a conspicuous gap. Ties into TASK-99 (volumetric — moonlight god-rays), TASK-104 (moon), and the path tracer's MIS sky-sampling story.

## Deliverables

- Night-sky shader pass added; switchable via a dev toggle.
- RenderDoc capture at midnight over Sponza, clear weather.
- Auto-exposure tracks across a full dusk → night → dawn sweep without clipping.
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
