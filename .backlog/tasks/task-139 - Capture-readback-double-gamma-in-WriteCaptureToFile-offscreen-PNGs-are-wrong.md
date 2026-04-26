---
id: TASK-139
title: Capture readback double-gamma in WriteCaptureToFile (offscreen PNGs are wrong)
status: To Do
assignee: []
created_date: '2026-04-26 17:20'
labels:
  - rendering
  - tooling
  - captures
  - bug
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
  - Source/Shaders/HLSL/finalBlendPass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during TASK-122 (AGX swap, 2026-04-26). The offscreen-mode capture path applies `sqrtf` (gamma ~2.0) on Float16 readback in `ExampleRenderingClient.cpp:861-863` (`WriteCaptureToFile`), but the shader already applied `AccurateLinearToSRGB` before writing to that texture. Net: every PNG written by `-dump_frames` and friends is double-gamma encoded.

### Why now

ACES (the prior tonemap) masked this via mid-tone crush. AGX preserves mid-tones, so the over-bright PNG readback is now visible: AGX captures look pastel / washed-out vs the actual on-screen result. Devalues the entire offscreen capture archive as a quality-trend tool — every PT-vs-rast comparison, every before/after diff, has had this confound baked in.

### Evidence

- TASK-122 closure documents: AGX captures look pastel in PNGs but the on-screen swapchain (format `R8G8B8A8_UNORM`, NOT `_SRGB`) shows correct AGX output. The output texture is correct; the export path is wrong.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:861-863` does `sqrtf` on each channel during readback.
- `Source/Shaders/HLSL/finalBlendPass.comp` writes `AccurateLinearToSRGB(...)` to the FinalBlend output (verify the exact line; this is the shader's own sRGB encode).

### Fix

Drop the `sqrtf` from the readback path. The shader's `AccurateLinearToSRGB` already produces display-referred sRGB; PNG should encode that directly without re-gamma. Validate by:
- Capturing a known scene (GISponza or GITestBox) before vs after the fix.
- Spot-checking that the post-fix PNG matches what the user sees on-screen via a windowed Main.exe at the same camera pose.

### Out of scope

- Don't touch the shader's tonemap or sRGB encode (those are correct).
- Don't migrate the capture path to a different image format (PNG is fine; just stop double-encoding).

### Why medium priority

Not a runtime regression — engine looks correct on screen. But every offscreen capture archive going forward is wrong-by-default until fixed; that affects every quality-comparison task. Cheap to fix (~1 line removal), high leverage on the validation tooling.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 sqrtf removed from WriteCaptureToFile readback path
- [ ] #2 Before/after capture pair shows post-fix PNG matches on-screen output at same camera pose
- [ ] #3 TASK-122 / TASK-6.10 capture archives still readable as-is (don't retroactively re-encode); document that pre-fix captures were double-gamma
- [ ] #4 GITestBox + GISponza windowed vs offscreen comparison documented in closure
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
