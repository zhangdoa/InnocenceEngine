---
id: TASK-170
title: 'Sun shadow artifacts still visible on UnitTest spheres (post-TASK-164)'
status: To Do
assignee: []
created_date: '2026-04-27 21:00'
labels:
  - rendering
  - shadows
  - bug
dependencies: []
priority: high
references:
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/Shaders/HLSL/common/sunSampling.hlsl
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-27**: "the sun shadow map artifacts still exists! i see them mostly on the spheres in the unit test scene."

TASK-164 fixed two RT sun shadow root causes (constant 65× too small; shading-vs-plane-normal offset divergence) and verified visually on GISponza. The agent skipped GITestBox/UnitTest verification ("dispatch brief asked specifically for GITestBox; Producer to decide priority" — see TASK-164 review #1). That gap shipped: artifacts on UnitTest spheres survived.

### Why spheres specifically

Spheres are the worst-case for normal-offset-based ray-origin biasing:

- Every ray-origin offset along the surface normal moves *off the sphere's surface*; depending on direction, this can over-correct (shadow detaches — peter-panning) or under-correct (self-intersection acne).
- At grazing angles relative to the sun, the offset's projection onto the sun-direction approaches zero, so even a "correct" offset becomes ineffective.
- A 5mm normal offset on a 0.5m-radius sphere is 1% of the radius — visible as a peter-panning gap if the camera is close.

### Investigation directions

1. **Bisect the artifact type**: with TASK-168's pass bypass toggle (when it lands), confirm the artifact disappears when SunShadowRTPass is bypassed. If yes: it's RT shadow. If artifacts survive bypass: another path entirely.
2. **Slope-scaled offset** (review #1's advisory #2 from TASK-164): offset by a magnitude that scales with `1/dot(N, L)` (or similar) to keep the projected escape constant across grazing angles. PT and Embree-style ray-tracers all do this.
3. **Switch to TMin offset along ray direction** instead of normal-offset on origin: ray starts at the surface but with `TMin = epsilon`. Avoids the sphere's curvature problem entirely. This is what `Ray Tracing Gems II` ch.6 recommends for shadow rays.
4. **Cite-prior-art**: GPUPathTracerClosestHit / RadianceCacheClosestHit may have already addressed this for spheres in another path; check before designing fresh.

### Out of scope

- Performance work (TASK-169).
- TAA ghost-streaks (deferred from TASK-164 review).
- Soft-shadow sample-count tuning.

### Owner

`rendering-researcher`. Apply peer-review-required.md (TASK-166) — this is the second iteration on the same surface, reviewer should specifically check that the fix verifies on UnitTest sphere geometry, not just GISponza.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Artifact reproduced + characterized — windowed capture from UnitTest scene at the affected camera, annotated
- [ ] #2 Root cause identified — slope-scale, TMin-offset, or other; cited file:line
- [ ] #3 Fix lands; UnitTest sphere shadow artifacts gone in windowed capture
- [ ] #4 GISponza captures still pass (no regression of TASK-164's fix)
- [ ] #5 GITestBox captures clean (the scene the TASK-164 brief originally asked for and the implementer skipped)
- [ ] #6 No new GBV ERROR / WARNING
<!-- AC:END -->
