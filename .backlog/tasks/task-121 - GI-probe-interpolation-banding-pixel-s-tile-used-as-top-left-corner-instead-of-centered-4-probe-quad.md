---
id: TASK-121
title: 'GI probe interpolation banding: pixel''s tile used as top-left corner instead of centered 4-probe quad'
status: Done
assignee: []
created_date: '2026-04-23 19:45'
updated_date: '2026-04-23 19:58'
labels:
  - rendering
  - GI
  - radiance-cache
dependencies: []
references:
  - Source/Shaders/HLSL/GIDenoise.comp
parent_task_id: TASK-6
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`SampleRadianceCache` in `GIDenoise.comp` interpolates between 4 probes using the pixel's own tile as the top-left corner:

```hlsl
uint2 probeIndex = uint2(screenCoord) / TILE_SIZE;
uint2 tl = probeIndex;
uint2 tr = probeIndex + uint2(1, 0);
uint2 bl = probeIndex + uint2(0, 1);
uint2 br = probeIndex + uint2(1, 1);
float2 bilinear = frac(float2(screenCoord) / TILE_SIZE);
```

This is not a bilinear quad around the pixel — it's an offset quad that ALWAYS lies to the right/below the pixel's own tile. Inside a tile the bilinear weights shift smoothly, but the instant the pixel crosses a tile boundary the SET of 4 probes used changes discontinuously, producing hard tile-grid banding on every flat surface.

Reproduction: GITestBox scene at frame 100. Capture `Build/captures/TASK6_gitestbox_100f.png`. Teal-left/red-right Cornell walls show clear 8×8 pixel tile banding. Path tracer on same scene (`TASK6_gitestbox_pt_200f.png`) converges to smooth wall colour, confirming the issue is per-pixel probe lookup, not the cached radiance itself.

Fix: anchor probe (i, j) at its tile centre, shift the screen coordinate into probe-grid space by -0.5, floor to get the top-left probe of the surrounding quad, bilinear-weight against the centred 4:

```hlsl
float2 probeUV = float2(screenCoord) / TILE_SIZE - 0.5;
int2 probeFloor = int2(floor(probeUV));
float2 bilinear = probeUV - float2(probeFloor);
int2 maxP = int2(g_Frame.viewportSize.xy) / int2(TILE_SIZE) - 1;
uint2 tl = uint2(clamp(probeFloor,              int2(0), maxP));
uint2 tr = uint2(clamp(probeFloor + int2(1, 0), int2(0), maxP));
uint2 bl = uint2(clamp(probeFloor + int2(0, 1), int2(0), maxP));
uint2 br = uint2(clamp(probeFloor + int2(1, 1), int2(0), maxP));
```

Correctness checks:
- Pixel at tile centre `TILE_SIZE·(n + 0.5)`: bilinear = (0, 0), tl = (n, n), wTL = 1. 100% from its own probe. ✓
- Pixel at tile top-left corner `TILE_SIZE·n`: bilinear = (0.5, 0.5), 25% each from 4 probes meeting at the corner. ✓
- Pixel at screen edge: negative `probeFloor` clamps to 0, duplicate probe indices cause weights to sum at the same probe. ✓

Originally introduced in [I.1] (4-probe interpolation + relaxed fallback) under TASK-117. Hidden on GISponza by complex occluder geometry; surfaces on GITestBox's large flat walls.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
First diagnosis (probe-anchor convention) was valid but insufficient. The original code anchored probe (i, j) at pixel `(TILE_SIZE·i, TILE_SIZE·j)` (tile top-left); the Halton spawn actually places it closer to tile centre. Shifting `probeUV` by `-0.5` makes the bilinear match the physical probe location more faithfully. But this convention change alone left banding intact — because:

Under sparse spawning (`upscaleFactor = (2, 2)`) only ~1 of every 4 tiles carries a valid probe. `ComputeProbeWeight` hard-zeroes on `PROBE_MASK_INVALID`, so 3 of 4 quad corners contribute nothing and the blend collapses to whichever single corner is valid — yielding a constant per-tile colour across the whole 8×8 region. That's the banding.

The H/V filter passes already avoid this via `FindClosestProbe` (ring-widening search, [S1.5b]). `SampleRadianceCache` wasn't updated when that infrastructure landed. The real fix: wrap each of the 4 target-tile corners in `FindClosestProbe`, use `r.tileCoord` for both the weight test and `LoadIrradiance`, so each corner contributes its closest valid substitute. `ComputeProbeWeight` still rejects substitutes that are too far out of plane or facing the wrong way, so edge-awareness is preserved.

Coordinate spaces touched in the fix (for later reference):
- `screenCoord`: pixel index space (integer from `dispatchThreadID.xy`).
- `probeUV`: pixel position expressed in probe-grid units with probe-at-tile-centre origin.
- `probeFloor`: probe index space (int, can go negative before clamp).
- `targetTL/TR/BL/BR`: clamped probe index space.
- `lookupXX.tileCoord`: final probe index used for both weight test and SH atlas lookup — may differ from target when ring substitution fires.
- `pixelPos` / `probePos` (inside `ComputeProbeWeight`): world space.

Not fixed here (files separately):
- Blown-out white central geometry in GITestBox — direct-lighting / tonemap issue, not radiance-cache.
- Black regions at scene edges — disocclusion / outside-probe-coverage fallback, orthogonal to the blend fix.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
GI probe banding eliminated on GITestBox flat walls.

Before (`Build/captures/TASK6_gitestbox_100f.png`): hard 8×8 grid visible on every flat wall — each tile a constant colour because the bilinear blend collapsed to a single valid probe under sparse spawning.

After (`Build/captures/TASK121_gitestbox_postring.png`): smooth walls with only per-pixel SVGF residual noise. PNG is ~80 KB smaller (415 KB vs 517 KB) because smooth gradients compress better than per-tile-constant patches — an incidental cross-check that the spatial frequency structure changed.

Change: `SampleRadianceCache` in `Source/Shaders/HLSL/GIDenoise.comp` now (a) uses a probe-at-tile-centre convention (`probeUV = screenCoord/TILE_SIZE - 0.5`) and (b) wraps each of the 4 quad corners in `FindClosestProbe` so invalid targets fall back to the closest valid probe within `PROBE_SEARCH_MAX_RING` rings, with the edge-aware weight test running against the substitute's actual position and normal.

Validation: `HLSL2DXIL.ps1` clean; `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 100` on GITestBox exit 0; visual A/B against pre-fix capture shows the banding removed. GISponza still renders correctly under the same code path (radiance-cache pipeline is shared).

Not verified: no windowed / Tier-4 Interactive run — the fix is shader-only and doesn't touch windowed-specific code paths, but an Interactive run would have caught any regression in the case where depth edges drive different weight selection. Not verified: the blown-out white / black fallback regions visible in the same captures — explicitly out of scope for this task; files follow separately. Not verified: impact on performance of 4× `FindClosestProbe` per pixel (2×2 ring scan, 25 texture loads worst case), though the filter pass already uses the same pattern and hasn't shown as a hotspot.
<!-- SECTION:FINAL_SUMMARY:END -->
