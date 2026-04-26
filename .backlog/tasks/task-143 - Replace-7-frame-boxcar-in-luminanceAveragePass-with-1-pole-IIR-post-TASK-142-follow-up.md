---
id: TASK-143
title: >-
  Replace 7-frame boxcar in luminanceAveragePass with 1-pole IIR (post-TASK-142
  follow-up)
status: To Do
assignee: []
created_date: '2026-04-26 18:51'
labels:
  - rendering
  - auto-exposure
  - smoothing
dependencies:
  - TASK-142
references:
  - .alignments/post-TASK-141-dark-pumping-shadow-diagnostic.md
  - Source/Shaders/HLSL/luminanceAveragePass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from the post-TASK-141 dimness/pumping diagnostic. Secondary contributor to brightness pumping (after TASK-142's K retune): the 7-frame boxcar in `luminanceAveragePass.comp:18-69` produces a hard step at frame 7 transition. Replace with a 1-pole IIR for smoother adaptation.

### Current implementation

`Source/Shaders/HLSL/luminanceAveragePass.comp`:
- Line 18: `static const uint numHistoryFrames = 8;`
- Lines 51-69: cyclic 7-slot buffer + uniform-weight average over slots 1..7

The boxcar is ~117ms at 60fps. When a frame slot rotates out, the average jumps. Visible as a sharp transient under AgX (where ACES had crushed it).

### Recommended fix

Replace the cyclic-buffer boxcar with a 1-pole IIR:

```hlsl
float adaptedLuminance = lerp(out_average[0], weightedAverageLuminance, 1.0 - exp(-dt / tau));
out_average[0] = adaptedLuminance;
```

Drops state from 8 floats to 1. tau ≈ 0.5-1.0 s for film-typical adaptation rate. Verify `g_Frame.deltaTime` exists in `PerFrame_CB` before committing — if not, derive from `g_Frame.frameIndex` delta-tracking or pass via CB.

### Coupling with TASK-142

The dominant cause of multi-second brightness pumping was the GI denoiser temporal cap (3-6s). The boxcar is a secondary 117ms transient on top. TASK-142's K retune already shrinks the GI-denoiser-driven swing's perceptual impact (steady state in flatter AgX region). Whether the boxcar transient is still perceptible after TASK-142 lands is the trigger to do this work — defer until user validates TASK-142.

### Out of scope

- Don't touch the histogram pass (`luminanceHistogramPass.comp`).
- Don't touch the GI denoiser temporal cap (separate concern; would require GI re-port work).

### Why medium priority

Quality-of-life smoothing. Not a visible regression. Safe to defer if TASK-142 alone gives acceptable adaptation feel.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 luminanceAveragePass.comp:18-69 replaced with 1-pole IIR using deltaTime
- [ ] #2 g_Frame.deltaTime confirmed available in PerFrame_CB (or derived/added)
- [ ] #3 tau named constant; documented choice rationale (~0.5-1.0 s film-typical)
- [ ] #4 Build green; smoke exit 0
- [ ] #5 User confirms boxcar transient is gone (subjective; require windowed eye-test)
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
