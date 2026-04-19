---
id: TASK-106
title: >-
  Sun shadow quality: replace current amateur filter with production-grade soft
  shadows
status: To Do
assignee: []
created_date: '2026-04-19 19:10'
labels:
  - rendering
  - shadows
  - lighting
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The rasterized sun shadow pass currently produces harsh, pixelated, aliased shadow edges with obvious cascade seams, under/over-darkening near grazing angles, and no perceptible penumbra. The result looks amateur next to the rest of the pipeline — shadow quality is one of the first things a viewer reads as "production" vs "hobby."

## Options (pick during implementation)

### Filter quality

- **PCF (poisson disk / Vogel disk)** — cheap, common; tune kernel size and tap count per cascade.
- **PCSS** (Percentage-Closer Soft Shadows) — distance-to-blocker driven penumbra; more realistic, moderate cost.
- **Moment / variance shadow maps** — good contact hardening, harder to tune, can leak at complex occluders.
- **Ray-traced contact shadows** — piggyback on the ray-tracing infrastructure the path tracer already uses; expensive but silences cascade seams.

### Cascade quality

- Verify cascade placement (log / uniform / hybrid) is sized for the current camera range.
- Tighten the splits, reduce wasted texel area on the far cascades.
- Add cross-cascade blending to hide seams.

### Filter artifacts to fix

- Peter-panning (shadow offset from base of object).
- Acne (surface shadowing itself) — tune bias or use slope-scaled bias with a floor.
- Light leaking through thin geometry — receiver-based offset.

## Current state (to capture before diving in)

- RenderDoc capture a reference frame so the "amateur" baseline is concrete.
- Note which cascade count / filter is in place (`SunShadowGeometryProcessPass`, its follow-up blur passes — SunShadowBlurOdd/Even are currently commented out in `ExampleRenderingClient::Setup`).

## Why

Lighting drives the first 80% of "does this look real." Soft, grounded shadows matter as much as any BRDF improvement. Ties to TASK-98 (cloth), TASK-99 (volumetric, god-rays want real shadows), TASK-105 (sun disc).

## Deliverables

- Before/after RenderDoc + screenshot on GISponza.
- Cost budget at 1080p documented.
- Quality tier options if the full filter is disproportionate.
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
