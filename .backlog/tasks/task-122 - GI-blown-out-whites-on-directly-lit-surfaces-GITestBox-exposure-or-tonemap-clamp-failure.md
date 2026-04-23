---
id: TASK-122
title: 'GI blown-out whites on directly-lit surfaces (GITestBox) — exposure or tonemap clamp failure'
status: To Do
assignee: []
created_date: '2026-04-23 20:00'
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

Reproduction: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 100` with `World.inl` auto-load swapped to `GITestBox.InnoScene`. See `Build/captures/TASK121_gitestbox_postring.png` — the central angled building cube and the ground patch are fully saturated white; path-tracer capture of same view shows them at proper intensity.

Candidate causes (not yet investigated):
- `finalBlendPass.comp` auto-exposure (`in_luminanceAverage[0]`) may not be scaling for this scene's light intensities.
- Sun light intensity in the scene file may be set for Sponza and way too hot for the smaller test box.
- `TonemapACES` may be receiving HDR values that saturate its ceiling curve.
- Emissive / sun disk writing unbounded radiance into `basePass` RT0 without any clamp.

Out of scope: radiance-cache banding (fixed in TASK-121), black fallback regions (filed as TASK-123).
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
