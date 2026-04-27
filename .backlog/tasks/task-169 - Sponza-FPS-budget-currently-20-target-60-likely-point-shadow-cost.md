---
id: TASK-169
title: 'Sponza FPS budget: currently ~20, target ≥60 (likely point-shadow cost)'
status: To Do
assignee: []
created_date: '2026-04-27 21:00'
labels:
  - rendering
  - performance
  - bug
dependencies:
  - TASK-168
priority: high
references:
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
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
- [ ] #1 Per-pass cost measured on GISponza using TASK-168 bypass toggle + TASK-165 -gpu_timer_log; numbers quoted
- [ ] #2 Root cause identified — which pass(es) are over-budget; cited file:line
- [ ] #3 Fix lands; GISponza windowed sustained ≥60 FPS measured over 30+ second walkthrough
- [ ] #4 Visual regression clean — captures show no quality loss vs pre-fix
- [ ] #5 No new GBV ERROR / WARNING from the fix
<!-- AC:END -->
