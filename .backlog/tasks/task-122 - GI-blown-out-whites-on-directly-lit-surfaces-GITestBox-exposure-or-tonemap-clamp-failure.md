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

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
