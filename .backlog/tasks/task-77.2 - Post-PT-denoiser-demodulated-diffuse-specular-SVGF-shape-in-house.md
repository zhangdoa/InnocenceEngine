---
id: TASK-77.2
title: 'Post-PT denoiser: demodulated diffuse/specular SVGF-shape (in-house, no NRD)'
status: To Do
assignee: []
created_date: '2026-05-07'
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
Add the **screen-space sample-reuse denoiser** missing from TASK-77.1. The hash-grid cache port (TASK-77.1, commits b6058cdc → 82c74743) is faithful to Capsaicin GI-1.0 but is architecturally a *long-tail-radiance feeder*, not a denoiser — every shipped primary-PT pipeline (Capsaicin, RTXPT, NRD-Sample, Lumen, GIBS) pairs the world cache with a screen-space stage where visible-motion reuse actually lives. TASK-77.1 stopped at the cache layer; this task fills the screen-space layer.

Concretely: temporal accumulation + spatial edge-aware filter on demodulated diffuse + specular radiance at the primary hit. The hash-grid cache from TASK-77.1 stays compile-time-OFF until TASK-77.3 (cache reactivation as long-tail feeder above this denoiser).

## Hard constraint — license

**No NVIDIA NRD library**. License incompatibility (project-side constraint, 2026-05-07 user direction). Architectural reference patterns from NRD-Sample are fair game (read the code; do NOT link the library or vendor any NVIDIA NRD source). Implementation is in-house, drawing on the open literature:

- SVGF (Schied et al. 2017) — temporal accumulation + edge-aware à-trous wavelet, variance-guided.
- A-SVGF (Schied et al. 2018) — adaptive history reuse.
- ReBLUR-shape patterns (hit-distance-driven blur radius, per-lobe diffuse/specular handling) — known from the SVGF lineage and ReBLUR talk slides; structurally implementable without NRD source.
- EA SEED Surfel-GI (SIGGRAPH 2021) — production-shipped post-PT denoise reference.

`git grep -i 'nrd\|nvidia.*nrd'` at any point during implementation must return references-list mentions only, not code.

## Architecture sketch

1. **Primary-hit signal split** (`GPUPathTracerRayGen.hlsl`): output diffuse + specular radiance separately, albedo-demodulated, with per-lobe hit distance.
2. **Temporal accumulation pass**: per-pixel previous-frame motion-vector reprojection, history-rejection on geometry/normal/depth disocclusion, variance estimate (mean^2 − mean of squares).
3. **Spatial filter pass**: edge-aware à-trous wavelet, variance-guided radius modulation, per-lobe. Hit-distance-modulated blur radius for specular.
4. **Composition**: re-modulate albedo, sum diffuse + specular, hand off to tonemap.

This builds atop the loop-per-bounce raygen already in place; no hash-grid coupling, no DXR-callback layer changes.

## Out of scope

- Hash-grid cache reactivation — TASK-77.3 (when filed).
- ReSTIR DI / GI / PT primary-hit reservoir reuse — TASK-77.3 candidate, not 77.2.
- Neural denoisers (NRC, OIDN, OptiX) — separate axis under TASK-77 umbrella.
- Motion-vector pipeline rework — current TAA motion vectors are reused as-is; if they prove insufficient, file separately.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 (AC-1, visual, blocking) Noise-floor reduction visible on motion across UnitTest + GITestBox + GISponza. User-direction layer-4 sign-off on at least one moving-camera capture per scene.
- [ ] #2 (AC-2, visual, blocking) No temporal artifacts (boil, ghost, lag). Static-camera convergence at least matches denoiser-OFF baseline.
- [ ] #3 (AC-3, supporting only) Per-pixel temporal stddev reduction on settled frames vs denoiser-OFF baseline. Cannot close on its own.
- [ ] #4 License audit: `git grep -i 'nrd\|nvidia.*nrd'` returns references-list mentions only, no code.
- [ ] #5 Bypass invariant: with denoiser disabled (compile-time toggle), output is bit-identical to current PT-only HEAD. Verified at every commit on the implementation chain.
- [ ] #6 Engine builds clean (RelWithDebInfo); GBV clean on smoke run.
- [ ] #7 Peer review by a fresh impl-stage agent (cross-stage if HLSL+C++ span warrants).
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

## Cross-references

- TASK-77 — parent umbrella (RD: PT denoise → promote to primary).
- TASK-77.1 — closed 2026-05-07 with "wrong-framing accepted" verdict; cache implementation correct + paper-port faithful, but cache architecture is feeder, not denoiser. Toggle stays compile-time-OFF.
- `.alignments/TASK-77.1-rework-paper-port-audit.md` — Capsaicin port audit (cache only).

## References to add to .claude/references.json on first HLSL author

- SVGF (Schied 2017): `https://cg.ivd.kit.edu/publications/2017/svgf/svgf_preprint.pdf`
- EA SEED Surfel-GI: `https://advances.realtimerendering.com/s2021/SIGGRAPH%20Advances%202021%20-%20Surfel%20GI.pdf`
- NRD-Sample (architectural reference only): `https://github.com/NVIDIA-RTX/NRD-Sample`
