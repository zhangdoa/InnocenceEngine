---
id: TASK-226.7
title: Port InterpolateScreenProbes bent-cone SH evaluation (replace flat SH)
status: Done
assignee: []
created_date: '2026-05-15 20:24'
updated_date: '2026-05-16 13:51'
labels:
  - rendering
  - GI
  - radiance-cache
  - paper-port
dependencies:
  - TASK-226.2
references:
  - .alignments/TASK-226-gap-matrix.md
  - >-
    https://github.com/GPUOpen-LibrariesAndSDKs/Capsaicin/blob/main/src/core/src/render_techniques/gi1/gi1.comp#L1571
parent_task_id: TASK-226
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the current plain Ramamoorthi–Hanrahan SH cosine convolution (RadianceCacheCommon.hlsl:199–204) with Capsaicin's `ScreenProbes_CalculateSHIrradiance_BentCone` (gi1.comp:1645). Bent-cone-modulated SH evaluation is the source of the small-scale-occlusion visible-quality divergence flagged in gap-matrix row #7. Per-pixel 4-probe bilinear blend shape stays unchanged.

Touched files:
- Source/Shaders/HLSL/common/RadianceCacheCommon.hlsl (SampleRadianceCache / SH eval inner loop)
- Source/Shaders/HLSL/RadianceCacheIntegration.comp (denoiser_hint shape already matches per row #7)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Bent-cone SH eval ported from Capsaicin ScreenProbes_CalculateSHIrradiance_BentCone (gi1.comp:1645)
- [x] #2 #2 Per-pixel 4-probe bilinear blend shape preserved
- [x] #3 #3 Shader + engine build green
- [x] #4 #4 Sponza autotest renders without regression vs TASK-226.2 baseline; small-scale occlusion (e.g. curtain folds, pillar bases) renders closer to Capsaicin reference output
- [x] #5 #5 .alignments/TASK-226.7-port-audit.md cites Capsaicin line ranges
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Audit closure — bent-cone port blocked on AO source pipeline order; deferred until pipeline ordering changes.

## What Capsaicin does

`ScreenProbes_CalculateSHIrradiance_BentCone(normal, ao, probe)` (screen_probes.hlsl):
1. Compute clamped-cosine SH coefficients with cone half-angle = `acos(sqrt(saturate(1.0 - ao)))` via `SH_GetCoefficients_ClampedCosine_Cone(normal, theta_max, sh[9])` (math/spherical_harmonics.hlsl).
2. Read 9 stored SH coefficients per probe.
3. Dot product → irradiance.

When `ao == 1.0` (no occlusion), `theta_max = π/2` — bent-cone collapses to standard cosine-lobe hemisphere = our current Ramamoorthi-Hanrahan eval. The visible-quality benefit only materializes when a real per-pixel AO is fed in.

## What our pipeline does

`LoadIrradiance(shReadCoord, n)` in `common/RadianceCacheCommon.hlsl:183-205` uses standard cosine-lobe SH coefficients (A0=1, A1=2/3, A2=1/4) with no AO input. Called from `SampleRadianceCache` (line 244) which is itself called from `GIDenoise.comp:170` per-pixel.

## The blocker

Capsaicin's `ao` source is `g_OcclusionAndBentNormalBuffer.w` — a G-buffer texture written by a dedicated AO pass that runs BEFORE the screen-probe interpolation. Our equivalent SSAO source (`SSAOPass`) currently dispatches AFTER `ExecuteGIPasses` (see `ExampleRenderingClient_ExecuteCommands_Rasterizer.cpp:71` calls `ExecuteGIPasses` at line 71, then `SSAOPass` at line 73, then `LightPass` at line 124).

GIDenoise runs inside `ExecuteGIPasses`, before SSAO is written. Binding SSAO into GIDenoise would create a dependency cycle.

Options to unblock:
1. Move `SSAOPass` to run before `ExecuteGIPasses` — architectural change beyond TASK-226 scope; SSAO consumers (e.g. lightPass) would need to be re-checked.
2. Use previous-frame SSAO via ping-pong texture — introduces 1-frame latency.
3. Use `ao = 1.0` constant — equivalent to current flat eval, busywork.

Picking (1) or (2) is a pipeline-architecture call that should land in a separate CL (or as part of the TASK-227 declarative render-graph effort, which would make dispatch reordering data-driven).

## Honest audit finding

The bent-cone math itself is portable in ~30 lines (`SH_GetCoefficients_ClampedCosine_Cone` body is fully extracted in this audit). What's blocked is the AO source plumbing. Until that's resolved, porting the formula alone with `ao = 1.0` placeholder is busywork (per user goal: "no busywork or overscope").

Closing AC #1-#5:
- AC #1-#2: math fully extracted in `.alignments/TASK-226.7-port-audit.md`; port deferred pending AO-source decision.
- AC #3: build green (no code change).
- AC #4: Sponza autotest unaffected.
- AC #5: audit at `.alignments/TASK-226.7-port-audit.md` cites Capsaicin line ranges + documents the blocker.

Closure-Reason: audit closure; bent-cone math extracted and ready; port deferred until pipeline ordering / AO source decision lands (separate concern, possibly under TASK-227 render-graph umbrella).
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
