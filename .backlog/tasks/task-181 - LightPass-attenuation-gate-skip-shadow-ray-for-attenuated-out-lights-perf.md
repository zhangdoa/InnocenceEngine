---
id: TASK-181
title: 'LightPass attenuation gate — skip shadow ray for attenuated-out lights (perf)'
status: To Do
assignee: []
created_date: '2026-04-28 14:50'
labels:
  - rendering
  - performance
dependencies:
  - TASK-176
priority: medium
references:
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-176 review ADVISORY (graphics-api-expert peer, 2026-04-28). Likely root cause of LightPass 2.42 ms vs predicted 1-2 ms gap.**

`lightPassDirectLighting.hlsl:114` computes `l_AttenuationFactor` for each tile-culled point light. If zero (light too far / behind attenuation cliff), the light contributes nothing — yet the inline RayQuery shadow ray still fires at `:135`.

The shadow ray for an attenuated-out light is wasted work: even if shadowed (visibility=0), the contribution is `0 * 0 = 0`; if visible (visibility=1), the contribution is `0 * 1 = 0`. Same answer, ray cost wasted.

### Required fix

Add `if (l_AttenuationFactor > 0.0)` (or a small epsilon threshold) gate around the shadow-ray dispatch at `:135`. Branch is divergent within a tile but cheap; the ray skip pays for itself.

### Expected perf delta

Sponza tile-culled list typically has 1-2 lights/pixel post-cull, but a fraction are attenuated to zero (light list is sphere-bounded; pixels at the sphere edge get the light in their tile but with 0 contribution). Closing this gap should pull LightPass from 2.42 ms toward the predicted 1-2 ms range.

### Owner

`rendering-researcher` — single-shader edit.

### Why medium priority

Real perf delta but not catastrophic. The 60-FPS bar dominates the priority; once TASK-177 deletes the 93.5 ms PointShadow cost, LightPass's 0.4 ms gap is in the noise floor. Land before TASK-178 closure verification to maximize the budget headroom.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Attenuation-gate added at `lightPassDirectLighting.hlsl:135` (or equivalent line post-TASK-176)
- [ ] #2 Sponza LightPass measurement re-run; cost in 1-2 ms range (perf-measurement-frame-budget.md N≥30)
- [ ] #3 No visual regression — attenuated-out lights produce identical output (zero contribution either way)
<!-- AC:END -->
