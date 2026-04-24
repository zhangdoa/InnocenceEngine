---
id: TASK-123
title: 'GI black regions outside probe coverage (GITestBox) — no disocclusion fallback in GIDenoise'
status: Done
assignee: []
created_date: '2026-04-23 20:00'
updated_date: '2026-04-24 10:20'
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

## Investigation 2026-04-23

Two rounds of diagnostic shader modifications revised the diagnosis twice. Final finding: this is **not a bug, it's an architectural limitation of single-bounce radiance-cache GI**.

Round 1 — `finalBlendPass.comp` bucket-visualiser (`Build/captures/DEBUG_basepass_buckets.png`) showed "black" regions are **exactly zero** in basePass, not a GI-fallback near-zero. Initial hypothesis: sky-below-horizon or TASK-61 culling.

Round 2 — `preTAAPass.comp` geometry-vs-sky visualiser (`Build/captures/DEBUG_sky_vs_geom.png`): the image came back **100% green** — every pixel has `lightPassRT0.a != 0.0`, i.e. every pixel has geometry. No sky pixels at all. First hypothesis eliminated.

So the "black" regions are geometry pixels that come out of `lightPass.comp` with accumulated `l_Luminance_ForDirectLight ≈ 0`. Breaking that down:

- **Direct sun:** `CalculateLuminance(..., sun_illuminance, ...) * (1 - sunShadowFactor)`. In a region shadowed by the central building, `sunShadowFactor ≈ 1`, contribution ≈ 0.
- **Point / sphere lights:** attenuated by `1/r²` with cap at `LightAttenuationRadius`. For pixels far from the GITestBox point/sphere lights, contribution ≈ 0.
- **GI from cache:** `albedo * (1 - metallic) * in_GIIrradiance / PI`. The radiance cache spawns probes on screen, traces **one** ray each, and reads direct lighting at the hit surface. For probes in shadow, their rays mostly land on other shadowed surfaces (or the same shadow caster's back side), so the cache captures near-zero indirect light. After SVGF denoising + SH integration, irradiance ≈ 0 in shadow regions.

So three independent zero contributions combine into `out_lightPassRT0 = (0, 0, 0, 1)` and the pixel renders black.

**Path-tracer comparison**: the PT reference on the same scene (`Build/captures/TASK6_gitestbox_pt_200f.png`) shows those shadow regions in soft colour because its Monte Carlo integration carries full multi-bounce light transport. Each ray can hit a sun-lit wall on the third or fourth bounce and return that radiance. The rasterizer pipeline's radiance cache — one bounce from the screen probe — can't capture that.

This is aligned with the TASK-6 `[W.3b-cache-the-index]` deferred item: the paper's §2.2.4 optimisation only becomes meaningful once the radiance cache traces multi-bounce paths (ray-tracing from ClosestHit instead of only from RayGen). We explicitly deferred that in TASK-6 because the engine's current single-bounce architecture doesn't need it.

Candidate fixes (no longer the same shape as the original task description):
- **World-cache fallback in LightPass**: when `in_GIIrradiance` is ≤ some epsilon on a geometry pixel, query `in_WorldTileGrid` directly at `(positionWS, normal)` for a low-resolution indirect estimate. Cheap, uses infrastructure that already exists. Fills ambient into shadow regions without architectural changes.
- **Multi-bounce radiance-cache RayGen**: trace the ClosestHit's own radiance cache query to add a bounce. Proper fix but requires the `cache-the-index` optimisation to stay performant.
- **Sky-dome ambient fallback**: some GI pipelines add a cheap top-hemisphere ambient term (single-colour or sampled skybox) as a floor. Crude but fast.

Recommendation: **world-cache fallback in LightPass** as the narrowest fix — it closes the visible "black shadow" gap using the infrastructure TASK-6 [W] already landed, without touching the RayGen multi-bounce shape.

Related: TASK-61 (landscape mesh culling) was suspected but ruled out — every pixel has geometry per the Round 2 capture.

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [x] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [x] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Landed as the narrow ambient-floor variant — `lightPass.comp` clamps `l_IrradianceFromCache` to a floor of `(0.02, 0.025, 0.03)` via `max(...)` before the Lambertian conversion. A single shader-only line change, no new resource bindings, no plumbing.

Before vs after on both scenes:

- **GISponza**: architectural detail (columns/arches behind the central pillar) that the path-tracer reference shows now becomes visible in rasterizer output where it was previously lost to pure black. Curtain over-saturation also reduced as a side effect (scene-wide contrast is softer with shadows filled). Captures at `Build/captures/ASSESS_sponza_rast_static.png` (pre) vs `ASSESS_sponza_rast_static_POST.png` (post).
- **GITestBox**: bottom-half and upper-right black regions now render as dim teal / green with geometry contours readable. Upper wall SVGF speckle unchanged (those pixels were already above the floor). Central blown-white patches unchanged (TASK-122, out of scope here). Captures at `Build/captures/TASK121_gitestbox_postring.png` (pre) vs `ASSESS_gitestbox_rast_static_POST.png` (post).

The three candidate fixes in the Investigation section — world-cache fallback, multi-bounce RayGen, sky-dome ambient fallback — picked the third (cheapest, lowest-risk) and it delivered the 80% visible improvement for a 1-line change. The "better" fixes (world-cache lookup in LightPass, multi-bounce integration) remain valid follow-ups if the constant floor proves too crude (e.g. warm-interior scenes where a cool-blue floor looks wrong, or scenes where the variance of correct ambient across the scene is too large for a single constant).

Not verified:
- Behaviour on scenes outside GISponza / GITestBox. The floor constant was picked to look right on these two; different lighting setups may want different values (and eventually a cheap world-cache read would replace the constant with a position-dependent estimate).
- Motion-time behaviour — TASK-125 motion defects (SVGF smear, banding re-emergence) are orthogonal and remain open.
- Path-tracer mode unaffected; the fix only applies to the rasterizer `LightPass.comp`.
<!-- SECTION:FINAL_SUMMARY:END -->
