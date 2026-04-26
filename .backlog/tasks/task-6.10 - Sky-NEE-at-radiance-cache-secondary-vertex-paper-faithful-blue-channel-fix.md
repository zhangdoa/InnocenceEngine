---
id: TASK-6.10
title: Sky NEE at radiance-cache secondary vertex (paper-faithful blue-channel fix)
status: To Do
assignee: []
created_date: '2026-04-26 13:52'
labels:
  - rendering
  - GI
  - paper-faithful
  - sky-nee
  - brightness
dependencies:
  - TASK-6.6
references:
  - .alignments/TASK-6.6-pt-vs-rasterized-gisponza.md
  - Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl
  - Source/Shaders/HLSL/GPUPathTracerClosestHit.hlsl
  - Source/Shaders/HLSL/RadianceCacheMiss.hlsl
parent_task_id: TASK-6
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
TASK-6.6's #1-ranked follow-up. Paper-faithful brightness fix for the under-energy gap that TASK-6.6's PT comparison surfaced. Predicted to close most of the ~50% blue-channel deficit and lift mean luma by ~10-15 units toward the GPU PT ground truth (145.80).

### Headline from TASK-6.6

GPU path tracer at 300 spp with 4 bounces + NEE + Disney/GGX BRDF shows GISponza mean_RGB **(136, 148, 146)** — near-neutral. Post-TASK-6.7+6.9 rasterized is approximately **(112-125, 121-137, 77-99)** — ~50% blue-channel deficit. The cause: PT's `SkyColor()` integration via sky NEE at every bounce contributes cool-blue light to interior surfaces. Our pipeline has **no sky NEE at the secondary vertex** — interior surfaces relying on bounce-from-sky receive only the previous-frame screen RT via `RadianceCacheClosestHit.hlsl:48-58`, which is a feedback loop that converges from below.

The miss path itself (`RadianceCacheMiss.hlsl:5-33`) IS wired correctly — sky color enters the radiance cache when probe rays geometrically escape the atrium roof, but that's rare for interior surfaces.

Reference: `.alignments/TASK-6.6-pt-vs-rasterized-gisponza.md` (the auditor's full alignment artifact). Read before starting.

### What to add

At the radiance-cache closest-hit (`RadianceCacheClosestHit.hlsl`), after the existing closest-hit shading but before returning to the radiance-cache integration, add a **sky NEE step**:

1. Sample a direction over the upper hemisphere relative to the hit-point's surface normal (cosine-weighted hemisphere sampling is the natural choice — same shape Capsaicin uses for its sky-NEE step).
2. Trace a shadow ray from the hit point toward `(skyDir, MAX_HIT_DISTANCE)`.
3. On visibility (no occlusion hit): accumulate `getSkyColor(skyDir) × BRDF × cos(theta) × 2π / pdf` into the closest-hit's outgoing radiance.
4. The result combines with the existing direct-light path (sun, points, spheres) and the existing indirect contribution from `in_LightPassOutgoingLuminance` (or its direct-only successor — see TASK-6.8 #7 / D4 for that work).

Reference: `GPUPathTracerClosestHit.hlsl` already implements this for the GPU PT — copy the pattern. The PT does it inside its own bounce loop; we adapt it to the single-bounce closest-hit invocation.

### Constraints

- **Sample budget**: one sky-NEE shadow ray per closest-hit invocation (1 spp at the secondary vertex). The probe-level sample budget (now 16/probe/frame post-TASK-6.7) gives the temporal averaging needed.
- **PDF accounting**: cosine-weighted hemisphere has `pdf = cos(theta) / π`, so the NEE contribution simplifies to `getSkyColor × BRDF × π`. Cross-check against `GPUPathTracerClosestHit.hlsl`'s implementation — match its convention exactly.
- **Visibility test**: opaque-only shadow ray; no transmission accumulation. Use the engine's existing shadow-ray infrastructure.
- **Coordinate with TASK-6.8 #7 / D4 (direct-only LightPass output)**: the two fixes compound — sky NEE adds the missing blue, direct-only-feedback breaks the converge-from-below loop. They can land in either order; document the cross-reference in your closure note.

### Validation

- Build green (shader + engine if needed for new bindings).
- Engine smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- **Direct PT comparison**: capture rasterized GISponza at the default Main Camera pose (matching `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/`). Compare mean_RGB and mean_luma against the PT reference. Target: blue-channel ratio improves from ~0.55× → ≥0.85× of PT's blue value; mean_L lifts by ≥8 units.
- **Orbit capture**: 60-frame orbit at `-camera_orbit 20,8,120`, dump frames 60-119. Compare against `Build/captures/TASK6_9_post_sponza/`. The yellow cast (which TASK-6.9 already softened from 13.5:1 to 1.96:1 R/B) should soften further toward PT's ~0.97:1.
- **No regression in noise** on the consec-frame metric (mean luma-delta ≤ TASK-6.9's 43.01).
- **No regression on GITestBox** (corners shouldn't get worse; auditor predicts they still need #5/#6 to fully smooth).
- **GPU validation pass** if any new bindings are added (`-gpu_validation` switch).

### Why high priority

Sky-NEE is the dominant residual gap to PT ground truth (per TASK-6.6's quantitative breakdown). Closes a visibly wrong color cast that the prior CL chain (TASK-6.5/6.7/6.9) couldn't fix because it's a structural absence of an entire light-transport path, not a calibration tweak. The user's "looks playable" pressure on showcase scene GISponza routes here.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Sky NEE step added at RadianceCacheClosestHit.hlsl secondary vertex: cosine-weighted hemisphere sample + shadow ray + getSkyColor × BRDF × π accumulation
- [ ] #2 PDF and BRDF conventions match GPUPathTracerClosestHit.hlsl exactly
- [ ] #3 Build green; 30-frame engine smoke exit 0
- [ ] #4 Default-camera PT comparison: blue-channel ratio improves from ~0.55× → ≥0.85× of PT's blue; mean_L lifts ≥8 units toward 145.80 truth
- [ ] #5 Orbit capture pair vs TASK6_9_post_sponza shows blue lift toward PT-neutral; HDR diff stats quoted
- [ ] #6 No noise regression on consec-frame mean luma-delta (≤43.01 from TASK-6.9)
- [ ] #7 No regression on GITestBox
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
