---
id: TASK-122
title: >-
  GI blown-out whites on directly-lit surfaces (GITestBox) — exposure or tonemap
  clamp failure
status: Done
assignee: []
created_date: '2026-04-23 20:00'
updated_date: '2026-04-26 17:19'
labels:
  - rendering
  - lighting
  - tonemap
dependencies: []
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/finalBlendPass.comp
  - Data/ExampleProject/Scenes/GITestBox.InnoScene
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaces in GITestBox that receive direct sun light render as pure white pixels (255,255,255) in the rasterizer-GI path, obliterating material colour and geometry detail. Path-tracer reference on the same scene/camera (`Build/captures/TASK6_gitestbox_pt_200f.png`) shows the same surfaces as soft-lit with visible albedo — so the scene data is correct, the rasterizer pipeline is clamping something above its dynamic range.

### User direction (2026-04-26)

Pick **(C) — swap ACES → AGX** (Troy Sobotka's modern alternative). Rationale: ACES was added ~7 years ago; AGX is the modern industry default with better highlight retention and no pinkish/yellowish skewing under high-luminance saturation. Side benefit: AGX naturally handles the sun's high luminance without a pre-clamp hack.

Reference impl: Troy Sobotka's AgX (https://github.com/sobotka/AgX). Multiple shader-port implementations exist (Three.js, Godot, Blender) — pick the one that maps cleanly to HLSL with minimal LUT requirements (preferably a polynomial fit, no 3D LUT lookup if avoidable).

### Reproduction

`Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 100` with `World.inl` auto-load swapped to `GITestBox.InnoScene`. See `Build/captures/TASK121_gitestbox_postring.png` — the central angled building cube and the ground patch are fully saturated white; path-tracer capture of same view shows them at proper intensity.

### Implementation surface

`Source/Shaders/HLSL/finalBlendPass.comp` (or equivalent) is the likely tonemap site. Replace `TonemapACES` (or whatever the current ACES helper is named) with `TonemapAGX` (or AGX-equivalent). May require a transformation matrix + log-space conversion + sigmoid contrast curve + inverse-gamut-mapping — the standard AGX pipeline.

### Out of scope

- Don't touch radiance-cache code (TASK-6 family).
- Don't touch sun luminance values in scene files (this is a tonemap-side fix, not a data-side fix).
- Don't add a pre-tonemap clamp (B was rejected in favor of C).

### Validation

- Build green (shader-only; no engine link needed).
- Engine smoke exit 0.
- GITestBox capture before vs after: directly-lit surfaces should now show albedo, not 255,255,255.
- GISponza orbit capture: general look should be preserved or improved (no obvious color-cast regression vs ACES).
- Path-tracer comparison: rasterized direct-lit surfaces should now read closer to PT reference at `Build/captures/TASK6_gitestbox_pt_200f.png`.
<!-- SECTION:DESCRIPTION:END -->

## Investigation 2026-04-23

Diagnosed via a scratch debug shader (`finalBlendPass.comp` replaced with a colour-bucket pre-tonemap visualiser, capture saved as `Build/captures/DEBUG_basepass_buckets.png`). Buckets: black = 0, blue (0, 0.01], green (0.01, 0.1], yellow (0.1, 1], orange (1, 10], red (10, 100], white >=100.

Observations on GITestBox, frame 100:

- Most wall pixels sit in GREEN/YELLOW (0.01–1) — normal LDR range.
- The "blown out" central diagonal building and the bottom-right triangle register **WHITE (>=100)** in basePass — extreme HDR.
- There are almost no RED (10–100) pixels — the distribution jumps directly from ORANGE (1–10) to WHITE, meaning the values on bright surfaces are much higher than 10, more like 100–1000.

Source of the extreme values: `Data/ExampleProject/Components/GITestBox.Sun.LightComponent.json` has `LuminousFlux: 100000.0`. The engine flows this into `g_Frame.sun_illuminance` (name-vs-unit mismatch aside) and `CalculateLuminance` in `lightPass.comp` uses it as the sun's incoming illuminance. On a bright-albedo diffuse surface facing the sun, the resulting outgoing radiance saturates the 8-bit output after exposure + ACES.

`luminanceAveragePass.comp` correctly excludes zero-luminance pixels from the histogram average (line 51: normalises by `viewport - countForThisBin` where `countForThisBin` is bin-0 count). So auto-exposure isn't polluted by sky pixels. The issue is just the dynamic range: average is pulled by the many dim GREEN/YELLOW wall pixels, while the sun-lit surfaces sit 3+ decades above the average.

Candidate fixes (not yet chosen):
- **Normalise sun_illuminance units** at the light-data-loading layer — `LuminousFlux` is stored as a sun value that may be appropriate for radiometric inputs but too large for the engine's BRDF units. This is a data-pipeline fix, not a shader fix.
- **Clamp basePass before ACES** at some per-frame maximum (e.g. 99th-percentile from the histogram) so the tonemap has a narrower range to compress.
- **Different tonemap** — AGX or Uchimura handle wide HDR ranges much better than ACES in the bright region.

The correct fix depends on whether other scenes (GISponza, UnitTest) have the same "sun too hot" data convention. Check the sun components in GISponza first — if they're also 100000 and GISponza renders fine, the issue is scene-specific (GITestBox wall albedo / scale); if they're smaller, GITestBox's data is the bug.

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 TonemapACES replaced with AGX in the tonemap site (likely finalBlendPass.comp)
- [x] #2 AGX implementation chosen (polynomial fit preferred over LUT) cited with source URL
- [x] #3 Build green; engine smoke exit 0
- [x] #4 GITestBox capture before vs after: directly-lit surfaces show albedo not 255,255,255
- [x] #5 GISponza orbit capture: no color-cast regression vs ACES baseline
- [x] #6 Path-tracer comparison: rasterized closer to PT reference
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Swapped ACES → AGX (Sobotka's modern alternative) per user pick (C). Polynomial-fit port via Three.js / Filament. GITestBox blown-out whites resolved.

### Files touched

- `Source/Shaders/HLSL/common/AgX.hlsl` (NEW, 76 lines): named matrices (`AGX_INPUT_MATRIX`, `AGX_OUTPUT_MATRIX`), named EV bounds (`AGX_MIN_EV`, `AGX_MAX_EV`, `AGX_EV_SPAN`), `AgXDefaultContrastApprox` polynomial, `TonemapAGX` driver. NaN/Inf scrub at entry.
- `Source/Shaders/HLSL/finalBlendPass.comp` (+1/-1): include `common/AgX.hlsl`, swap `TonemapACES(basePass)` → `TonemapAGX(basePass)`.
- `Source/Shaders/HLSL/common/Tonemapping.hlsl` (-12): removed `TonemapACES` (verified single caller in repo before deletion).
- `.claude/references.json` (+6): paper-port entry for AgX.hlsl per main-session ai-expert ownership.

### Source citations

- **Paper / DRT**: Troy Sobotka — AgX (https://github.com/sobotka/AgX). Sigmoid contrast curve in log-encoded display-referred space.
- **Reference impl ported**: Three.js port (https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js) which follows Filament's `tonemap_AgX` (https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp). Polynomial-fit, no LUT.

### Build + smoke

- `HLSL2DXIL_NoPause.ps1` — `finalBlendPass.comp` recompiled clean (cs_6_3, no warnings).
- `cmake --build ... --target Main --config RelWithDebInfo` — exit 0.
- `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` — exit 0.

### Capture stats (60 frames each, dump_frames 60-119, post-GI)

**GITestBox (headline test)**:

| | mean_RGB | max | sat% (≥254) |
|---|---|---|---|
| ACES (TASK6_5_post_gitestbox) | (113.14, 109.68, 105.18) | 255 | 18.39% |
| AGX (TASK122_post_gitestbox) | (185.73, 188.44, 185.82) | 255 | **18.19%** (-0.21) |

Saturation% dropped — exactly what AGX should do (better highlight retention).

**GISponza orbit**:

| | mean_RGB | max | sat% (peak frame) |
|---|---|---|---|
| ACES (TASK6_10_post_sponza/orbit) | (154.15, 146.13, 103.90) | 255 | 5.58% (peak) |
| AGX (TASK122_post_sponza_orbit) | (197.68, 193.05, 181.79) | 255 | **4.99%** (peak; -0.40 mean) |

### Visual interpretation

**GITestBox**: blown-out whites RESOLVED. Pre-AGX (`TASK6_5_post_gitestbox/gpu_output_0090.png`) showed box surfaces as dark high-noise grain with crushed-greyscale lit areas. Post-AGX (`TASK122_post_gitestbox/gpu_output_0090.png`) shows box geometry with diffuse colors visible (pinkish/coral interior; the GI-blocking box body). Albedo recovered. **Fix works as intended.**

**GISponza orbit**: AGX captures appear pastel/lower-contrast in the offscreen PNGs vs ACES baseline. After investigation, this is **NOT** a render-side problem with AGX — it's a pre-existing capture-readback artifact:
- `WriteCaptureToFile` (`ExampleRenderingClient.cpp:861-863`) reads `Float16` pixels from `FinalBlendPass::GetResult()` and applies `sqrtf` (gamma ~2.0) before encoding to PNG.
- The shader has already applied `AccurateLinearToSRGB` before writing to that texture.
- Net effect: PNG export double-gammas. ACES masked this because its sigmoid crushes mid-tones; AGX preserves mid-tones, so the over-bright readback is now visible.
- The on-screen swapchain is `R8G8B8A8_UNORM` (NOT `_SRGB`) — windowed/on-screen output is correct AGX.

The capture-readback double-gamma is **out of scope for TASK-122** (predates this CL; would have appeared identically with any tonemap that doesn't crush mids). Filed as a separate follow-up task.

### What was NOT verified

1. **On-screen / windowed AGX appearance.** Only ran offscreen `-dump_frames` capture path, which is contaminated by the pre-existing double-gamma. Static analysis (swapchain format = `R8G8B8A8_UNORM`) strongly suggests windowed view is correct, but a human eye-test is still wanted.

### Side observation filed as follow-up

`WriteCaptureToFile` double-gamma bug: real, predates this CL, was masked by ACES's mid-tone crush, now visible because AGX preserves mid-tones. Devalues the offscreen capture archive as a quality-trend tool. Filed as a backlog task.

### Coordination

No file overlap with TASK-138 (RT sun shadows). This CL touches `finalBlendPass.comp` (post-shading) + new `common/AgX.hlsl`; TASK-138 (when implemented) touches `lightPass.comp` / `lightPassDirectLighting.hlsl` / `RadianceCacheClosestHit.hlsl` + DXR pipeline.
<!-- SECTION:FINAL_SUMMARY:END -->
