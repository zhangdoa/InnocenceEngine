---
id: TASK-94
title: 'Path tracer: sky sampled only when visible, not as a universal miss fallback'
status: To Do
assignee: []
created_date: '2026-04-19 17:10'
labels:
  - path-tracer
  - lighting
  - sky
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What happened

Previously, the path tracer sampled the sky environment on any ray miss. That made Sponza look blown-out / too shiny because every indirect ray that eventually missed geometry — including rays that would physically be occluded by the atrium roof — returned full sky radiance.

A quick fix turned the miss→sky fallback off, which solved the Sponza shine but also silenced genuine sky contribution in scenes where the sky is actually visible. Both states are wrong.

## What we want

Sky samples the sky only when the sky is actually visible from the ray origin in the ray's direction — i.e. the ray exits the scene without being occluded by any geometry. The current heuristic (any miss = sky) is too permissive; the current override (no miss = sky) is too restrictive.

## Directions to consider (pick during implementation)

- **Visibility check at shading**: on a diffuse/specular bounce, before adding a direct-to-sky contribution, cast a visibility ray to "the sky" (or, equivalently, treat sky as a directional/environment light and MIS it with a shadow ray). If occluded, drop the contribution; if unoccluded, use it.
- **Tracked ray depth / hit history**: only apply sky on the PRIMARY miss or after a reflection where the reflected ray direction clears the bounds, not on arbitrary indirect misses.
- **Energy budget**: cap the sky contribution on deeper bounces (some engines do per-bounce attenuation), keeping the physical answer qualitatively right without exploding brightness.

Pick the one that matches our BRDF/sampler shape; document the choice inline.

## Acceptance Criteria

- [ ] #1 Sponza (roofed atrium) no longer appears blown-out with sky contribution — render matches reference expectation at convergence
- [ ] #2 A scene with clearly visible sky (skybox / open sky dome) shows correct sky contribution on primary rays and appropriate indirect/reflection contribution
- [ ] #3 RenderDoc or screenshot diff against a known-good (pre-regression) frame confirms both cases
- [ ] #4 The chosen mechanism (visibility-shadowed sky, depth-gated, energy-capped) is documented in the path-tracer shader with a one-line rationale
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
