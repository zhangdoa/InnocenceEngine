---
id: TASK-125
title: 'GI motion-dependent defects: SVGF reprojection smear + banding re-emergence under camera rotation'
status: Done
assignee: []
created_date: '2026-04-23 20:55'
labels:
  - rendering
  - GI
  - radiance-cache
  - motion
  - paper-port
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

### Root cause: the denoiser diverged from paper §2.4.3 at the [I.3e] landing

Re-reading the paper text (`Build/GI1_0.pdf`, §2.4.3 "Denoising"):

> "These issues can be solved using a simple denoiser based on temporal accumulation and an adaptive spatial filter, where we compute the spatial filter radius depending on the number of samples accumulated in history. As not all pixels have a history, such as disoccluded pixels, we adapt the filtering radius to the number of accumulated samples to reduce the noise"

Figure 19 is captioned "Spatial filtering guided by dilated blur mask" and shows (a) disocclusion mask → (b) dilated blur mask → (c) filtered irradiance.

The paper specifies:
- **One spatial filter pass**, not a cascade.
- **Radius computed as a function of history count**: low N → large radius (hide undersampling), high N → small radius (preserve converged detail).
- **Edge-aware weights** on depth + normal (not luminance).
- **Dilated blur mask** as the radius driver.

Our [I.3e] implementation is a 3-pass SVGF à-trous with fixed strides 1/2/4, SVGF-moments-derived per-pixel temporal variance, and an SVGF-style luminance edge-stop (`σ_L`). None of that is in the paper. It's imported wholesale from Schied et al. 2017's spatiotemporal variance-guided filter — a denoiser for path tracers, not the radiance cache's specific output.

Why this happened: I (Claude) read §2.4.3's "adaptive spatial filter" phrase and pattern-matched it to SVGF from prior knowledge, instead of following the paper's explicit single-pass-radius-by-history design. Each [I.3e.1–4] subslice was internally consistent ("build toward SVGF") but the whole ladder was climbing the wrong wall. Capsaicin follows the paper's shape — variable-radius blur, `blur_mask = max(kGIDenoiser_MaxBlurMask - lighting.w, 0)`, radius = blur_mask's integer value. No à-trous, no SVGF moments.

The motion defects this task was filed for (spiral smear + banding re-emergence) are direct consequences of this divergence. SVGF was not designed to absorb the radiance cache's specific noise characteristics (probe churn under motion, sparse-spawn update cycle); fixed à-trous strides can't react to per-pixel history count the way the paper's radius-by-history scheme does.

### Scope correction: this is a re-alignment with the paper, not a "tune the denoiser"

The fix is not parameter tuning inside GIDenoise.comp / GIATrous*.comp. It's replacing that whole pipeline with the paper's spec:

1. Remove the three `GIATrousStride{1,2,4}.comp` passes entirely.
2. Rewrite `GIDenoise.comp` as a single spatial filter whose radius comes from a dilated blur mask of the history count (SVGF moments texture becomes unused — can be kept for diagnostic value or removed).
3. Build the dilated blur mask like Capsaicin: compute raw sample count per pixel in the temporal accumulation, dilate across a 3×3 neighbourhood (paper's Figure 19(b)), then drive filter radius = `max(BLUR_MASK_MAX - sample_count, 0)` or equivalent.
4. Keep the temporal accumulation pass's blend-rate adaptive on color_delta (Capsaicin's mechanism) — that's the part that actually addresses the spiral-smear-under-motion failure mode.

This is a substantial CL. It should be scheduled after reading the Capsaicin implementation of the full pipeline (not just the denoiser) and updating TASK-6's [I.3e] notes to record the divergence.

### Reference implementation to consult before fixing: Capsaicin (AMD GPUOpen)

`https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/` — AMD's reference for the GI-1.0 paper we ported in TASK-6. Clone to `Build/reference/Capsaicin/` (gitignored) for study before touching the denoiser.

The denoiser architecture is fundamentally different from our SVGF A-trous and directly addresses this task's defects:

- **ReprojectGI** (`src/core/src/render_techniques/gi1/gi1.comp`, ~line 3997) computes a *color delta* (current vs reprojected history luma, 1/8-EMA smoothed, `gi1.comp:4077`) and derives an adaptive blend rate `alpha_blend = saturate(1 - |color_delta| / lumaB)`. Under motion `color_delta` rises, `alpha_blend` drops, and —
- **Dynamic history cap** (`gi1.comp:4092–4097`): `max_sample_count = lerp(4, 8·N_max, alpha_blend)`. When a pixel goes unstable, the cap drops to 4 samples — old history gets scaled down to that cap (`lighting *= max_sample_count / lighting.w`), which is *exactly* the "evict stale history during motion" behaviour that our fixed `MAX_HISTORY_N = 32` + `MIN_TEMPORAL_ALPHA = 0.05` lacks. This is likely the single biggest fix for the spiral smear on the floor.
- **Spatial filter** (`gi1.comp:4104–` `FilterGI`) is a variable-radius blur (0–8 pixels) driven by a per-pixel *blur mask* = `max(kGIDenoiser_MaxBlurMask - lighting.w, 0)`. High-sample-count pixels get zero blur; low-sample-count pixels get wide blur to mask undersampling. Distinct from our A-trous 1/2/4 stride cascade.
- **Validity gates on reprojection** are stricter: `distance(world, previous_world) < cell_size` and `dot(normal, previous_normal) > 0.95` (we use 0.9 on normal and 5% depth ratio). `cell_size` is depth-scaled AND grazing-angle-boosted (`gi1.comp:4026`).

The banding-under-motion defect (defect 2 above) is less directly addressed by Capsaicin's denoiser — their `screen_probes.hlsl` uses a different spatial-interpolation scheme that's worth studying on its own. TODO on probe interpolation needs a separate reading pass; this notes section covers only the denoiser half.

Guidance for whoever picks this up: match Capsaicin's shapes before attempting tuning. The SVGF tweaks I (Claude) tried speculatively (`MIN_TEMPORAL_ALPHA = 0.02` + `MAX_HISTORY_N = 128`) gave ~10% p95 flicker reduction and were reverted once the user pointed out "read the reference, don't tune the paper". Mark this as the pattern to avoid — Capsaicin first, then diverge intentionally.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [x] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
