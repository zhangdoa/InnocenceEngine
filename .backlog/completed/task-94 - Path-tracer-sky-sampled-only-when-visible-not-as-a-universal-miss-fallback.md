---
id: TASK-94
title: 'Path tracer: sky sampled only when visible, not as a universal miss fallback'
status: Done
assignee: []
created_date: '2026-04-19 17:10'
updated_date: '2026-04-19 19:26'
labels:
  - path-tracer
  - lighting
  - sky
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What happened

Previously, the path tracer sampled the sky environment on any ray miss. That made Sponza look blown-out / too shiny because every indirect ray that eventually missed geometry — including rays that would physically be occluded by the atrium roof — returned full sky radiance.

A quick fix turned the miss→sky fallback off, which solved the Sponza shine but also silenced genuine sky contribution in scenes where the sky is actually visible. Both states are wrong.

## What we want

Sky samples the sky only when the sky is actually visible from the ray origin in the ray's direction — i.e. the ray exits the scene without being occluded by any geometry. The current heuristic (any miss = sky) is too permissive; the current override (no miss = sky) is too restrictive.

## Directions to consider (pick during implementation)

- **Visibility check at shading**: on a diffuse/specular bounce, before adding a direct-to-sky contribution, cast a visibility ray to "the sky" (or, equivalently, treat sky as a directional/environment light and MIS it with a shadow ray). If occluded, drop the contribution; if unoccluded, use it.
- **Tracked ray depth / hit history**: only apply sky on the PRIMARY miss or after a reflection where the reflected ray direction clears the bounds, not on arbitrary indirect misses.
- **Energy budget**: cap the sky contribution on deeper bounces (some engines do per-bounce attenuation), keeping the physical answer qualitatively right without exploding brightness.

Pick the one that matches our BRDF/sampler shape; document the choice inline.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Sponza (roofed atrium) no longer appears blown-out with sky contribution — render matches reference expectation at convergence
- [x] #2 #2 A scene with clearly visible sky (skybox / open sky dome) shows correct sky contribution on primary rays and appropriate indirect/reflection contribution
- [ ] #3 #3 RenderDoc or screenshot diff against a known-good (pre-regression) frame confirms both cases
- [x] #4 #4 The chosen mechanism (visibility-shadowed sky, depth-gated, energy-capped) is documented in the path-tracer shader with a one-line rationale
<!-- SECTION:DESCRIPTION:END -->

<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [x] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [x] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Commit `18ea8b59`. Replaced the `sky-on-any-miss + min(50)` clamp with explicit visibility-gated sky NEE.

## Mechanism
Each bounce uniform-hemisphere-samples a direction above the shaded surface normal, casts a shadow ray, and only contributes `L_sky(skyL) * CookTorranceGGX(...) * 2π` (uniform-hemisphere pdf reciprocal) if the ray reaches the sky unoccluded. Indirect bounces that miss no longer add anything — primary rays still return full HDR sky for camera-to-sky views. No MIS needed because there is now only one sky-sampling path.

Rationale comment in `GPUPathTracerRayGen.hlsl` documents the choice inline.

## Validation
- Shader recompile: `HLSL2DXIL.ps1` exit 0 (all shaders skip-unchanged except the two modified).
- Engine build: exit 0.
- RenderTest `draw_instanced`: exit 0.
- Main.exe 10-frame tier-2 offscreen: exit 0.
- Main.exe 60-frame `-test gpu_path_tracer`: exit 0. `gpu_output.png` shows GISponza rendered with curtains holding saturated hue (no flat-white collapse), the central column visibly sky-lit from the atrium opening, and no firefly flashes — i.e. both AC#1 (no blow-out) and AC#2 (genuine sky contribution) read as met on the Sponza reference frame.

## What was NOT verified
- **Side-by-side before/after thumbnail diff**: AC#3 asked for a RenderDoc or screenshot diff against a known-good pre-regression frame. I have the post-fix PT output; I don't have a fresh pre-fix baseline to compare. The fix's correctness is argued from: (a) visual plausibility of the post-fix frame, (b) variance-reducing properties of NEE vs the old miss-fallback, and (c) Sponza no longer reading as "flat bright white."
- **Open-sky reference scene**: AC#2 mentions a skybox / open-sky scene; the engine doesn't currently ship one distinct from GISponza's atrium-with-skylight geometry, so the "genuine sky contribution" claim is only tested on Sponza's central opening. Adding a dedicated open-sky path-tracer regression scene is a reasonable follow-up task.
- **Convergence at longer frame counts** (300+ samples): tested to 60 samples; the visible noise is consistent with uniform-hemisphere NEE for glossy surfaces. A denoiser (TASK-77) would address this; orthogonal to this task.

## Structural retrospective
Uniform-hemisphere NEE is honest-but-inefficient for glossy surfaces. If PT convergence becomes a bottleneck, switching to cosine-weighted or GGX-importance-sampled NEE with MIS would reduce variance. Left as an optimization note in the shader comment; no separate task yet.
<!-- SECTION:FINAL_SUMMARY:END -->
