---
id: TASK-125
title: 'GI motion-dependent defects: SVGF reprojection smear + banding re-emergence under camera rotation'
status: To Do
assignee: []
created_date: '2026-04-23 20:55'
labels:
  - rendering
  - GI
  - radiance-cache
  - motion
dependencies: []
references:
  - Source/Shaders/HLSL/GIDenoise.comp
  - Source/Shaders/HLSL/GIATrousCommon.hlsl
  - Source/Shaders/HLSL/RadianceCacheReprojection.comp
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced by the TASK-124 orbit smoke test on GISponza:
`Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120`. Captures archived at `Build/captures/TASK124_orbit/`.

Two distinct motion-only defects the static-camera validation of TASK-121 couldn't possibly catch:

### Defect 1 — SVGF spiral / swirl smear on the floor

Throughout every orbit frame, the floor region carries a strong circular-swirl artefact centred roughly at the image centre. Frame-to-frame progression shows the swirl rotating opposite to the camera yaw — a clear temporal-reprojection signature: the denoiser's history buffer is reused from a previous yaw, motion vectors partially account for it, and the residual misregistration accumulates into the swirl.

Suspects:
- `GIDenoise.comp` uses a single reprojected tap (post [I.3e.3] A-trous split); under fast yaw, the reprojected tap lands on a pixel whose previous-frame content was from a different probe / surface hit. Depth-ratio validity gate (5%) is too lax under motion, and normal gate isn't strict enough either.
- A-trous stride-4 kernel has wide spatial support — mis-registered history bleeds across ~9 pixels per stride, smearing the error into the swirl's radial streaks.
- Motion vectors from `OpaquePass` may not be fully accurate for the 2D reprojection model when the whole scene is rotating (angular motion vs linear motion handling).

Likely fixes (to investigate):
- Tighten the depth-ratio validity gate under high motion (detect via motion vector magnitude) — snap to 100% raw irradiance when motion exceeds a threshold.
- Variance-adaptive blend rate on reprojection mis-match: if the reprojected tap's luma differs from the current tap's luma by more than 2× the smoothed temporal variance, treat as disocclusion.
- Per-pixel motion-vector-length gate: above N pixels/frame motion, force N=1 (reset moments history) rather than carrying stale E[L], E[L²].

### Defect 2 — Per-tile banding re-emerges under rotation

The banding TASK-121 eliminated on static GITestBox flat walls reappears more strongly on GISponza walls during orbit. The ring-search substitution in `SampleRadianceCache` works per-frame, but under yaw:
- Probes that were valid last frame (with baked SH) shift off-screen or change tile coverage.
- Probes that are newly-on-screen are invalid for a few frames until the sparse spawn schedule spawns them.
- The ring-search picks substitutes whose SH was also baked under the old yaw — their radiance doesn't match the current pixel's world direction as well.

Effectively the `FindClosestProbe` substitutes are stale under motion, and the bilinear blend of stale probes produces the same tile-constant appearance the original [I.1] bug did.

Likely fixes:
- Prioritise recently-updated probes over nearby but stale ones. Add a "last updated frame" tag to the probe mask; ring-search prefers candidates within N frames of current.
- Force higher spawn density under rotation (camera angular velocity adjusts `upscaleFactor`) so probes repopulate faster.
- World-cache fallback (TASK-123 candidate fix) would also help here: when probes are stale, fall back to the world cache instead of blending stale SH.

### Reproduction recipe

```
# In Bin/:
RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen \
    -total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120
```

Then compare consecutive `Bin/gpu_output_0060.png` through `gpu_output_0119.png` (or the archived `Build/captures/TASK124_orbit/gpu_output_0060/75/90/105/119.png`). The swirl on the floor and the per-tile banding are visible frame-to-frame.

### Why this was not caught by TASK-121's validation

TASK-121's close-time evidence was a single static frame on a scene where flat-wall coverage happened to have all 4 bilinear corners valid. Under rotation, the probe-mask state churns per frame, and the motion-dependent failure modes above take over. Direct consequence of the validation-methodology gap TASK-124 addresses — filed as a concrete follow-up artefact of using the new tooling.
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
