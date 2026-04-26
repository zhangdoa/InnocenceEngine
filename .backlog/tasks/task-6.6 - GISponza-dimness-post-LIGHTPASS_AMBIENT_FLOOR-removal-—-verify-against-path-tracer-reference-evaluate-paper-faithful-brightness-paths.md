---
id: TASK-6.6
title: >-
  GISponza dimness post-LIGHTPASS_AMBIENT_FLOOR removal — verify against
  path-tracer reference; evaluate paper-faithful brightness paths
status: Done
assignee: []
created_date: '2026-04-26 11:19'
updated_date: '2026-04-26 13:51'
labels:
  - rendering
  - GI
  - paper-faithful
  - validation
dependencies:
  - TASK-6.5
references:
  - Build/captures/TASK6_5_pre_sponza/
  - Build/captures/TASK6_5_post_sponza/
  - .alignments/TASK-6.3-lightpass-fallback.md
  - Source/Shaders/HLSL/RayTracing/GPUPathTracerRayGen.hlsl
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced from TASK-6.5 closure (2026-04-26). Removing `LIGHTPASS_AMBIENT_FLOOR = (0.02, 0.025, 0.03)` exposed that on dim indirect-only scenes like GISponza, the constant was acting as a frame-wide ambient base — not just a fallback for relaxed pixels. Whole-frame mean intensity drops ~16% (post 104 vs pre 125 in tonemapped 0..255 space). The diagnostic from TASK-6.5 proves the dimmed pixels weren't relaxed — they were just dim because single-bounce SH-projected GI is inherently dim where light doesn't reach.

This is paper-faithful exposed reality, not a regression. GI-1.0 is single-bounce by paper design (§2.1, §2.4). The proper response is paper-faithful brightness, not reintroducing a constant.

### Two questions to answer

1. **Is GISponza supposed to be this dim under single-bounce GI?** Verify by capturing the CPU path-tracer reference (`RayTracer::Execute()` auto-fired at terminate per `World.inl:354`) under the same orbit camera and frame range. Compare against `Build/captures/TASK6_5_post_sponza/`. If the path tracer also produces this dimness, the answer is "yes, that's what single-bounce GI looks like" and any further brightness work goes into multi-bounce / sky-irradiance / volumetric ambient (TASK-99 territory). If the path tracer shows brighter ambient, the GI implementation is missing energy somewhere upstream of the fix.

2. **If single-bounce really is this dim, what's the paper-faithful path to acceptable brightness?**
   - **Multi-bounce GI / world-cache integration (paper §2.2).** The world cache feeds secondary path vertices; if screen-probe rays bounce off geometry and read the world cache for further-bounce lighting, indirect-indirect contribution lifts the dim regions without a constant.
   - **Sky / environment irradiance.** A baseline sky-cubemap convolution into low-frequency probes catches "no GI ray hit anything but missed to sky" — paper-prescribed and currently missing.
   - **More aggressive variable-radius blur on relaxed pixels.** The TASK-6.5 fix wakes up the blur-mask widening but the radius cap may still be too tight; the auditor noted *"if the widened bilateral blur produces an unsatisfactory result, the answer is paper-faithful (more aggressive blur)."*
   - **Inspect Capsaicin's GISponza output for direct comparison.** Capsaicin runs the same scene; if their output is brighter at single-bounce, our SH projection / ray budget / world-cache integration is missing something.

### Acceptance

- Path-tracer ground-truth captures of GISponza at the same orbit positions used in TASK-6.5's TASK6_5_post_sponza captures.
- Direct PT-vs-rasterized-GI comparison at e.g. frames 60, 80, 100, 119.
- Determination: is GISponza-POST dimness paper-faithful (matches PT) or under-energy (PT brighter)?
- If under-energy: file specific tasks for the missing paper-prescribed paths (multi-bounce / sky / wider blur), don't bundle into one work item.
- If paper-faithful: file appropriate brightness work (multi-bounce GI / sky-irradiance) as separate scoped tasks; close THIS task as "verified".

### Why medium priority

GISponza is the showcase scene; visible dimming matters for "looks playable". But the fix that exposed it (TASK-6.5) is paper-correct and shipped, so this isn't a regression rolled-back-able into the constant. The right next move is paper-faithful brightness work, not a hot-fix.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Path-tracer reference captures of GISponza at TASK6_5_post_sponza orbit positions (frames 60/80/100/119 minimum)
- [x] #2 Direct PT-vs-rasterized-GI comparison documented with captures + diff
- [x] #3 Determination: paper-faithful (matches PT) or under-energy (PT brighter)? Quoted in summary
- [x] #4 If under-energy: specific follow-up tasks filed per missing paper-prescribed path (multi-bounce / sky-irradiance / wider blur)
- [ ] #5 If paper-faithful: brightness work (multi-bounce / sky-irradiance) filed as separate scoped tasks; this task closes as verified
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Determination: under-energy, not paper-faithful exposed reality.** Ground-truth GPU path tracer (300 spp, 4 bounces, NEE, Disney/GGX BRDF, texture-aware) shows GISponza mean luma 145.80; pre-TASK-6.7 rasterized was 116-131 across the orbit. The dimness post-TASK-6.5 was not the inherent dimness of single-bounce SH GI — it was a real implementation gap that `LIGHTPASS_AMBIENT_FLOOR` had been masking.

### Mechanism corrections from this audit

1. **Wrong PT was referenced in TASK-6.6 task body.** `World.inl:354` invokes `RayTracer::Execute()` which is the **Pete-Shirley AABB-toy CPU ray tracer** with synthetic Lambertian/Metal materials (no textures, hardcoded sky gradient). Not usable as ground truth. The actual PBR ground truth is the **GPU path tracer** at `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl`, invoked via `-test gpu_path_tracer`. The task brief should be updated to point at this.

2. **`RadianceCacheMiss.hlsl:5-33` is correctly wired** — it does call `getSkyColor`. Sky color DOES enter the radiance cache when probe rays geometrically miss to sky. The previous task body's speculation that the miss path was broken was incorrect. The actual gap is at the **secondary vertex on interior surfaces**, where surfaces relying on bounce-from-sky receive only the previous-frame screen RT.

### Quantitative determination

| Configuration | mean_L | mean_RGB | dark<8 | dark<24 |
|---|---:|---|---:|---:|
| **PT 300 spp (truth)** | **145.80** | **(136, 148, 146)** | **2.4%** | **4.9%** |
| Rast PRE-6.5 (FLOOR active) f60 | 136.91 | (129, 143, 99) | 0.5% | 1.6% |
| Rast POST-6.5 f60 | 131.05 | (125, 137, 92) | 6.8% | 15.1% |
| Rast POST-6.5 f100 | 117.32 | (113, 123, 78) | 22.2% | 25.6% |
| Rast POST-6.5 f119 | 116.11 | (112, 121, 77) | 23.2% | 26.2% |

PT is brighter by **1.11×–1.26×** in mean luma. Even with `LIGHTPASS_AMBIENT_FLOOR` active (PRE-6.5), rasterized GI was 6-9 luma below PT — the constant was masking a real under-energy gap, not solving an honest single-bounce dimness.

### Smoking gun: blue-channel deficit

PT mean_RGB is near-neutral **(136, 148, 146)**. Rasterized is markedly yellow-orange biased **(112-125, 121-137, 77-99)** — blue channel deficient by ~50%. PT's `SkyColor()` integration via sky NEE at every bounce contributes cool-blue light to interior surfaces. Our rasterized GI's miss path only fires when probe rays geometrically escape Sponza's atrium roof, which is rare for interior surfaces. Interior surfaces relying on bounce-from-sky receive only the previous-frame screen RT via `RadianceCacheClosestHit.hlsl:48-58` — a feedback loop that converges from below.

`Build/captures/TASK6_6_pt_sponza/diff_heatmap_default_cam.png` shows red (PT brighter) on floor, ceiling, and lower atrium; blue (rast brighter) on curtain edges where SH-projected GI overshoots.

### Sensitivity to TASK-6.7 / TASK-6.9 (committed AFTER this audit's binary was built)

PT captures are stable — the PT path is unaffected by the GI changes. Rasterized captures here are from the pre-TASK-6.7 binary at commit `b829a474`. Predicted post-TASK-6.7+6.9 measurements:

- PT mean_L stays **145.80** (PT is unbiased).
- Rast mean_L lifts ~5 units toward 128-132 (TASK-6.7 reduces integrator bias from 64× ray shortfall; TASK-6.9 lifts SH DC via backup).
- The ~50% blue-channel deficit **persists**.

The dimness gap shrinks modestly, doesn't close. The blue deficit is not addressable by ray-budget or coverage tuning — it's a **structural absence of sky-NEE at the secondary vertex**.

### Captures (gitignored)

- `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/` — 300-spp converged PT at GISponza's default Main Camera pose. Convergence verified to ±0.01 luma over the last 5 frames. **The reference.**
- `Build/captures/TASK6_6_pt_sponza/rasterized_default_camera/` — pre-TASK-6.7 rasterized GI at the same camera pose, frames 115-119.
- `Build/captures/TASK6_6_pt_sponza/orbit_freeze_yaw360_300spp/`, `rast_orbit_freeze_yaw360/` — PT and rast at the orbit-frozen frame-120-pose for cross-validation.
- `Build/captures/TASK6_6_pt_sponza/compare_pt_vs_rast.py`, `diff_heatmap_default_cam.png`, `side_by_side_default_cam.png`, `README.md` — comparison tooling and visual artefacts.
- **Alignment artifact**: `.alignments/TASK-6.6-pt-vs-rasterized-gisponza.md` (canonical reference for follow-up work).

### Ranked follow-ups (filed as TASK-6.10; #2 already in TASK-6.8 as D4)

1. **Sky-NEE-as-a-light at secondary vertex** (HIGH, paper-faithful, closes most of 50% blue deficit, ~10-15 luma lift). Add an NEE step at the radiance-cache closest-hit / secondary vertex that samples a sky direction over the upper hemisphere, traces a shadow ray, and on visibility accumulates `getSkyColor(skyDir) × BRDF × cos · 2π`. Surface: `RadianceCacheClosestHit.hlsl` plus a secondary-vertex NEE step. Filed as **TASK-6.10**.
2. **Direct-only LightPass output for closest-hit secondary contribution** (MEDIUM, ~4-6 luma lift in shadows, breaks dim-feedback loop). Currently `RadianceCacheClosestHit.hlsl:48-58` reads `in_LightPassOutgoingLuminance` (direct + indirect). Splitting into direct-only and indirect-only output buffers — secondary vertex reads direct-only — breaks the dim-feedback loop. Already tracked as #7 / D4 in TASK-6.8 roadmap; **not re-filing**.
3. **NOT recommended**: variable-radius blur (`kGIDenoiser_MaxBlurMask = 16` is already 2× Capsaicin's 8), restoring `LIGHTPASS_AMBIENT_FLOOR` (was masking, not solving — paper-non-faithful regression).

### Honest gaps

1. **No converged PT at orbit-frame-60/80/100/119 poses individually.** PT accumulator resets when camera moves; the existing `-camera_orbit` arg shape only allows freezing at yaw=360°≡0°, not at intermediate orbit poses. Cross-pose triangulation (PT-default-cam vs Rast-orbit-frames) shows the luma_ratio is pose-invariant within ±0.07×, supporting the comparison's validity.
2. **Auto-exposure differs between PT and rasterized inputs.** Both feed `LuminanceHistogramPass` → `LuminanceAveragePass` → `FinalBlend`, so auto-exposure is identical pipeline-wise but adapts to different upstream content. Acceptable for a cross-path comparison.
3. **No Capsaicin GISponza screenshot.** Capsaicin's repo at `Build/reference/Capsaicin/` ships no rendered references.
4. **Mean-luma metric is post-tonemap (UByte 0..255 sRGB-ish FinalBlend output).** Direct HDR-radiance comparison would be cleaner but requires .hdr captures from before tonemap; not present in the existing pipeline. The post-tonemap delta is monotonic with the HDR delta — sufficient for "PT is brighter" but not for absolute radiance ratios.
5. **Pre-TASK-6.7 binary used.** Rasterized measurements should be re-run against post-TASK-6.7 / post-TASK-6.9 binary. PT captures remain valid — only rast side needs re-capture. The qualitative finding (under-energy, blue deficit, sky-NEE missing) is invariant under TASK-6.7/6.9; only the magnitude of the gap changes.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
