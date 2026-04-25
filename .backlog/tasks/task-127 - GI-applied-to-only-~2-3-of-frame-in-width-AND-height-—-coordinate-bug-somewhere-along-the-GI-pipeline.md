---
id: TASK-127
title: >-
  GI applied to only ~2/3 of frame in width AND height — coordinate bug
  somewhere along the GI pipeline
status: Done
assignee: []
created_date: '2026-04-25 00:00'
updated_date: '2026-04-25 22:30'
labels:
  - rendering
  - GI
  - coordinates
  - regression-archeology
dependencies: []
references:
  - Source/Shaders/HLSL/GIDenoise.comp
  - Build/captures/TASK125_CL1/gpu_output_0119.png
  - Build/captures/TASK125_CL1/static_119.png
  - Build/captures/TASK125_CL1/windowed_59.png
parent_task_id: TASK-6
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Surfaced during user review of TASK-125 CL1 captures (`Build/captures/TASK125_CL1/gpu_output_0119.png` plus the windowed and static captures alongside it). The user observed:

> "the top left has the GI applied, like only 2/3 of the image in both height and width, feels like coordinates issue somewhere along the pipeline."

### This is pre-existing, not a CL1 regression

This defect predates TASK-125 CL1. It is visible in CL1 captures because CL1 made the underlying scene structure visible (by removing the spiral that previously occluded the whole frame in baseline TASK124_orbit captures). The CL1 changes (color-delta + dynamic history cap port from Capsaicin) do not touch screen-space coordinates, viewport plumbing, or the probe-grid extents — they only adjust the temporal accumulation blend rate and history cap.

Filing this as a separate task so it is not conflated with CL1 work and so TASK-125's closure is not blocked on diagnosis here. It IS, however, a blocker for declaring the GI port (TASK-6 umbrella) "done" — a 2/3 × 2/3 coverage region is visibly wrong on every scene.

### Symptom

The upper-left ~2/3 × 2/3 of the frame has GI bounce contribution; the remaining right strip and bottom strip do not. The cutoff appears as a fairly clean rectangular boundary, which strongly suggests a coordinate / extent mismatch somewhere along the chain — not a content-dependent failure (e.g. occlusion, missing probes, disocclusion fallback).

### Reference captures

- `Build/captures/TASK125_CL1/gpu_output_0119.png` — orbit-mode offscreen capture, frame 119.
- `Build/captures/TASK125_CL1/static_119.png` — static-camera offscreen capture, same frame index.
- `Build/captures/TASK125_CL1/windowed_59.png` — windowed Main.exe capture for the same scene.

All three show the same upper-left coverage region.

### Likely candidates to investigate (not picked — leave open for the implementer)

- **Viewport size mismatch between GI dispatch and final compose.** GI may be dispatched against the full viewport but consumed at a different rect (e.g. compose stage uses a half-resolution or letterboxed sub-rect, or vice versa).
- **UV / screen-space coord normalization off by a factor.** E.g. `viewport.xy` (offset) used where `viewport.zw` (extent) was intended; or a half-resolution dispatch read at full resolution without the corresponding UV scale.
- **Tile-grid coord rounding in `SampleRadianceCache`.** `Source/Shaders/HLSL/GIDenoise.comp` uses `probeUV = float2(screenCoord) / float2(TILE_SIZE, TILE_SIZE) - 0.5`. If `viewportSize` is not a multiple of `TILE_SIZE`, the right/bottom edges may fall outside the probe grid and clamp to a stale or zero entry.
- **Probe-grid clamp dimensions.** `maxProbeIndex = uint2(g_Frame.viewportSize.xy) / TILE_SIZE - 1` — verify this matches the actual probe-mask grid dimensions allocated by the radiance-cache passes. An off-by-one or a viewport-vs-render-target discrepancy here would crop the visible probe-coverage region exactly the way the symptom suggests.
- **LightPass / compose stage reading the GI texture with a wrong sampler/extent.** A clamp-vs-wrap sampler on a sub-extent texture, or an SRV created with a mismatched width/height, would also reproduce a clean rectangular cutoff.

### Repro

The defect is visible in the captures listed above without needing to re-run anything. To reproduce on demand:

```
RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen \
    -total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120
```

Compare `Bin/gpu_output_0119.png` against the reference. Windowed reproduction: launch Main.exe normally on GISponza and visually inspect — the right ~1/3 strip and bottom ~1/3 strip will be missing GI contribution.

### Why this is medium priority

Cosmetic-but-visible, not a crash or data-loss bug. Does block declaring TASK-125 / TASK-6 done since the GI image is visibly cropped on every scene. Not introduced by current in-flight work, so unblocking is not urgent for any other landing CL.
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Root cause was C++/HLSL constant drift, not a coordinate-pipeline bug as initially hypothesized. `RadianceCacheIntegrationPass.h` declared `SH_TILE_SIZE = 2` (stale, from when SH had 4 coefficients in 2×2). HLSL canonical `RayTracingTypes.hlsl:6` declares `SH_TILE_SIZE = 3` (correct per AMD GI-1.0 paper §2.4.2 — 9 SH coefficients in 3×3 layout). C++ side allocated the SH atlas at `probeGridW × 2`; HLSL side wrote/read at `probeIndex × 3`. At any resolution, probes past `floor(atlas_width / 3) = 2/3 × probeGridW` wrote/read out-of-bounds (UAV OOB returns zero), producing the upper-left ~2/3 × ~2/3 GI coverage symptom.

### Investigation shape

Investigated in parallel by paper-auditor (Capsaicin / GI-1.0 invariant audit) + rendering-researcher (engine-side trace). The auditor's invariants (probe-grid round-up, dispatch ceiling division, integer-coord addressing) ruled OUT the obvious paper-side suspects — those would cause ≤7px edge loss, not 1/3-frame loss. The actual culprit was an implementation-detail constant the auditor's invariants didn't cover (atlas layout isn't paper-prescribed). Researcher's engine-side trace through allocation vs read/write extents found the `2 vs 3` mismatch.

### Fix

- New `Source/ExampleProject/RenderingClient/RadianceCacheConstants.h` — single C++ namespace `Inno::RadianceCache` with `TILE_SIZE`, `SH_TILE_SIZE`, `UPSCALE_X/Y`, `SPAWN_TILE_SIZE_X/Y`, plus `TileCount(extent)` helper. Comments anchor each constant to its HLSL canonical file.
- `RadianceCacheIntegrationPass.cpp` — atlas allocation now uses `RadianceCache::SH_TILE_SIZE` (= 3). Local helper variables `l_probeGridWidth/Height` make the multiplication intent obvious.
- All five GI/RadianceCache pass `*.cpp` files now include `RadianceCacheConstants.h` and use `RadianceCache::TileCount(...)` for ceiling-divided dispatch extents.
- All six pass headers (four RadianceCache passes + `GIDenoisePass` + `RadianceCacheIntegrationPass`) had per-class duplicated `TILE_SIZE = 8` / `SH_TILE_SIZE = 2` / `SPAWN_TILE_SIZE_*` constants removed.
- `GIDenoisePass.cpp:296` dispatch line — was `uint32_t(viewportSize.x / 8.0f)` (magic literal + floor division). Now `RadianceCache::TileCount(...)` (named constant + ceiling). Comment explicitly calls out the cropping hazard.

### Validation

- Build: `cmake --build Build --config RelWithDebInfo --target Main` and `--target ExampleRenderingClient` — both green, Main.exe relinked, no new warnings.
- Behavior: orbit-mode offscreen capture, 60 frames at `-camera_orbit 20,8,120` (identical CLI to baseline TASK125_CL1), exit code 0.
- Captures (in `Build/captures/TASK127_fix/`):
  - `gpu_output_0119.png` vs `Build/captures/TASK125_CL1/gpu_output_0119.png` — bottom-right structural geometry now lit; baseline had a black strip there.
  - `gpu_output_0075.png` vs baseline — right ~1/3 strip drapes now visibly lit (turquoise/orange) instead of dimmed.
  - `gpu_output_0089_final_binary.png` — fresh capture against fully-rebuilt binary, full-frame GI coverage with no rectangular cutoff.
  - `windowed_59.png` — windowed Main.exe capture; full-frame illumination, no visible cropping.

### Follow-ups filed

- **TASK-6.4** (low) — paper-fidelity floor→ceil migration for `RadianceCacheCommon.hlsl` clamp + dispatch math engine-wide. Auditor identified these as real divergences from Capsaicin but invisible at 1920×1080 (axes are exact multiples of 8); manifests as ≤7px edge-strip loss at arbitrary windowed resolutions.
- **TASK-136** (medium) — systemic audit for the C++/HLSL constant-drift class across other graphics subsystems (shadow CSM, light culling, BRDF LUT, TAA history, etc.). The TASK-127 drift class is almost certainly not unique to GI.

### What was NOT verified

- No unit tests run (none exist that exercise the GI pipeline at this layer; engine-level tests are integration-only and the visual capture is the integration test).
- No RenderDoc capture (visual diff between baseline and fix in `Build/captures/` PNG pairs is the evidence).
- No runtime mismatch shape assertion. The single-source-of-truth header eliminates the per-pass drift class entirely; further runtime guard would require shader-side reflection (not available in this engine). If TASK-136 wants stronger runtime guards, the natural place is `RadianceCacheIntegrationPass::PrepareCommandList` checking that bound atlas dimensions equal `TileCount(viewport) × SH_TILE_SIZE` per axis.
<!-- SECTION:FINAL_SUMMARY:END -->
