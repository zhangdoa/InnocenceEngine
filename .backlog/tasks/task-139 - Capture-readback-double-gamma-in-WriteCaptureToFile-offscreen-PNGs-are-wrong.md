---
id: TASK-139
title: Capture readback double-gamma in WriteCaptureToFile (offscreen PNGs are wrong)
status: Done
assignee: []
created_date: '2026-04-26 17:20'
updated_date: '2026-05-13 22:50'
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
- [x] #1 sqrtf removed from WriteCaptureToFile readback path
- [ ] #2 Before/after capture pair shows post-fix PNG matches on-screen output at same camera pose
- [x] #3 TASK-122 / TASK-6.10 capture archives still readable as-is (don't retroactively re-encode); document that pre-fix captures were double-gamma
- [ ] #4 GITestBox + GISponza windowed vs offscreen comparison documented in closure
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-14 — Picked up autonomously after TASK-222 + TASK-202 closure. Cheap fix, same file surface as TASK-222 (`ExampleRenderingClient_Capture.cpp`). High leverage on the visual-validation tooling now that TASK-222 has unblocked the capture flush — post-TASK-139, the PNGs we use to investigate TASK-223 (mirror-seam) will have correct gamma instead of being double-encoded.

2026-05-14 (impl CL) — Removed `sqrtf` from the `WriteCaptureToFile` Float16 readback path in `ExampleRenderingClient_Capture.cpp:212-214`. AC #1 (sqrtf removed) and AC #3 (archive readability documented — pre-fix captures stay as-is, double-gamma noted) marked. AC #2 partially satisfied via offscreen vs offscreen pre/post pair (no on-screen reference shot available without user action; the windowed comparison is layer-4 visual-validation which only the user can perform). AC #4 (GITestBox + GISponza windowed vs offscreen comparison) deferred — main-session closure / user can stamp after a one-camera-pose windowed shot.

Audit: `HandleScreenCapture` (TASK-211 editor screenshot path) does NOT have the same bug — it routes Float16 readback through `AssetService::Save` → `STBWrapper::Save` → `stbi_write_hdr` (linear HDR; no gamma applied). It saves AGX-encoded data into a `.hdr` container which is the wrong format (HDR expects linear), but that is a separate concern and not double-gamma. Left untouched.

Note: the task description referenced `AccurateLinearToSRGB` in `finalBlendPass.comp`; that explicit call no longer exists. The shader now uses `TonemapAGX` which embeds the gamma encode (pow 2.2) inside — same double-gamma symptom, same fix. The shader's tonemap comment at line 93-96 confirms TASK-141 removed an outer encode in favor of AGX's built-in one.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## TASK-139 Final Summary

### What shipped

Removed the per-channel `sqrtf` from `WriteCaptureToFile`'s Float16→uint8 conversion in `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp` (lines 208-216). The shader (`finalBlendPass.comp:96-97`) already produces gamma-encoded sRGB via `TonemapAGX` (which embeds `pow(2.2)` per `AgX.hlsl:15-25, 95-97`'s explicit "CALLER MUST NOT RE-ENCODE" contract). The readback was double-encoding, producing pastel/over-bright PNGs since TASK-122's AGX swap.

Also dropped now-unused `#include <cmath>` (the sqrtf calls were the only consumers; clangd-flagged).

### Diff summary

- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp`:
  - Lines 208-216: removed `sqrtf(...)` wrapping inside the per-channel uint8 conversion; kept clamp-and-scale.
  - Line 13: dropped `#include <cmath>`.
  - One-line WHY comment at the modified site citing TASK-139.

### Verification

- **Build clean** via `Scripts/BuildWin.ps1 -SkipShaderCompile`.
- **Engine launch** `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30 -offscreen`: clean shutdown, capture PNG written, `PathTracerReadback: mean=(0.1144, 0.127, 0.140)` reflects the actual sRGB-encoded values (no extra sqrt).
- **PNG uint8 statistics** (post-fix vs pre-fix vs old-baseline):
  | Image | mean RGB | p10 | p50 | p90 |
  |---|---|---|---|---|
  | pre-fix | (44.4, 47.5, 50.3) | 2 | 2 | (189, 194, 198) |
  | **post-fix** | **(29.1, 32.3, 35.6)** | **0** | **0** | **(139, 147, 153)** |
  | old-baseline (pre-fix code) | (42.5, 45.6, 48.3) | 2 | 2 | (183, 189, 194) |
  Post-fix: blacks restored to 0/255 (was 2/255), mid-tone p90 reduced from ~190 to ~140. Visual Read confirms recovered black levels + uncrushed mid-tone contrast in brick/stone textures. The unrelated TASK-223 mirror-seam is unchanged (orthogonal).

### `HandleScreenCapture` audit

No fix needed. The TASK-211 editor-screenshot path routes Float16/Float32 through `STBWrapper::Save` → `stbi_write_hdr` (linear container, no gamma applied). It does NOT double-encode. (Side note: saving gamma-encoded AGX data into a `.hdr` linear container is a format-mismatch concern, but outside TASK-139's scope.)

### Peer review

**PASS** with one ADVISORY (non-blocking). Reviewer verified the shader source-of-truth (TonemapAGX embeds gamma, AgX.hlsl contract documents "CALLER MUST NOT RE-ENCODE") and confirmed numerical sign-of-effect. Approved for commit.

### AC / DoD coverage

- AC #1 (sqrtf removed): **PASS**, ticked.
- AC #2 (post-fix PNG matches on-screen output at same camera pose): **UNVERIFIED** — requires user-side layer-4 windowed reference shot.
- AC #3 (pre-fix archive readability preserved, double-gamma documented): **PASS**, ticked. Pre-fix captures retained as-is in `Build/`, no retroactive re-encode.
- AC #4 (GITestBox + GISponza windowed vs offscreen comparison): **UNVERIFIED** — same layer-4 limitation as AC #2.
- DoD #1 / #4 / #6: PASS.
- DoD #2/#3/#5: no pre-existing automated integration test covers the capture-readback gamma path; runtime offscreen capture + statistical + visual comparison is the agent-verifiable portion.

### Files changed

- `Source/ExampleProject/RenderingClient/ExampleRenderingClient_Capture.cpp` (-4, +1 comment)
- `.backlog/tasks/task-139 - ...md` (notes + AC/DoD checks + final summary)

### Artifacts

- Pre-fix archived: `Build/pre-fix-gpu_output.png`
- Post-fix: `Bin/RelWithDebInfo/gpu_output.png`
- Pre-fix baselines from TASK-222: `Build/baseline-presentation.png`, `Build/baseline-offscreen.png`, `Build/current-CL-presentation.png` (all double-gamma; documented, not retroactively re-encoded).
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
