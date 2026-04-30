---
id: TASK-181
title: 'LightPass attenuation gate — skip shadow ray for attenuated-out lights (perf)'
status: Done
assignee:
  - rendering-researcher
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
- [x] #1 Attenuation-gate added at `lightPassDirectLighting.hlsl:135` (or equivalent line post-TASK-176)
- [ ] #2 Sponza LightPass measurement re-run; cost in 1-2 ms range (perf-measurement-frame-budget.md N≥30) — **REFUTED by data, see Final Summary**
- [x] #3 No visual regression — attenuated-out lights produce identical output (zero contribution either way)
<!-- AC:END -->

## Final Summary

Gate landed; TASK-176 ADVISORY hypothesis refuted by data.

**What landed (commit a414bae7's successor — see git log).**

`l_AttenuationFactor > 0.0` short-circuit prepended to the inline-RT shadow guard in `EvaluateTiledPointLighting`. Threshold is the analytic boundary, not an epsilon: `SmoothDistanceAttenuation = saturate(1 - factor*factor)^2` returns *exactly* 0 once `distance >= attenuation_radius` (verified in `BSDF.hlsl:14-19`; DXC IEEE-754 fp model gives `0*0 == 0` exact). Composes with the existing `g_Frame.pointShadowBypass` (TASK-195) and `l_PointLight.shadow.x` (TASK-149) gates in the same conditional, leftmost so it short-circuits before the cbuffer read.

**AC #1 (PASS).** Gate landed.

**AC #3 (PASS).** Visual regression analytically impossible (the multiply `attenuation * lightDirect == 0` either way); empirically confirmed: `Scripts/TestGIScene.ps1` PASS, MAE 0.389 < 0.45 threshold.

**AC #2 (REFUTED, NOT met).** Perf A/B: GISponza autotest, RelWithDebInfo, 1000 frames each, GPU-timestamps via `-gpu_timer_log`, N=33 LightPass samples.
- A baseline: mean 1.8024 ms, stddev 0.0728 ms.
- B with-gate: mean 1.8054 ms, stddev 0.0718 ms.
- Delta: +0.003 ms (+0.2%) — within noise (< 0.1× stddev).

The TASK-176 ADVISORY framed this CL as the likely root cause of the 2.42 ms vs 1-2 ms LightPass headroom gap. The data refutes that hypothesis on the GISponza camera path. Either (a) the autotest camera doesn't visit attenuated-out edge-tile cases often enough to expose the worst-case win, or (b) the headroom hypothesis was structurally wrong and the ~0.4 ms gap lives elsewhere (BSDF cost, light-list iteration overhead, divergence, IES sampling, …).

**Why the gate stays.** Type-correctness and dead-work avoidance are valid even when the perf delta is sub-noise on this scene. The cost of the guard is one fp compare per per-light iteration — cheaper than the trace it skips. Worst-case scenes (denser PointLight populations, narrow attenuation radii) would show a measurable delta; the gate is correct on principle.

**Follow-up.** A new task investigates the unexplained ~0.4 ms LightPass headroom: either build a denser-light scene that exercises edge-tile attenuated-out cases (validating the gate's worst-case win), or attribute the gap elsewhere before declaring LightPass closed. Filed as a separate hygiene commit immediately after this CL lands.

**Tooling observation (not filed).** `-gpu_timer_log` Verbose dump fires at most once every 30 frames (`GPU_TIMER_LOG_PERIOD_FRAMES`), so capturing N=33 samples required `-total_frames 1000`. An opt-in per-frame dump for perf-measurement runs would make future N≥30 captures much cheaper. Not filing yet — will be filed if perf measurement becomes a regular workflow.
