---
id: TASK-223
title: FinalBlend / readback path produces vertically-mirrored Sponza output
status: To Do
assignee: []
created_date: '2026-05-13 22:28'
labels:
  - rendering
  - bug
  - readback
  - finalblend
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Symptom

`Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30` (with or without `-offscreen` / `-gpu_validation`) auto-captures a `gpu_output.png` where the rendered Sponza scene is **structurally corrupted**:

- **Vertical mirror-seam** down the image centerline; left and right halves do not cleanly join. The right half appears to be a flipped duplicate of the left, not the actual right side of the scene.
- **Large triangular geometry blob** in the lower-center (white at pre-existing baseline, green-tinted post-TASK-222 fix when the readback samples correct content). Probably an untextured / NaN-shaded mesh.
- **Duplicated curtain / arch geometry** flanking both sides.

Stone arches and brick masonry are readable; the path tracer is producing coherent surfaces. The corruption is structural (geometry layout / viewport / blit), not a denoiser or shading artifact.

## Pre-existing scope

Surfaced during TASK-222 visual validation (2026-05-14). Bisect-via-stash confirmed: present at pre-CL HEAD in **both** presentation and offscreen modes. Pre-TASK-222 captures appeared dimmer/grey because the broken state-tracker made the readback sample mismatched data; with TASK-222 landed, the readback content is brighter and accurate, making the structural corruption clearly visible.

This bug was masked until now because:

- Under `-gpu_validation`, the D3D12 validation layer killed the engine before the PNG flush (TASK-222 fixed that).
- Without `-gpu_validation`, the readback silently produced dim/desaturated garbage that nobody scrutinised closely — early visual-validation agents read the captures as "clean Sponza" because the dominant content (recognizable curtains/arches) was present, and the mirror-seam wasn't called out.

## Likely root-cause space (unverified)

1. **Viewport / scissor halving** — the path tracer or FinalBlend pass renders into only half the width, then the readback or composition pass mirrors / duplicates it. Possible since `DispatchRays` width arguments or `RSSetViewports` setup is sometimes copy-pasted.
2. **Stereo / multi-view path mistakenly active** in non-VR runs — engine might have a leftover VR / split-screen branch that fires even when only one eye / view is configured.
3. **`ReadTextureBackToCPU` row-pitch / format handling** — DXGI `D3D12_RESOURCE_DESC` row pitch padding vs the readback buffer's tightly-packed write could halve-stride the image. Channels-swapped tint observed post-TASK-222 hints at a related DXGI_FORMAT vs UAV-format mismatch in the readback CPU-side copy.

## Reproduction

```
Scripts/BuildWin.ps1 -SkipShaderCompile
Bin/RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 30
# Inspect: Bin/RelWithDebInfo/gpu_output.png
```

Compare visually to `C:/GitRepo/InnocenceEngine/Build/baseline-presentation.png` (pre-TASK-222 HEAD) or `current-CL-presentation.png` (post-TASK-222).

## Deliverables

- Identify which stage of the pipeline introduces the mirror-seam.
- Confirm whether the green/white triangular blob is shared root-cause or a separate issue.
- Fix and re-capture: PNG shows a single coherent Sponza render (no vertical mirror, no untextured blob).

## Cross-references

- TASK-222 (FinalBlend readback transition under `-gpu_validation`) — the fix that exposed this bug; do not roll back. Reference PNGs:
  - `C:/GitRepo/InnocenceEngine/Build/baseline-presentation.png` (pre-CL, dim corrupt)
  - `C:/GitRepo/InnocenceEngine/Build/baseline-offscreen.png` (pre-CL, dim corrupt)
  - `C:/GitRepo/InnocenceEngine/Build/current-CL-presentation.png` (post-CL, bright corrupt with channel tint)
- Skill `visual-validation` — discipline that surfaced this (numeric metrics + dominant-content description never substitute for full visual inspection).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 #1 `Main.exe -total_frames 30` capture shows single coherent Sponza render — no vertical mirror-seam, no duplicated geometry, no untextured triangular blob
- [ ] #2 #2 Result holds in both presentation and `-offscreen` modes
- [ ] #3 #3 Pre-existing integration tests covering rendering remain green
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
