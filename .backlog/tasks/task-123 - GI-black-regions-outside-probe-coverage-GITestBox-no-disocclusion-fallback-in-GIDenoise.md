---
id: TASK-123
title: 'GI black regions outside probe coverage (GITestBox) — no disocclusion fallback in GIDenoise'
status: To Do
assignee: []
created_date: '2026-04-23 20:00'
labels:
  - rendering
  - GI
  - radiance-cache
dependencies: []
references:
  - Source/Shaders/HLSL/GIDenoise.comp
  - Source/Shaders/HLSL/lightPass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Entire regions at the bottom and upper-right of the GITestBox render output are hard black. Path-tracer reference shows the same regions receiving indirect light (wall-to-wall bleed). In the rasterizer path the contribution is dropping to zero, not just low intensity.

Most likely cause: pixels in those regions have all 4 bilinear corners fail the ring-search + edge-aware weight test inside `SampleRadianceCache` (`GIDenoise.comp`). The relaxed-interpolation fallback path runs when `totalWeight > 0` fails, but it still uses probes whose irradiance may be zero or whose mask is INVALID — `LoadIrradiance` on a never-spawned probe tile returns whatever the SH atlas holds there (likely zero).

Reproduction: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 100` with `World.inl` swapped to GITestBox. See `Build/captures/TASK121_gitestbox_postring.png` — bottom and upper-right corner: pure RGB(0,0,0). Path-tracer `TASK6_gitestbox_pt_200f.png`: same regions show light bleed.

Investigations to run:
- Is the relaxed fallback actually reading from sky / never-initialised probe tiles that legitimately have 0 irradiance?
- Is the ring-search (`PROBE_SEARCH_MAX_RING=2`) too narrow for these edge regions — they're > 2 probe-tiles from any valid probe?
- Is the G-buffer alpha=0 early-out being taken for these pixels (which would leave the zero-init UAV clear), masking a different problem entirely?

TASK-121 fixed banding on flat walls that had probe coverage. Black regions are the complementary failure: pixels where coverage is zero in any reachable ring.

Candidate fixes:
- Wider ring search (but cost scales quadratically).
- Fall back to world-cache read (`in_WorldTileGrid`) when all 4 quad corners exhaust their rings — we already have that path in ClosestHit.
- Accept "sky" in the output and let the direct-lighting path drive instead — only kicks in if the black pixels also have zero direct lighting, which they may well.
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
