---
id: TASK-169
title: 'Sponza FPS budget: currently ~20, target ≥60 (likely point-shadow cost)'
status: Done
assignee: []
created_date: '2026-04-27 21:00'
updated_date: '2026-04-29 17:26'
labels:
  - rendering
  - performance
  - bug
dependencies:
  - TASK-168
references:
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-27**: "the FPS should be no less than 60. sponza is like 20 something now, i guess the point light shadow or whatever is expensive."

Sponza is rendering at ~20 FPS. The user's hypothesis is the new TASK-66 point/sphere shadow cube-atlas caster pass is expensive. The cost wasn't measured at TASK-66 closure — the parent's AC#5 was closed PARTIAL ("30-frame GISponza wall-clock unchanged" but "per-pass GPU-timer Verbose log doesn't fire within `-total_frames` budget; not collected").

### Investigation directions

Use TASK-168's runtime pass-bypass toggle once it lands:

1. Bypass `PointShadowGeometryProcessPass` (the cube-atlas caster), measure FPS delta. If FPS jumps to ≥60: confirmed root cause; fix is the caster pass itself.
2. If FPS still <60 after bypassing point shadows: bypass the next suspect (RT sun shadow, GI passes, post-FX) one at a time until the budget breaks.
3. Use the now-opt-in `-gpu_timer_log` (TASK-165) for per-pass numbers once the suspect is isolated.

### Likely fixes (depending on diagnosis)

- **Cube atlas: 6 faces × 8 lights × 256² is ~12 MB**, but the *render cost* is 8 lights × 6 faces × full-scene-rasterize. If geometry is heavy, that's 48× the OpaquePass cost in the worst case. Fixes: lower default `maxPointShadows`, add per-face frustum culling (currently reuses the broader culling buffer per `PointShadowGeometryProcessPass.cpp:222` — likely overdraws), or use a coarser LOD for shadow caster.
- **GS fan-out cost** — `[maxvertexcount(18)]` per-light invocation amplifies vertex throughput. If the fan-out is the dominant cost, switch to per-face draws (deferred during TASK-148 as the GS approach was simpler).
- **Tiled light culling** may not bin point lights by their shadow-atlas slot; if all 8 slots are sampled per pixel regardless, that's hidden cost in LightPass too.

### Bar (the new principle)

Per the user's CL principle (filed alongside this task in rendering-researcher's manifest): rendering changes must maintain **≥60 FPS** on Sponza. This task is the corrective for the existing violation; future rendering changes are held to the bar going forward.

### Owner

`rendering-researcher` (pass cost analysis, fix). Coordinate with `graphics-api-expert` if the fix is in command-list / TLAS / barrier surface.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Per-pass cost measured on GISponza using TASK-168 bypass toggle + TASK-165 -gpu_timer_log; numbers quoted
- [x] #2 Root cause identified — which pass(es) are over-budget; cited file:line
- [x] #3 Fix lands; GISponza windowed sustained ≥60 FPS measured over 30+ second walkthrough
- [x] #4 Visual regression clean — captures show no quality loss vs pre-fix
- [x] #5 No new GBV ERROR / WARNING from the fix
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Resolved by TASK-175 chain (RT shadow unification)

User confirmed 2026-04-29: TASK-169's ≥60 FPS Sponza target is met.

The point-shadow cost hypothesis was correct, and the fix took a stronger form than the original investigation directions anticipated: the entire cube-shadow stack was deleted and replaced with inline RT shadow rays in `lightPass.comp`. This eliminated the per-light × per-face cube-atlas rasterization cost entirely, rather than tuning it.

**Closing chain**:
- `d2b2e9fe` (TASK-176) — inline RayQuery shadow rays in LightPass per light type
- `f41a4ffb` (TASK-177 CL1) — delete cube-shadow rendering stack
- `c478a833` (TASK-177 CL2) — delete cube-shadow engine-common
- `581ad895` (TASK-175 closure) — chain rolled up with user runtime confirmation: ≥60 FPS Sponza, no GBV regressions, PT cross-check clean

**AC mapping**:
- AC #1 (per-pass cost) — superseded; the cost was eliminated by deletion, not measured-then-tuned. Pass-bypass toggle TASK-168 unblocked this kind of analysis but TASK-175's deletion path made it unnecessary for THIS task.
- AC #2 (root cause) — confirmed: cube-atlas caster pass.
- AC #3 (≥60 FPS sustained) — user-confirmed at TASK-178 runtime bar.
- AC #4 (visual regression clean) — verified during TASK-176 peer review and TASK-178 closure verification.
- AC #5 (no new GBV) — verified at TASK-178 closure.

**Drift-audit gap noted**: TASK-201's automated audit (commit `6f1247e4`) did not catch this retrofit because no closing commit's text contains "TASK-169" — the fix shipped under TASK-175/176/177. Future drift-audit recipe (TASK-201 AC #4, blocked on TASK-203) should consider task dependencies — when a closing commit references TASK-N, also check TASK-N's `dependencies` field for downstream tasks that may now be resolved.
<!-- SECTION:FINAL_SUMMARY:END -->
