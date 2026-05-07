---
id: TASK-77
title: >-
  R&D: path tracer denoise + faster convergence → promote to primary rendering
  pipeline
status: To Do
assignee: []
created_date: '2026-04-18 19:34'
updated_date: '2026-04-30 19:12'
labels:
  - R&D
  - path-tracer
  - rendering
  - architecture
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Direction

Path tracer quality is now correct (TASK-69 back-face normals, TASK-67 sphere NEE, TASK-74 per-frame material refresh, TASK-77 indirect-sky firefly clamp). Real-time accumulation still needs seconds to converge on Sponza interior — fine for offline / preview stills, not yet fine for interactive framerates. Investigate what it takes to make the path tracer the **primary** rendering pipeline, retiring the rasterized GBuffer-forward chain into a debug / fallback path.

## Two axes, in order of typical impact

### 1. Denoiser
- **Temporal accumulation reprojection** (already have a TAA pass, reuse its motion vectors): reproject previous-frame's irradiance estimate to this frame via depth+velocity, then blend with current-frame noisy sample. Turns 1 spp/frame into something that looks like 30-60 spp after a second of camera stability.
- **Spatial edge-aware filter** (à-trous wavelet, bilateral, SVGF): one post-pass on the noisy sample before (or alongside) temporal accumulation. Intel/NVIDIA have OSS implementations that port cleanly to HLSL.
- **Intel Open Image Denoise / NVIDIA OptiX Denoiser** as the shipping-grade option — integrates as an external SDK.
- Start simple: temporal + small spatial → evaluate.

### 2. Faster convergence (fewer samples needed per frame)
- **ReSTIR DI** for direct lighting: spatiotemporal resampling makes NEE trivially convergent for many-light scenes. Biggest one-shot convergence win in the last few years.
- **Light BVH / alias table** for many-light scenes: even with the current flat light-list NEE, Sponza's small light count is fine, but becomes a bottleneck as soon as the scene gains > 100 lights.
- **Proper multi-importance sampling (balance heuristic)** between BRDF-sampling and light-sampling lobes — we sample per lobe today with single-lobe PDF, which biases variance.
- **Stratified sub-pixel sampling + path reuse** (Morton-jittered, or blue noise) instead of Halton-only.

## Why promote path tracer to primary

- Correct GI by construction (no radiance cache to debug, no probe-grid mismatch).
- Shared shading code with the rasterized OpaquePass already (BRDF, Fresnel, Disney diffuse) so material parity isn't an extra cost.
- Sponza already renders recognizably at 30-300 frames accumulated; with a denoiser the visible sample count per frame becomes 1 spp + temporal history, which is what the offline-lookalike renderers ship at real-time.
- Retires two codepaths for shadows / GI / reflection that currently both need maintenance (rasterizer cascaded shadows, radiance cache for GI, reflection probes). One pipeline = less drift.

## Non-goals

This task is **R&D direction**, not a concrete implementation step. Each axis above becomes its own implementation task once the direction is approved. Denoiser first makes the most sense to land earliest — it's independent of the other work.

## Exit criteria for the direction

- GISponza at 1080p renders visually clean (subjective) at ≤ 30 ms / frame on the Laptop GPU target, with the denoiser producing temporally stable output that the user can't distinguish from offline convergence after one second of camera stability.
- Rasterizer is marked "debug / comparison mode" in the config — only the path tracer is the default render path.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Direction approval (2026-04-30)

User approved the PT-primary direction this task framed. Strategic spine for downstream tasks:

> Under PT-primary direction (TASK-77 approval, 2026-04-30), rasterization-trick subsystems — shadowmap pipeline (cube atlas, point/sphere shadow techniques), SSGI / RadianceCache-as-rasterizer-feature, screen-space reflection tricks, light-volume rasterization, etc. — become fallback / debug-comparison only. Investment in their quality, structure, or extension stops paying for itself.

Umbrella moves from R&D-direction-only to actively-decomposed. Phase tasks get filed against the two axes the description identifies (denoiser first, faster-convergence second), each as its own implementation task.

**Phase 1 filed**: TASK-77.1 — Path-tracer temporal reprojection denoiser (phase 1, reuse TAA motion vectors). The cheapest first axis with the highest typical impact, per this task's own framing ("Start simple: temporal + small spatial → evaluate"). Design call by `rendering-researcher`; implementation lanes by `graphics-api-expert` (HLSL/compute) and `rendering-researcher` (per-pass C++).

**Other axes still scope-able, not yet filed**: spatial à-trous/SVGF, ReSTIR DI, light BVH, balance-heuristic MIS, blue-noise stratified sampling, OIDN/OptiX integration. File each when TASK-77.1's outcome makes the next axis the natural pick — don't pile on now.

**Closure condition for this umbrella**: closes only when the rasterizer demotion lands as the user-facing default (config marks rasterizer "debug / comparison mode" per the task's own exit criteria). Phase tasks closing individually don't close the umbrella; the demotion CL does.

**Cross-ref triggered by this approval**:
- TASK-153 (point-shadow validation) — closed obsolete in the same batch (no longer load-bearing).
- TASK-137 (oversize GI/RadianceCache .cpp split-before-grow) — archived in the same batch (growth signal evaporates).
- TASK-108 (unified radiance cache) — paused pending TASK-77.1 outcome (rescope-or-archive call after denoiser direction settles).
- TASK-128 (ping-pong helper extraction) — demoted, re-linked to TASK-77.1 as the natural trigger (third concrete ping-pong site).

## Phase progression update (2026-05-07)

**TASK-77.1 closed** with "wrong-framing accepted" verdict. Implementation correct + paper-port faithful (Capsaicin GI-1.0 hash-grid radiance cache, 5-CL D1-reversal chain b6058cdc → 82c74743). Architectural diagnosis: hash-grid cache is a *long-tail-radiance feeder*, not a denoiser — every shipped primary-PT pipeline pairs the world cache with a screen-space stage where visible-motion sample reuse lives. TASK-77.1's brief asked the cache to be the denoiser; the chain implementation could not overcome that architectural mismatch. Toggle stays compile-time-OFF; cache reactivates as feeder once a screen-space denoiser stage exists.

**TASK-77.2 filed**: in-house SVGF-shape post-PT denoiser. Demodulated diffuse + specular at primary hit, temporal accumulation + edge-aware spatial filter, no NRD library (license incompatibility — user direction 2026-05-07). Architectural reference: SVGF (Schied 2017), A-SVGF (Schied 2018), EA SEED Surfel-GI (SIGGRAPH 2021), NRD-Sample read-only (no link / vendor).

**TASK-77.3 candidate, NOT filed**: cache reactivation as long-tail feeder above the new denoiser; possible ReSTIR GI integration on primary hit. Per `backlog-workflow.md` § "Don't pile on" — file when 77.2 outcome makes 77.3 the natural pick.

**Other axes still scope-able**: light BVH, balance-heuristic MIS, blue-noise stratified sampling, OIDN integration. Same "don't pile on" rule.
<!-- SECTION:NOTES:END -->
