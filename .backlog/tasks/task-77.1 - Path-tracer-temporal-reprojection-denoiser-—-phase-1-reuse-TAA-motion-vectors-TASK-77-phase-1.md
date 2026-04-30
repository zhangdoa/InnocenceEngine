---
id: TASK-77.1
title: >-
  Path-tracer temporal reprojection denoiser — phase 1, reuse TAA motion vectors
  (TASK-77 phase 1)
status: To Do
assignee: []
created_date: '2026-04-30 19:14'
updated_date: '2026-04-30 19:14'
labels:
  - R&D
  - path-tracer
  - rendering
  - denoiser
dependencies: []
parent_task_id: TASK-77
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why

TASK-77's "denoiser first" recommendation is the cheapest first axis with the highest typical impact. TASK-77's framing: *"Temporal accumulation reprojection (already have a TAA pass, reuse its motion vectors): reproject previous-frame's irradiance estimate to this frame via depth+velocity, then blend with current-frame noisy sample. Turns 1 spp/frame into something that looks like 30-60 spp after a second of camera stability."* + *"Start simple: temporal + small spatial → evaluate."*

Phase 1 is the smallest viable denoiser that produces a visible quality lift on the PT output: temporal reprojection only, no spatial filter. Spatial à-trous / SVGF is a separate sub-task, not bundled.

This is the cheapest first axis because:

1. The TAA pass already produces a motion-vector buffer for the rasterizer pipeline. Reuse first, derive PT-specific motion only if the TAA buffer doesn't fit.
2. Temporal accumulation is well-understood (every PT-real-time renderer ships some variant). No research delta.
3. One pass in, one pass out — no architecture-wide changes.
4. Visual A/B is direct: denoise off vs on, on the same camera path, with the noise floor obvious in stills.

Per TASK-77's own framing, this is the natural first phase to land before any of the other axes (spatial, ReSTIR, light BVH) come into scope.

## Owner

- **Design call**: `rendering-researcher` (the architectural-friction questions below need a design pass before either implementation lane starts).
- **Implementation lane 1**: `graphics-api-expert` (HLSL/compute — the reprojection compute pass itself, motion-vector sourcing, descriptor wiring, GPU-state contracts).
- **Implementation lane 2**: `rendering-researcher` (per-pass C++ — pass author, PrepareCommandList, RenderTargetsCreationFunc, integration with the existing PT pass and the composition site).

The two lanes can run in parallel after the design call resolves the friction surface.

## Architectural friction surface to scope at design time

These are not blocking the filing but must be answered at the design dispatch. Don't speculate ahead of the design call — these are the open questions a `rendering-researcher` design pass resolves.

### (a) Where does the noisy-sample buffer come out of the path tracer?

The current GPUPathTracer pass writes its accumulated output to a render target. The denoiser needs the **per-frame-noisy** sample, not the accumulated value (otherwise temporal reprojection compounds onto a value that already has temporal averaging baked in). The design call decides whether this is:

- A new per-frame-fresh output buffer alongside the existing accumulated buffer (cleanest; doubles PT output bandwidth).
- A toggle between "PT does its own accumulation" and "PT outputs raw 1-spp" (denoiser owns accumulation entirely).
- Some hybrid where the PT pass exposes both.

### (b) Whether the existing TAA pass's motion-vector buffer is GBuffer-derived (works for rasterizer) or whether the path tracer needs its own motion-vector path

TAA's motion vectors today come from rasterizing world-space velocity into a velocity G-buffer. For PT this works only for first-bounce surfaces visible to the camera (which is the majority of pixel-coverage in Sponza). For pixels visible only via reflection / refraction, TAA's motion vectors are wrong — the projected pixel didn't come from the surface they appear on.

Phase 1 scope likely accepts the rasterizer-style limitation (first-bounce-only motion vectors), with the understanding that mirror surfaces and glass will exhibit reprojection ghosting until a phase-N task replaces this with PT-side per-bounce motion. Design call confirms that scope.

### (c) Where does the denoised result composite into the final framebuffer alongside the rasterizer's path?

Today the rasterizer chain composites Opaque + GI + Lit + TAA → FinalBlend. The PT pass currently outputs to a separate target the user toggles between. The denoiser changes the topology: PT-noisy → reproject → denoised, and *that* is what the user sees in PT-mode. The design call decides:

- Where the reprojection pass lives in the dispatch order.
- Whether it ping-pongs internally or shares a ping-pong with another consumer.
- How TAA's existing depth+velocity history is exposed to it (read-only sibling bind, or a copy).
- What the composition site is — does PT-mode replace TAA in the rasterizer chain, or run alongside it for A/B inspection?

## Acceptance Criteria (draft — refined by design call)

These are seeds; the design call dispatches refines them. Don't over-specify before the design pass.

- [ ] #1 Noisy-sample buffer captured from GPUPathTracer (per-frame, not accumulated)
- [ ] #2 Motion-vector source decided (TAA reuse vs PT-derived) and documented in the task's Implementation Notes with the design rationale
- [ ] #3 Reprojection compute pass authored, builds clean, integrates into the dispatch order
- [ ] #4 Composition site decided and wired (PT-mode replaces TAA in the rasterizer chain, or runs alongside — design call's call)
- [ ] #5 Engine builds clean (RelWithDebInfo); GBV clean on smoke run
- [ ] #6 On-screen visual A/B: denoise off vs denoise on, same camera path, stills + short video — documented in Final Summary with a clear noise-floor delta
- [ ] #7 Peer review per `peer-review-required.md` — fresh-context reviewer of opposite role family (graphics-api-expert reviews if rendering-researcher implements; vice versa)

## Out of scope

These are TASK-77's other axes — separate sub-tasks, not bundled into phase 1:

- Spatial denoising (à-trous wavelet, bilateral, SVGF). Would be TASK-77.2 or similar.
- ReSTIR DI for direct lighting.
- Light BVH / alias table for many-light scenes.
- Multi-importance sampling balance heuristic between BRDF and light-sampling lobes.
- Stratified sub-pixel sampling (blue noise, Morton).
- Intel Open Image Denoise / NVIDIA OptiX Denoiser integration (the shipping-grade option — fundamentally different architecture).

Phase 1 deliberately scopes the smallest temporal step that produces a visible delta. Other axes get their own tasks once phase 1's outcome makes the next pick natural.

## Cross-ref

- **Parent**: TASK-77 (R&D umbrella — PT-primary direction approval 2026-04-30).
- **Trigger point for**: TASK-128 (ping-pong helper extraction). The denoiser's per-pixel ping-pong of (irradiance estimate, sample count) is the natural third concrete site that justifies extracting the helper alongside the existing GI pair and TAA's motion-vector ping-pong.
- **Re-evaluation trigger for**: TASK-108 (unified radiance cache). Once this task's outcome is known, the cache-as-denoiser axis is either rescoped into a PT-only world-space cache or archived.
- **Discipline anchors**: `regression-fix-flow.md` (visual A/B is the discipline for "did this make the image better"), `peer-review-required.md` (fresh-context reviewer between implementer and commit), `tech-choice-vs-default.md` (default = follow TASK-77's recommended cheapest first axis; SOTA = ReSTIR / OIDN; recent-project-precedent = TAA's existing reprojection pattern. Default-first picked because TASK-77 explicitly framed it as the cheapest first axis).
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



## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Noisy-sample buffer captured from GPUPathTracer (per-frame, not accumulated) — design call decides exact shape
- [ ] #2 Motion-vector source decided (TAA reuse vs PT-derived) and documented in Implementation Notes with the design rationale
- [ ] #3 Reprojection compute pass authored, builds clean, integrates into the dispatch order
- [ ] #4 Composition site decided and wired (PT-mode replaces TAA in the rasterizer chain, or runs alongside — design call's call)
- [ ] #5 Engine builds clean (RelWithDebInfo); GBV clean on smoke run
- [ ] #6 On-screen visual A/B: denoise off vs denoise on, same camera path, stills + short video — documented in Final Summary with a clear noise-floor delta
- [ ] #7 Peer review per peer-review-required.md — fresh-context reviewer of opposite role family
<!-- AC:END -->
