---
id: TASK-6.11
title: >-
  Direct-only LightPass output buffer for radiance-cache secondary-vertex bounce
  (TASK-6.8 #7/D4 successor)
status: Done
assignee: []
created_date: '2026-04-26 16:31'
updated_date: '2026-04-26 16:43'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Closed wrong-premise.** The work this task asked for is already implemented in the codebase. The closest-hit secondary-vertex bounce already reads a Lambertian-only direct buffer, not `direct + indirect`. The audit (`.alignments/post-TASK-6.5-noise-gap.md` D4 entry) was wrong about which RT is bound at the closest-hit's `in_LightPassOutgoingLuminance` SRV.

### Verification chain (rendering-researcher dispatched 2026-04-26 traced end-to-end before editing)

| Layer | File:line | Object |
|---|---|---|
| Shader: `out_lightPassRT0` write | `lightPass.comp:148` | `float4(l_DirectLuminance, 1.0)` where `l_DirectLuminance += l_IndirectLuminance` at line 143 → **direct + indirect** (consumed by tonemap path only) |
| Shader: `out_lightPassRT1` write | `lightPass.comp:150` | `float4(l_IndirectSeedLuminance, 1.0)` → **Lambertian-only direct** (`albedo * Illuminance / PI`), no specular, no GI |
| Shader binding (cache pass): `in_LightPassOutgoingLuminance` | `RayTracingBindings.hlsl:24` | `Texture2D ... : register(t5)` |
| C++ slot 6 maps to t5 | `RadianceCacheRaytracingPass.cpp:94-99` | comment "t5 - light pass illuminance result" |
| C++ binds slot 6 | `RadianceCacheRaytracingPass.cpp:240` | `LightPass::Get().GetIlluminanceResult()` |
| `GetIlluminanceResult()` returns | `LightPass.cpp:339-342` | `m_IlluminanceResult` |
| `m_IlluminanceResult` is bound to RT1 | `LightPass.cpp:316` | slot 19 = `out_lightPassRT1` |

Conclusion: closest-hit reads Lambertian-only direct lighting at `RadianceCacheClosestHit.hlsl:167`. Not RT0. The misleading variable name `in_LightPassOutgoingLuminance` (sounds like "outgoing radiance" / final RT) was the trap.

### Confirmation in the shader's own design comment

`lightPass.comp:140-143`:

```
// GI composes into the visual RT only; the illuminance RT carries
// direct lighting only so next-frame ray hits do not re-accumulate
// already-accumulated indirect energy.
l_DirectLuminance += l_IndirectLuminance;
```

This comment is the design intent the task asked the implementer to add. It already exists.

### Why this matters

If the implementer had executed without verifying:
1. Would have added a duplicate `m_DirectOnlyLuminance` RT identical in content to `m_IlluminanceResult` — duplication of state, two RTs writing the same thing.
2. Would have added a duplicate SRV binding identical to slot 6 in `RadianceCacheRaytracingPass.cpp` — wasted descriptor slot.
3. Validation captures would show zero change because the swap is `RT_A → RT_B` where `RT_A == RT_B`. Either bait-and-switch on the user with "validation showed no change as predicted," or reverse-engineer a justification for measurement noise.

The implementer correctly stopped and reported per the brief's anchored constraint and per `feedback_no_data_integrity_assumptions.md` / `feedback_verify_source_before_chasing.md`.

### Documentation updates landed alongside this closure

- `.alignments/post-TASK-6.5-noise-gap.md` D4 entry annotated **WITHDRAWN** with correction note pointing here.
- `.alignments/TASK-6.10-sky-nee-secondary-vertex.md` got a **CORRECTION** section pointing here; the artifact's references to "feedback loop" and "TASK-6.8 #7/D4 sibling" are now flagged as wrong-premise.
- TASK-6.8 roadmap's #7 entry annotated WITHDRAWN.

### Reframing the residual gap (TASK-6.10 closure had a wrong attribution)

The TASK-6.10 closure attributed the static-pose blue-ratio miss (0.670× vs target 0.85×) partly to "D4 needing to land." That attribution was wrong. The full residual gap routes to:

1. **Material routing** at the secondary vertex (placeholder `SECONDARY_VERTEX_ALBEDO_FALLBACK = 0.5`; real Sponza limestone is ~(0.7, 0.65, 0.55)). This is the dominant remaining gap. Worth filing as its own task when prioritized.
2. **TASK-66** missing point/sphere shadow maps — PT-comparison confounder. Per recently-saved `feedback_pt_comparison_must_account_for_rast_omissions.md`.
3. **World-up sampling proxy** at the secondary vertex (no shading normal in RC pipeline). Documented in TASK-6.10's alignment artifact.
4. **World-cache write-side** doesn't pick up the sky-NEE term yet (one-frame propagation latency through screen-probe chain). Documented in TASK-6.10's closure.

### Optional micro-CL not executed

The implementer flagged that renaming `in_LightPassOutgoingLuminance` → `in_LightPassDirectLuminance` (or `…IlluminanceResult` to match C++) in `RayTracingBindings.hlsl:24` and `RadianceCacheClosestHit.hlsl:167` would prevent future audits from making the same misread. Single-purpose rename, ~5 lines. **Not in this CL.** File as a follow-up if desired; per `feedback_no_dismissing_tool_noise.md` it would be the principled fix to the source of the confusion.

### Files NOT modified (correct outcome)

- `Source/ExampleProject/RenderingClient/LightPass.cpp` — would have added duplicate RT.
- `Source/ExampleProject/RenderingClient/LightPass.h` — would have added duplicate accessor.
- `Source/Shaders/HLSL/lightPass.comp` — would have added duplicate write.
- `Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl` — current read is correct.
- `Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp` — would have added duplicate slot.
- `Source/Shaders/HLSL/RayTracingBindings.hlsl` — would have added duplicate SRV decl.
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
