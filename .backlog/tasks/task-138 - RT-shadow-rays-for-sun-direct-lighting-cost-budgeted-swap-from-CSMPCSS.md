---
id: TASK-138
title: RT shadow rays for sun direct lighting (cost-budgeted swap from CSM+PCSS)
status: To Do
assignee: []
created_date: '2026-04-26 16:49'
updated_date: '2026-04-26 17:21'
labels:
  - rendering
  - shadows
  - lighting
  - raytracing
dependencies:
  - TASK-140
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User direction (2026-04-26): try hardware-RT shadow rays for the sun, cost-budgeted against the existing CSM+PCSS path (TASK-106 closed 2026-04-20). Current CSM with PCSS measures ~3ms in PIX per user; RT shadows are acceptable if cost ≤ that.

### Why now

User's stated priority queue: physically correct sun light → point/sphere shadows (TASK-66) → GI improvements. RT shadows are paper-faithful for an angular sun (~0.5° angular diameter) — proper contact hardening, no cascade seams, no acne/peter-panning trade-offs to tune. PCSS is a rasterizer approximation; hardware RT does the math directly.

Also forward-looking: TASK-66 (point/sphere shadow maps) will land next. If RT shadows for sun work and cost is acceptable, the same RT-shadow infrastructure can extend to point/sphere lights instead of building a separate shadow-map pyramid for each. Net code surface smaller, single light-shadow path instead of three.

### Scope

1. **Add a DXR shadow-ray dispatch** (or extend an existing RT pipeline) that, for each shaded pixel, traces a shadow ray toward the sun direction. Returns visibility (0 = shadowed, 1 = visible). Use the existing `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` pattern from TASK-6.10's sky NEE — it's the fastest shadow-ray flag set for opaque-geometry-only visibility.
2. **Soft shadows via cone jitter**: jitter the ray direction within the sun's angular cone (~0.5° half-angle for our sun). Single jittered sample with TAA accumulation may suffice; otherwise 2-4 samples.
3. **Cost measurement**: capture PIX (or RenderDoc) timing of the new RT shadow pass vs the existing CSM+PCSS pass. Quote both numbers explicitly.
4. **Decision based on cost**:
   - **If RT cost ≤ CSM+PCSS (~3ms baseline)**: replace CSM+PCSS path with RT shadows; remove `SunShadowGeometryProcessPass` (cascade depth render) + `SunShadowResolver`'s PCSS evaluation; `lightPassDirectLighting.hlsl::EvaluateSunLighting` consumes the new visibility texture instead.
   - **If RT cost > CSM+PCSS by >50%**: keep both paths, gate via build/runtime flag. Document the cost.
   - **If RT cost is between (1.0× and 1.5× CSM)**: ship the swap if quality justifies it (no cascade seams, paper-faithful soft shadows). Otherwise keep CSM as default.

### Constraints

- **DO NOT touch point/sphere shadow paths.** TASK-66 owns that scope. If RT shadow infrastructure is generalizable to point/sphere lights, note it in the closure summary as a follow-up — but don't attempt to fold those into this CL.
- **MaxTraceRecursionDepth was bumped to 2 in TASK-6.10** (`DX12RenderPassResourceService.cpp:498`). RT shadows from a primary visibility pass are depth-1 from the shaded pixel — no further bump needed. If you bind the RT-shadow dispatch into a recursive context (e.g. as part of the radiance-cache pipeline), the existing depth-2 cap covers it.
- **No magic numbers** per `feedback_no_magic_numbers.md`. The sun's angular half-angle should be a named constant (`SUN_ANGULAR_HALFANGLE_RAD = 0.00872665` ≈ 0.5°) sourced from a single header.
- **Loud on data violations** per `feedback_no_data_integrity_assumptions.md`. If the sun direction is invalid (zero, NaN), the shadow trace must fail loudly, not silently return "always visible".

### Validation

- Build green (engine + shader + DXR PSO).
- Engine smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- GBV pass with `-gpu_validation -total_frames 10` clean.
- **Cost measurement**: PIX capture (or equivalent profiler) showing new RT shadow pass time + existing CSM+PCSS pass time. Quote both.
- **Visual check**: GISponza windowed capture before vs after — soft shadows from sun, no cascade seams, contact hardening present. Compare against `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` (PT reference has correct angular-sun shadows for cross-check).
- **Regression check**: GITestBox visual check — no acne/peter-panning artifacts on the angled cube faces.
- If cost decision is "swap": validate that removing CSM passes doesn't break any other consumer (search for `SunShadowGeometryProcessPass` / `SunShadowResolver` callers). If "keep both": validate the gate switches both paths cleanly.

### What was NOT verified — to call out in closure

- Windowed framerate sustained over a 5-minute walkthrough (not a CI-style smoke).
- Hardware-tier sensitivity (RT cost varies wildly between RDNA2/3, Ada, etc.).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 DXR shadow-ray dispatch added for sun direct lighting; consumed by EvaluateSunLighting
- [ ] #2 Sun's angular half-angle named constant (SUN_ANGULAR_HALFANGLE_RAD); cone-jittered for soft shadows
- [ ] #3 Build green; smoke exit 0; GBV pass clean
- [ ] #4 PIX/profiler measurement of new RT shadow pass vs current CSM+PCSS, both quoted in summary
- [ ] #5 Cost-based decision documented: RT replaces CSM, both paths kept with gate, or RT shipped despite cost (with justification)
- [ ] #6 Visual capture vs PT reference shows angular-sun soft shadows, no cascade seams, contact hardening
- [ ] #7 GITestBox no acne/peter-panning regression
- [ ] #8 If 'swap' chosen: CSM passes (SunShadowGeometryProcessPass, etc.) removed and no orphaned consumers
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
