---
id: TASK-6.5
title: >-
  Thread relaxed-interpolation hint through GI temporal accumulator
  (paper-faithful TASK-6.3 successor)
status: To Do
assignee: []
created_date: '2026-04-26 09:38'
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
- [ ] #1 SampleRadianceCache (RadianceCacheCommon.hlsl) returns float4 with `(irradiance, hint)`
- [ ] #2 GIDenoise.comp:162-168 propagates the hint into color.w instead of hardcoding 1.0
- [ ] #3 LIGHTPASS_AMBIENT_FLOOR constant and the max(...) clamp removed from lightPassIndirectCompose.hlsl
- [ ] #4 LightPass.cpp unchanged — no world-cache binding added
- [ ] #5 GISponza + GITestBox before/after captures show corners now lit by spatial blur, not by a fixed cool-blue tint
- [ ] #6 Debug capture confirms blur-mask values are non-trivial at known-relaxed pixels
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
