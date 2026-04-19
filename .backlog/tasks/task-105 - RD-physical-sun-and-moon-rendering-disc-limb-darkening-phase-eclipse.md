---
id: TASK-105
title: 'R&D: physical sun and moon rendering (disc, limb darkening, phase, eclipse)'
status: To Do
assignee: []
created_date: '2026-04-19 18:40'
labels:
  - rendering
  - sky
  - research
  - astronomy
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

Render the sun and moon as actual celestial bodies — visible disc, correct angular diameter (~0.5°), limb darkening for the sun, phase and libration for the moon — rather than the current "sun is a directional light with no disc, moon doesn't exist."

## Scope

### Sun

- Visible disc with limb darkening (Eddington 3-coefficient or Neckel-Labs 6-coeff model).
- Chromatic aberration at the disc edge (sun is slightly redder at the rim).
- Photosphere color temperature driven by sun elevation (connects to the atmospheric-scattering pass's Rayleigh/Mie extinction).
- Correct angular diameter — the disc must be ~0.5° in the viewport regardless of FOV.

### Moon

- Textured sphere (NASA CGI Moon Kit or equivalent) with lit hemisphere computed from sun direction (same geocentric math).
- Phase — automatically falls out of sun/moon positions; no ad-hoc mask.
- Libration — slow rotation on up-down and left-right axes over ~18 years; cheap to compute, adds a surprising amount of realism.
- Earthshine — terminator side gets faint blue-grey illumination from Earth-reflected sunlight.

### Shared

- Geocentric ephemeris: convert date/time + lat/lon into sun/moon azimuth/elevation vectors. Accuracy target: good enough for visual (not astronomy-grade), so Meeus "Astronomical Algorithms" chapter 22 / 47 with 1° precision is fine.
- Eclipse handling — when the moon occludes the sun, the disc goes through partial → total, and the corona/chromosphere show briefly at totality. Optional; flag if chasing it is expensive.

## Why

Sun and moon are the two brightest objects in the sky; representing them as an invisible directional light fundamentally limits realism. Ties directly to TASK-99 (volumetric god-rays from a real sun disc), TASK-103 (moonlight as primary illuminant at night), and TASK-104 (clouds backlit by the moon).

## Deliverables

- Sun and moon discs visible in the sky with the correct angular size and phase.
- RenderDoc capture sequence across a full day → dusk → night → dawn cycle.
- Path tracer samples both as area lights (not just the directional approximation).
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
