---
id: TASK-6.11
title: >-
  Direct-only LightPass output buffer for radiance-cache secondary-vertex bounce
  (TASK-6.8 #7/D4 successor)
status: To Do
assignee: []
created_date: '2026-04-26 16:31'
labels:
  - rendering
  - GI
  - paper-faithful
  - feedback-loop
dependencies:
  - TASK-6.10
references:
  - .alignments/post-TASK-6.5-noise-gap.md
  - .alignments/TASK-6.6-pt-vs-rasterized-gisponza.md
  - .alignments/TASK-6.10-sky-nee-secondary-vertex.md
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/lightPassCommon.hlsl
  - Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl
parent_task_id: TASK-6
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
D4 from `.alignments/post-TASK-6.5-noise-gap.md` (noise-floor audit), originally tracked as #7 in TASK-6.8 roadmap. Promoted to its own task because it's the natural sibling to TASK-6.10 — together they approach PT truth on GISponza static-pose.

### Headline

`Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl:48-58` reads `in_LightPassOutgoingLuminance` (the previous-frame screen RT, which is `direct + indirect`) at the secondary vertex. This creates a **converge-from-below feedback loop**: GI bounce reads from a buffer that's already been GI-darkened, which keeps the steady state below true convergence.

PT's equivalent path uses **direct-light evaluation only** at secondary vertices (sun + point + sphere NEE), with multi-bounce indirect coming from the explicit second-bounce ray rather than from a feedback buffer. The fix: split LightPass output into two RTs (or one RT with a separate direct-only output) and feed the direct-only buffer into the secondary-vertex bounce.

### What to add

1. **`Source/ExampleProject/RenderingClient/LightPass.cpp`** — add a second render-target binding for "direct-only luminance". Allocate the texture, register it in the pass, expose via a `GetDirectOnlyLuminance()` accessor mirroring `GetLuminanceResult()`.
2. **`Source/Shaders/HLSL/lightPass.comp`** — write direct lighting (sun + point + sphere) into the new RT separately from the existing `out_lightPassRT0` (which keeps `direct + indirect` for FinalBlend's tonemap input). The `EvaluateSunLighting` + `EvaluateTiledPointLighting` calls (per the post-TASK-135 split in `lightPassCommon.hlsl`) already produce the direct contribution as a separable variable — this is mostly a write-side plumbing change.
3. **`Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl`** — swap `in_LightPassOutgoingLuminance` for the new direct-only SRV at the closest-hit's on-screen feedback read site (current line 48). Sky NEE (TASK-6.10) keeps its additive contribution on top.
4. **Wire the new SRV** through whatever pipeline-binding plumbing the radiance-cache pass uses (likely `RadianceCacheRaytracingPass.cpp`).

### Important caveat — point/sphere shadow maps still missing

`TASK-66` (point/sphere light shadows in rasterizer) is still open. The "direct" RT this CL produces will contain unshadowed point/sphere over-brightness. That over-brightness then feeds into the GI bounce. This is not a regression caused by this CL — the over-brightness is in the current `direct + indirect` buffer too — but it means D4 alone won't fully close the static-pose blue-ratio target (TASK-6.10 missed it at 0.670× vs target 0.85×). Closing that target requires both D4 AND TASK-66.

**Brief constraint for the implementer**: do NOT attempt to add point/sphere shadowing as part of this CL. Scope strictly to the LightPass output split + the feedback-loop swap. Per `feedback_anchor_invariants_in_dispatch.md`.

### Validation

- Build green (engine link required — new RT allocation and binding).
- Engine smoke: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- GBV pass with `-gpu_validation -total_frames 10` clean (the new RT binding is the most likely failure surface).
- Capture pair vs `Build/captures/TASK6_10_post_sponza/`:
  - 60-frame orbit at `-camera_orbit 20,8,120`, dump frames 60-119
  - Save to `Build/captures/TASK6_11_post_sponza/`
  - HDR diff stats (consec-frame noise + per-frame mean RGB)
- Direct PT comparison at the default Main Camera pose vs `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/`:
  - Quote mean_RGB and mean_L
  - **Predicted partial close**: warm channels (R, G) should lift; mean_L moves further toward PT 145.80; blue may also lift slightly (sky-NEE energy now propagates one bounce further without dim-feedback attenuation). Static-pose blue-ratio target 0.85× still likely unreached without TASK-66.
- Check FinalBlend output is unchanged (it still consumes `direct + indirect` from RT0; the new direct-only RT is GI-bounce-feedback only).
- **No regression** on consec-frame noise (≤39.20 from TASK-6.10).

### Why high priority

Sibling to TASK-6.10. Compounds predictably. Closes the dominant remaining structural gap in the GI pipeline (after this and TASK-66, the rasterized GI should be PT-comparable on Sponza without confounders).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 LightPass.cpp adds a direct-only output RT + GetDirectOnlyLuminance() accessor
- [ ] #2 lightPass.comp writes direct (sun+point+sphere) into the new RT separately from RT0 (direct+indirect)
- [ ] #3 RadianceCacheClosestHit.hlsl swaps in_LightPassOutgoingLuminance for the new direct-only SRV at the on-screen feedback read site
- [ ] #4 FinalBlend output unchanged (RT0 still feeds tonemap)
- [ ] #5 Build green; engine smoke exit 0; GBV pass clean
- [ ] #6 Orbit capture pair (60 frames) vs TASK6_10_post_sponza; HDR diff + per-frame mean RGB stats quoted
- [ ] #7 Default-camera PT comparison: warm channels lift toward PT, mean_L moves closer to 145.80; document any remaining gap (blue-ratio target 0.85× may still be unreached pending TASK-66)
- [ ] #8 No noise regression on consec-frame mean luma-delta (≤39.20 from TASK-6.10)
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
