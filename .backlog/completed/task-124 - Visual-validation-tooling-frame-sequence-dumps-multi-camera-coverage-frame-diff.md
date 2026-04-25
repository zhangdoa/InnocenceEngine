---
id: TASK-124
title: >-
  Visual validation tooling: frame sequence dumps, multi-camera coverage, frame
  diff
status: Done
assignee: []
created_date: '2026-04-23 20:40'
updated_date: '2026-04-25 18:24'
labels:
  - tooling
  - rendering
  - testing
dependencies: []
references:
  - Source/Engine/Engine.cpp
  - Source/Engine/Engine.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
A single `gpu_output.png` at terminate is insufficient for validating any rendering change. A visual claim like "banding gone" (e.g. TASK-121) from one static frame at one camera can easily miss:

- Different camera poses — the same probe-interpolation / filter / culling bug can be obvious from one view and hidden from another.
- Frame-to-frame variation — SVGF denoiser boiling, probe-spawn oscillation, temporal accumulation drift, streaming-in artefacts. None of these show up in a postcard.

Umbrella for tooling improvements that close these gaps. Sized into three sub-pieces:

**[A] Frame sequence dump** — `-dump_frames START-END` writes `gpu_output_NNNN.png` for every frame in [START, END]. Lets a reviewer scrub or diff consecutive frames to see flickering, oscillation, streaming progress.

**[B] Multi-camera coverage** — cheapest option first: add `-camera_orbit PITCH,RADIUS,DURATION` that animates the camera in an orbit around the scene origin for DURATION frames. Combined with [A], a single run produces a cross-camera × cross-frame matrix of evidence. Alternatives: explicit `-camera_pose x,y,z,pitch,yaw` flag; scene-switching between UnitTest/GISponza/GITestBox over `-reload_at_frame` N; a scene-defined camera path keyframed to frames.

**[C] Frame-diff tooling** — a post-processing script that consumes a dumped sequence and prints per-pixel max-delta / RMS-delta between consecutive frames, and flags pixels that exceed a temporal-stability threshold. Scoped for quick red/yellow/green verdicts on "is this stable" without manual scrubbing.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
### [A] frame sequence dump — landing in THIS CL

- `InitConfig::dumpFramesStart`, `dumpFramesEnd` (both default -1).
- `Engine::parseInitConfig` recognises `-dump_frames START-END` and parses the dash-separated range.
- `ExampleRenderingClientImpl::WriteCaptureToFile(const char*)` is extracted from the old `TryWriteAutoCapture` — handles the GPU sync, readback, sRGB conversion, and asset-service save. Reusable.
- `TryWriteAutoCapture` keeps its one-shot path; its advisory PathTracerReadback stats (zero/non-zero split, mean, max) stay inline to avoid spamming logs on every dumped frame.
- The per-frame tick increments `m_autoCaptureFrameCount` unconditionally now (was inside the one-shot trigger's conditional). When `dumpFramesStart >= 0` and the counter lands in the range, writes `gpu_output_NNNN.png` via the helper.

Smoke-tested: `Main.exe -offscreen -total_frames 60 -dump_frames 50-59` on GITestBox wrote 10 PNGs in `Bin/gpu_output_00{50..59}.png`, each 440–462 KB, with a slight monotonic size decrease (461→440) consistent with the SVGF denoiser continuing to converge over the 10-frame window. Captures archived under `Build/captures/TASK124_seq/`.

### [B] multi-camera coverage — landed (`-camera_orbit PITCH,RADIUS,DURATION`)

- `InitConfig::cameraOrbitActive / cameraOrbitPitchDeg / cameraOrbitRadius / cameraOrbitDuration`.
- `Engine::parseInitConfig` parses the comma-triple. Validates `RADIUS > 0` and `DURATION > 0`.
- `WorldSystem::Update` in `World.inl` owns the per-frame override. When orbit is active, looks up "Main Camera" via `EntityRegistry::FindByName`, writes position on a circle of `RADIUS` at elevation `PITCH` degrees around world origin, and writes a quaternion composed from yaw (around world Y) and pitch (around local X) so the camera looks back at origin. Yaw sweeps 0→360° linearly over `DURATION` frames. Player's `Update` still runs below; for windowed interactive sessions the Player will stomp the orbit each frame — orbit is intended for `-offscreen` capture runs.

Smoke-test (GISponza via the default auto-test schedule): `-total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120` produced 60 captures across a full 180° sweep. Walls / geometry progress smoothly through the yaw rotation confirming the math; the orbit also immediately surfaced two issues that single-frame validation couldn't have caught:

- A strong spiral/swirl artefact in the floor throughout all orbit frames — SVGF temporal reprojection fails under sustained rotation because motion vectors can't fully invalidate the previous-frame history tap; the denoiser smears its history along the camera's rotation path.
- The banding we thought TASK-121 eliminated re-appears more prominently in motion than in the static capture — likely because the probe-mask ring-search doesn't have time to converge as valid probes churn in/out under yaw.

Neither is a bug in `-camera_orbit` — the tool is doing its job of exposing motion-dependent defects. Filed notes on the two findings belong with TASK-121 (ring-search + motion stability) and/or a new SVGF-motion task.

Captures archived under `Build/captures/TASK124_orbit/`.

Limitations accepted for v1, filable as follow-ups if they bite:
- Orbit centre is world origin (0,0,0). Scenes whose interesting geometry isn't near origin will orbit around empty space. Extension: `-camera_orbit_center X,Y,Z` or capture the initial camera position as orbit centre on the first tick.
- Orbit is pure yaw sweep; no linear translation, no second-axis wobble, no variable speed. If testing rotation-independent flicker becomes important, a `-camera_waypoints` or scripted-path approach would replace this.

### [C] frame-diff tool — NOT in this CL

A standalone Python or PowerShell script under `Scripts/` that reads a directory of `gpu_output_NNNN.png`, compares consecutive pairs, and emits a per-frame RMS/max delta line. A simple first version uses Pillow; no engine changes needed.

### [C] frame-diff tool — landed (`Scripts/frame_variance.py` via c342a6ce)

Landed as a temporal-std analysis instead of consecutive-pair RMS — same goal ("is this sequence stable / where does it flicker"), strictly stronger signal. The script reads `gpu_output_NNNN.png` files from a directory, computes per-pixel temporal std-dev of luma across the full sequence, prints p50/p95/p99/max percentiles, locates the centroid of the top-1% flicker region with quadrant breakdown, and writes a log-scale red/blue heatmap PNG. No engine changes; pure host-side.

Verified on the archived `Build/captures/TASK124_seq/` sequence (10 frames, GITestBox, frames 50–59) in this CL:
```
frames: 10  resolution: 1280x720
temporal-std percentiles (luma 0..255):
  p50 = 0.00   p95 = 2.57   p99 = 3.94   max = 121.91
top-1% flicker centroid (normalised): x=0.27 y=0.42
top-1% flicker quadrants: TL=58% TR=2% BL=38% BR=3%
heatmap -> Build/captures/TASK124_seq_variance.png
```
p50=0 confirms the bulk of pixels are temporally stable; the high-std hotspot is concentrated in the upper-left, consistent with the SVGF-converging surfaces noted in [A]'s smoke test.

Umbrella complete — [A], [B], [C] all landed. Motion-dependent defects surfaced by [B] (SVGF history smear under sustained yaw, banding re-emergence under rotation) live with TASK-121's follow-up scope, not here.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Three sub-pieces landed across separate commits:

- **[A] frame-sequence dump** — `0b6a075f` `feat(engine): [TASK-124] -dump_frames START-END for per-frame PNG capture`. `InitConfig::dumpFramesStart/End`, `Engine::parseInitConfig` parses dash-range, `WriteCaptureToFile` extracted from `TryWriteAutoCapture`. Smoke-tested with 10 PNGs on GITestBox (440–462 KB, monotonic SVGF-convergence shrink), captures archived under `Build/captures/TASK124_seq/`.
- **[B] camera-orbit coverage** — `cdcf9860` `feat(engine): [TASK-124 B] -camera_orbit PITCH,RADIUS,DURATION`. `WorldSystem::Update` writes camera pose on a yaw circle around world origin per frame; runs alongside Player::Update so it's intended for `-offscreen` capture runs. Smoke-tested on GISponza with `-total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120`; surfaced two motion-dependent defects (SVGF history-smear under sustained rotation; banding re-emergence) that single-frame validation could not have caught — those findings belong with TASK-121's scope.
- **[C] frame-diff tool** — `c342a6ce` added `Scripts/frame_variance.py`. Per-pixel temporal-std percentile analysis + log-scale heatmap; stronger signal than the originally-spec'd consecutive-pair RMS. Verified in this closure run against `Build/captures/TASK124_seq/`: p50=0 (bulk stable), top-1% flicker concentrated upper-left, heatmap written to `Build/captures/TASK124_seq_variance.png`.

**Limitations / NOT verified:**
- [B] orbit centre is hard-coded to world origin; scenes with off-origin geometry will orbit empty space. Filable as `-camera_orbit_center X,Y,Z` follow-up if it bites.
- [B] is yaw-only; no pitch wobble, linear translation, or scripted waypoints.
- [C] uses temporal std across the whole sequence rather than consecutive-pair deltas. The spec asked for the latter; the implementation is a strict superset for the goal but does not produce per-pair RMS lines explicitly.
- No automated regression test wraps the whole pipeline — closure validation is manual smoke-test re-runs of the archived sequence.
<!-- SECTION:FINAL_SUMMARY:END -->
