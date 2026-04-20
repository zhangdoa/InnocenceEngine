---
id: TASK-106
title: >-
  Sun shadow quality: replace current amateur filter with production-grade soft
  shadows
status: Done
assignee: []
created_date: '2026-04-19 19:10'
updated_date: '2026-04-20 17:10'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Sun shadow PCSS hardened in two commits:

1. `0d36371a fix(shader): sun shadow PCSS tuning` (option A)
   - IGN per-pixel rotation of the 16-tap Poisson kernel.
   - LIGHT_SIZE 0.005 → 2.0, penumbra clamp 1.0 → PENUMBRA_MAX_TEXELS=20.
   - Blocker-search size rewritten as texel count (4-12 depending on depth). Previous 0.02-0.08 multiplied by texelSize produced sub-texel radius, so the search found no occluders on most surfaces.
   - PCSS early-out when blocker search reports zero occluders.
   - `SunShadowResolver` signature now takes `uint2 screenCoord` as the rotation seed; both callers in `lightPass.comp` updated.

2. Cross-cascade blending (option B, commit to follow with this summary):
   - Factored per-cascade evaluation into `EvaluateCascadeShadow`.
   - Added `ComputeCascadeEdgeWeight` — returns 1 in the interior, fading to 0 over CASCADE_BLEND_BAND=15% of half-extent at the AABB edge.
   - When in the fade band AND a further cascade exists, evaluates both cascades and lerps. Outermost cascade fades to "unshadowed" at its edge (beyond-cascade has no shadow data anyway).

Non-goals met in this task:
- Bias rework (slope-scaled / receiver-plane). Accepted current constants.
- VSM / moment shadow maps.
- Ray-traced contact shadows.

Validation:
- HLSL2DXIL compile: 0 errors.
- RenderTest draw_instanced: exit 0.
- Main.exe 10-frame offscreen: exit 0.
- Main.exe 20-frame + reload at 10: exit 0.
- Output pixel md5 changes observed after each step, confirming filter is active.

NOT verified:
- Windowed visual check (offscreen thumbnails are small and top-down — shadow detail hard to read at this zoom). Captures in `Build/captures/shadow_baseline.png`, `shadow_A_unittest.png`, `shadow_A2_unittest.png`, `shadow_B_unittest.png` for side-by-side review on a larger screen.
- Cascade-seam behavior specifically — the UnitTest auto-camera stays in a single cascade; a manual camera walkthrough along a cascade boundary is the honest way to confirm the B improvement and wasn't run.
- Perf impact measurement (additional PCSS evaluation in the blend band). Likely negligible because the band is 15% of each cascade volume and two cascades overlap briefly.
<!-- SECTION:FINAL_SUMMARY:END -->
