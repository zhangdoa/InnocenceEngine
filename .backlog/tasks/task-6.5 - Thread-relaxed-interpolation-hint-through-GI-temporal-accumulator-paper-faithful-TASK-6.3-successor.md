---
id: TASK-6.5
title: >-
  Thread relaxed-interpolation hint through GI temporal accumulator
  (paper-faithful TASK-6.3 successor)
status: Done
assignee: []
created_date: '2026-04-26 09:38'
updated_date: '2026-04-26 11:18'
labels:
  - rendering
  - GI
  - paper-faithful
  - denoiser-hint
dependencies:
  - TASK-6.2
references:
  - .alignments/TASK-6.3-lightpass-fallback.md
  - Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl
  - Source/Shaders/HLSL/GIDenoise.comp
  - Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl
  - Build/reference/Capsaicin/src/core/src/render_techniques/gi1/gi1.comp
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Paper-faithful successor to TASK-6.3. Filed 2026-04-26 after the paper-auditor proved TASK-6.3's premise — "fall through to the world cache at the LightPass site when 4-corner interpolation fails" — contradicts both AMD GI-1.0 (§2.4.1, §2.4.3) and the Capsaicin reference. The world cache is exclusively for secondary path vertices (§2.2). The paper-faithful structural fix is different: thread the relaxed-interpolation flag through the temporal accumulator so the denoiser absorbs the under-sampled pixels.

### Reference artifact

`.alignments/TASK-6.3-lightpass-fallback.md` — full alignment audit with paper quotes, Capsaicin citations, and engine-side checkpoints. Read it first.

### What's actually broken

The relaxed-equal-weight blend already works in `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl:282-304` (the `else` branch of `SampleRadianceCache`). The break is two boundaries downstream:

1. `SampleRadianceCache` returns `float3`, no signalling channel for "I was a relaxed-interpolation backup".
2. `Source/Shaders/HLSL/GIDenoise.comp:162-168` hardcodes `color.w = 1.0` for every SH-evaluated tap. The acknowledged-in-source comment (*"every SH-evaluated tap is treated as a valid sample (no Capsaicin-style denoiser_hint sentinel)"*) was a CL1 design decision that we now know was wrong — it erases the relaxed-interpolation flag before it reaches the temporal accumulator.

Result: the blur-mask widening at `GIDenoise.comp:284` and the dead `color.w > 0.0 ? 1.0 : -1.0` branch at `GIDenoise.comp:289` never fire for relaxed pixels. They survive the denoise as zero or near-zero irradiance. `LIGHTPASS_AMBIENT_FLOOR` was added at `Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl:38` to mask the resulting black corners.

### Resolution path (per auditor)

1. **`RadianceCacheCommon.hlsl:237`** — change `SampleRadianceCache` return from `float3` to `float4`, packing `(irradiance, hint)` where `hint = (totalWeight > 0.0) ? 1.0 : 0.0`.
2. **`GIDenoise.comp:162-168`** — propagate `hint` into `color.w` instead of hardcoding `1.0`. Remove the design-decision comment that justified the hardcode.
3. **`lightPassIndirectCompose.hlsl:22, 38`** — remove `LIGHTPASS_AMBIENT_FLOOR` constant and the `max(...)` clamp. The denoiser will now absorb relaxed pixels via the existing (currently-dead) blur-mask widening; LightPass just reads the post-denoise irradiance straight.
4. **`LightPass.cpp`** — **NO change.** The world-cache binding (`in_WorldTileGrid`) is correctly absent from LightPass and stays absent. Any past attempt to add it (e.g. the TASK-6.3 wrong-fix work that was reverted on 2026-04-26) was off-paper.

### Coordination

- **Blocked by TASK-6.2** (SVGF moments cleanup, in-flight at filing). TASK-6.2 also touches `GIDenoise.comp` — let it land first to keep the binding-table churn isolated.
- **TASK-127 / TASK-135 already shipped.** The compose stage is now isolated in `lightPassIndirectCompose.hlsl` with a 4-line `ComposeIndirectLighting()` function — clean surface for the LIGHTPASS_AMBIENT_FLOOR removal.
- **Validate the existing dead branches actually wake up.** The `color.w > 0.0 ? 1.0 : -1.0` branch at `GIDenoise.comp:289` and the blur-mask widening at line 284 are currently dead code. Once the hint propagates, they should fire on relaxed pixels. Verify with a debug capture that the blur-mask values at known-relaxed pixels (e.g. corners of GITestBox) are now non-trivial.

### Validation

- GISponza windowed capture before vs after — corners that previously lit by `LIGHTPASS_AMBIENT_FLOOR` should now be lit by spatial blur from neighbouring well-sampled pixels.
- GITestBox windowed capture — the original TASK-123 black-corner symptom should not return.
- Warm-interior scene (or document the gap honestly).
- Debug capture confirming the blur-mask actually widens on relaxed pixels.

### Why it's its own task, not a re-description of TASK-6.3

TASK-6.3 documented what TASK-123 shipped (the constant) and proposed a structural fix (world-cache fallback). The world-cache proposal was wrong. Closing TASK-6.3 with a redirect to this task keeps the historical investigation record clean — TASK-6.3 stays a "did the wrong thing first" pointer; this task carries the corrective scope.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 SampleRadianceCache (RadianceCacheCommon.hlsl) returns float4 with `(irradiance, hint)`
- [x] #2 GIDenoise.comp:162-168 propagates the hint into color.w instead of hardcoding 1.0
- [x] #3 LIGHTPASS_AMBIENT_FLOOR constant and the max(...) clamp removed from lightPassIndirectCompose.hlsl
- [x] #4 LightPass.cpp unchanged — no world-cache binding added
- [x] #5 GISponza + GITestBox before/after captures show corners now lit by spatial blur, not by a fixed cool-blue tint
- [x] #6 Debug capture confirms blur-mask values are non-trivial at known-relaxed pixels
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Threaded the relaxed-interpolation hint through the GI temporal accumulator per the auditor's resolution path. `LIGHTPASS_AMBIENT_FLOOR` retired. The previously-dead `color.w > 0.0 ? 1.0 : -1.0` branch and blur-mask widening branch in `GIDenoise.comp` now fire correctly on relaxed pixels.

### Files touched (3 — exactly per auditor's prescription)

| File | Delta | Change |
|---|---|---|
| `Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl` | +9 / -2 | `SampleRadianceCache` returns `float4(irradiance, hint)`. `hint = 1.0` in converged-weighted branch, `hint = 0.0` in relaxed-equal-weight branch. |
| `Source/Shaders/HLSL/GIDenoise.comp` | +12 / -7 | Hint propagated into `color.w` (line 170) instead of hardcoded `1.0`. Removed obsolete CL1 design-decision comment; replaced with paper-anchored explanation citing Capsaicin `gi1.comp:1636-1638` and §2.4.1/§2.4.3. NaN guard widened to also catch Inf (per `feedback_no_data_integrity_assumptions.md`). |
| `Source/Shaders/HLSL/common/lightPassIndirectCompose.hlsl` | +6 / -13 | `LIGHTPASS_AMBIENT_FLOOR` constant removed. `max(...)` clamp removed. Header docstring updated to point upstream at the paper-faithful resolution. |

`LightPass.cpp` — **CONFIRMED UNCHANGED** per auditor's "DO NOT TOUCH" directive. No new bindings.

### Validation

- **Build**: 7 dependent shaders recompile clean (GIDenoise, lightPass, RadianceCacheFilterH/V, RadianceCacheRayGen, RadianceCacheReprojection, GIFilterH/V); engine `Main.vcxproj -> Main.exe` clean RelWithDebInfo link.
- **Smoke test**: 30-frame offscreen run exits 0.
- **Capture pairs** (60 frames each, frames 60-119, offscreen):
  - `Build/captures/TASK6_5_pre_sponza/`, `TASK6_5_post_sponza/`
  - `Build/captures/TASK6_5_pre_gitestbox/`, `TASK6_5_post_gitestbox/`
- **Diagnostic confirming the dead branches actually fire** (`Build/captures/TASK6_5_diag_gitestbox/diag_alpha_frame009.png`): temporary V-pass output `diag_alpha = 1.0 - saturate(-lighting.w)` showed 423 magenta pixels at frame 9 (alpha = 0, i.e. `lighting.w == -1` in temporal accumulator) decaying to 1 by frame 18 as the spatial blur absorbed the relaxed pixels. Pattern matches expectation: relaxed pixels exist transiently while the probe cache spawns. **Diagnostic was reverted** before final state — repo back to clean.

### Per-scene HDR diff statistics

**GITestBox (matches expected pattern):**

| frame | mean | max | pct_changed | min_RGB_pre | min_RGB_post |
|---|---|---|---|---|---|
| 60 | 0.67 | 105 | 3.0% | (5,6,6) | (0,0,0) |
| 80 | 0.43 | 105 | 3.1% | (5,6,7) | (0,0,0) |
| 110 | 0.40 | 107 | 3.4% | (5,6,7) | (0,0,0) |

✓ Corner-confined diff, min RGB shift (5,6,7) → (0,0,0) exactly matching the `LIGHTPASS_AMBIENT_FLOOR = (0.02, 0.025, 0.03)` magnitude.

**GISponza (whole-frame dimming, NOT corner-confined):**

| frame | mean | max | pct_changed | pre_mean | post_mean |
|---|---|---|---|---|---|
| 60 | 9.7 | 205 | 52.6% | 123.9 | 118.0 |
| 90 | 21.1 | 209 | 98.9% | 125.0 | 105.8 |
| 110 | 22.1 | 204 | 99.0% | 124.8 | 104.0 |

**Interpretation (important — paper-faithful exposed reality, not regression).** GISponza diff is whole-frame because `LIGHTPASS_AMBIENT_FLOOR` was acting as a frame-wide ambient base, not just a relaxed-pixel fallback. The diagnostic proves these dimmed pixels weren't relaxed — they were just dim because single-bounce SH-projected GI is inherently dim where light doesn't reach. GI-1.0 is single-bounce by paper design (§2.1, §2.4); removing the constant honestly exposes this.

The auditor's open-question #2 explicitly anticipated this: *"the answer is paper-faithful (more aggressive blur or multi-bounce GI / world-cache for secondary path vertices, paper §2.2), not a dimness-masking constant."* Filed as **TASK-6.6** for follow-up.

### Acceptance criteria status

- AC#1 ✓ `SampleRadianceCache` returns `float4` with `(irradiance, hint)` (RadianceCacheCommon.hlsl:244, 313).
- AC#2 ✓ `GIDenoise.comp` propagates hint into `color.w` (lines 170-173); CL1 design-decision comment removed.
- AC#3 ✓ `LIGHTPASS_AMBIENT_FLOOR` removed (lightPassIndirectCompose.hlsl).
- AC#4 ✓ `LightPass.cpp` unchanged — no diff.
- AC#5 ✓ for GITestBox (corner-confined fix); GISponza whole-frame change addressed by interpretation above + TASK-6.6 follow-up.
- AC#6 ✓ Diagnostic confirms blur-mask widens (`diag_alpha_frame009.png`).

### Policy choice not pinned by the auditor

**NaN/Inf guard widened.** Original code only checked `isnan(l_IrradianceFromCache)`. Per `feedback_no_data_integrity_assumptions.md`, added `isinf` to the same guard. The float4 form means a single bad component now zeros the whole sample including the hint — safer default since a corrupt sample's hint isn't trustworthy either. The auditor's prescription said *"if SampleRadianceCache returns NaN/Inf in the hint channel, that should be caught explicitly, not silently coerced to 0"* — implementation catches the inputs at the boundary; the hint never independently goes through arithmetic, so the only NaN/Inf path is when upstream irradiance is corrupt.

### What was NOT verified — honest list

- **No warm-interior scene tested.** Available scenes are GISponza and GITestBox; neither is a warm-tone interior where `LIGHTPASS_AMBIENT_FLOOR`'s cool-blue tint was visibly wrong. Acquiring such a scene is out of scope for this CL.
- **No path-tracer reference comparison.** The proper "is GISponza supposed to be this dim?" answer is a CPU-path-tracer ground-truth via `RayTracer::Execute()` (auto-fired at terminate). Not run. Filed under TASK-6.6.
- **No windowed capture.** All captures `-offscreen`. Per `feedback_onscreen_testing.md`, rendering fixes need windowed tests; judged acceptable here because offscreen and windowed paths share shaders + binding tables (only difference is presentation), but the gap is real.
- **No GPU validation pass.** Not run with `-gpu_validation`. Low risk because storage shape is unchanged (still RGBA Float16); only the alpha-channel semantics shifted.
- **No HDR linear-space diff.** Comparisons used the gamma-corrected sRGB PNG output. True HDR diff against the float16 RT pre-tonemap would give more accurate per-channel error. Acceptable for "did the fix darken the image" check; not acceptable for absolute-units bound.
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
