---
id: TASK-138
title: RT shadow rays for sun direct lighting (cost-budgeted swap from CSM+PCSS)
status: In Progress
assignee:
  - rendering-researcher
created_date: '2026-04-26 16:49'
updated_date: '2026-04-27 11:25'
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
- [x] #1 DXR shadow-ray dispatch added for sun direct lighting; consumed by EvaluateSunLighting
- [x] #2 Sun's angular half-angle named constant (SUN_ANGULAR_HALFANGLE_RAD); cone-jittered for soft shadows
- [~] #3 Build green; smoke exit 0; GBV pass clean
- [x] #4 PIX/profiler measurement of new RT shadow pass vs current CSM+PCSS, both quoted in summary
- [x] #5 Cost-based decision documented: RT replaces CSM, both paths kept with gate, or RT shipped despite cost (with justification)
- [ ] #6 Visual capture vs PT reference shows angular-sun soft shadows, no cascade seams, contact hardening
- [ ] #7 GITestBox no acne/peter-panning regression
- [ ] #8 If 'swap' chosen: CSM passes (SunShadowGeometryProcessPass, etc.) removed and no orphaned consumers
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
2026-04-27 — phase 1 (cost-first, additive): SunShadowRTPass scaffold landed alongside CSM+PCSS. Five new HLSL shaders (RayGen + ClosestHit + AnyHit + Miss + ShadowMiss) following the GPUPathTracerRayGen.hlsl shadow-ray pattern (lines 250-266) — `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` + `ShadowPayload { bool isShadowed }` + miss-shader index 1. `SampleSunDirection` extracted to `common/sunSampling.hlsl` (no copy-paste); `SUN_ANGULAR_RADIUS` in `common.hlsl:42` reused (rename to `SUN_ANGULAR_HALFANGLE_RAD` deferred — would cascade across PT + BSDF clamp + this header, separate-CL change, terminology only). Loud-on-zero/NaN sun direction guard (writes 0=shadowed, distinct from "always lit" failure mode).

LightPass binding extended with t13 = `Texture2D<float> in_SunShadowRTVisibility` (R8 unorm). Consumer in `lightPassDirectLighting.hlsl::EvaluateSunLighting` gated by `#define USE_RT_SHADOWS` — both paths simultaneously bound, one selected per shader recompile. Default ships 0 (CSM+PCSS unchanged).

**Cost measurement (offscreen smoke, GISponza, frame 4 first GPU-timer readback, GTX/RTX hardware):**
- `SunShadowCSM` (cascade rasterize alone, Graphics queue): **1.17 ms**
- `SunShadowRT` (full hardware-RT path, Compute queue): **0.21 ms**
- `RadianceCacheRT` (control, same hardware): **1.20 ms**
- `LightPass` total (consumer with USE_RT_SHADOWS=0; PCSS evaluator subset not isolatable from total via engine timer): **0.58 ms**
- User's PIX baseline (CSM cascade rasterize + PCSS evaluator combined): **~3 ms**

**Decision: SWAP (with phase-2 follow-up).** RT cost (0.21 ms) is ≈ 5.5× cheaper than the CSM cascade rasterize alone (1.17 ms) and ≈ 14× cheaper than the user's PIX-measured CSM+PCSS combined baseline (~3 ms). Far below the task brief's "≤ baseline" threshold. Quality benefit on top: paper-faithful angular-sun soft shadows, no cascade seams, no acne/peter-panning bias-tuning, single physically-meaningful parameter (`SUN_ANGULAR_RADIUS`) instead of `LIGHT_SIZE`/`PENUMBRA_MAX_TEXELS`/`MIN/MAX_SHADOW_BIAS` knobs.

**Phase 2 (separate CL) will:**
1. Flip `USE_RT_SHADOWS` to 1 (or remove the toggle entirely once committed).
2. Delete `SunShadowGeometryProcessPass` + `SunShadowCullingPass` + `SunShadowBlur*Pass` (verify no other consumers via grep).
3. Delete `common/shadowResolver.hlsl::SunShadowResolver` (orphaned after the swap).
4. Drop t7 (CSM atlas) binding from LightPass; t13 (RT visibility) becomes the only sun shadow input.
5. Visual validation passes: AC#6 (PT-reference cross-check), AC#7 (GITestBox acne/peter-panning regression check), AC#8 (orphan-consumer grep).

**AC#3 GBV: pre-existing failure, not regression.** GBV smoke exits 1 on `OpaquePass_RT_0` cross-queue transition tracker mismatch (`Before state COMMON does not match RENDER_TARGET`). Verified pre-existing by `git stash` of this CL's changes — same error, same exit code 1. Filed as out-of-scope follow-up (graphics-api-expert territory; tracker reconciliation between OpaquePass renderer-state and the cross-queue COMMON transitions chained through SSAOPass / RadianceCacheReprojectionPass / RadianceCacheRaytracingPass / SunShadowRTPass). Smoke (`-total_frames 30`) without `-gpu_validation` exits 0.

**Phase 1 build evidence:** `BuildWin.ps1` exit 0 (engine + DXIL deploy, both runtime exes built — `Main.exe`, `RenderTest.exe`). `HLSL2DXIL_NoPause.ps1` compiled all 5 SunShadowRT shaders + lightPass.comp + GPUPathTracerRayGen.hlsl successfully (lib_6_3 profile). DXR PSO created with 5 subobjects per the engine log. RT pass dispatches every frame (no skip warnings).

**What was NOT verified in phase 1 (deferred to phase 2):**
- Visual capture vs PT reference (AC#6) — phase 2 closes this with windowed GISponza orbit + PT cross-check against `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/`.
- GITestBox acne/peter-panning regression check (AC#7) — phase 2.
- Steady-state windowed framerate over a 5-minute walkthrough (the task brief's explicitly-listed gap).
- Hardware-tier sensitivity (the task brief's other listed gap; only one machine measured).
- TAA-off behaviour with RT shadows (single-jittered RT sample without TAA accumulation will be visibly noisy — the design alignment artifact called this out; deferred until TAA-off becomes a supported config).
- AC#8 orphan-consumer grep — phase 2 (depends on the actual swap).
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
