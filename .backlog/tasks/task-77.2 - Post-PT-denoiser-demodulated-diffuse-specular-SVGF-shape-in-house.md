---
id: TASK-77.2
title: 'Post-PT denoiser: demodulated diffuse/specular SVGF-shape (in-house, no NRD)'
status: In Progress
assignee:
  - '@claude'
created_date: '2026-05-07'
updated_date: '2026-05-07'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## CL-1 design plan (2026-05-07)

### Architectural fork — Path A locked

Path B (reuse rasterized GBuffer) and Hybrid (reuse rasterized for "where am I", PT-write for denoiser-specific channels) are **broken-by-construction in the PT-primary path**: `OpaquePass` is gated under `if (!m_GPUPathTracerActive)` (`ExampleRenderingClient_PrepareCommands.cpp:82`, `_ExecuteCommands.cpp:144`), so `in_opaquePassRT0..3` carry stale or zero contents in PT mode. Reusing them would re-elevate the demoted rasterizer subsystem to load-bearing — exactly the failure mode `state/project-direction.md` warns against.

**Path A locked**: PT raygen writes per-primary-hit channels itself. Channel layout mirrors `OpaquePass.frag:124-129` so existing `DecodeGBuffer` / `lightPass.comp:161` decoders are reusable on the denoiser side without forking.

- RT0: positionWS (rgb) + mesh-id-or-skyflag (a)
- RT1: normalWS (rgb) + roughness (a)
- RT2: albedo (rgb) + metalness (a)
- RT3: motionVec.xy + hitDist (z) + reserved (w)

Motion vector at primary hit = project hit position with current `g_Frame.v_inv * g_Frame.p_original` and stored `prev_v * prev_p` (already maintained as `m_PrevViewMatrix` in `GPUPathTracerPass.h:91`); subtract in screen space.

### Per-pass design

**Signal split** (`GPUPathTracerRayGen.hlsl`, modified): at `bounce == 0` capture GBuffer-equivalent channels and split radiance into `radianceDiffuse` + `radianceSpecular`. Lobe assignment fires at the **primary-hit BSDF importance sample only** (existing `lobeSample < pDiffuse` branch ~line 629); the sampled lobe tags the path immutably for the rest of the path. NEE-at-primary-hit goes to diffuse (specular-NEE refinement is CL-5).

**Temporal accumulator** (CL-2 — `PTDenoiseTemporalPass`): compute shader, motion-vector reprojection, per-lobe history rejection (depth Δ + normal dot + mesh-id), variance estimate (Welford). Pattern ports from `GIDenoise.comp:184-260` swapping the rasterized motion-vector source for the PT-written one.

**Spatial à-trous** (CL-3 — `PTDenoiseAtrousPass`): single compute shader dispatched 5× with stride 1/2/4/8/16, ping-pong textures. SVGF edge-stopping weights (depth, normal, luminance vs √variance). Hit-distance modulates specular blur radius.

**Composition** (CL-4 — folded into `FinalBlendPass` or new `PTComposePass`): re-modulate diffuse by primary-hit albedo, sum diffuse + specular, write to a "denoised display radiance" texture replacing PT's accumulation as tonemap input. **PT accumulation buffer stays untouched** — toggle-OFF bit-identity holds.

### CL split

| CL | Scope | AC | Visible-progress shape |
|----|-------|----|------|
| **CL-1** | Raygen splits radiance into diffuse/specular + writes 4 GBuffer-equivalent UAVs at `bounce == 0`. New toggle `Inno::PTDenoise::ENABLED` defaults OFF. AccumBuffer composition unchanged. | AC-5 bypass (bit-identical), AC-6 builds clean. | RenderDoc-visible new UAV outputs at `bounce == 0`; no on-screen change. |
| **CL-2** | `PTDenoiseTemporalPass` — temporal accumulation + variance estimate, per-lobe history rejection. Composition still uses pre-temporal output when toggle ON. | Per-lobe history textures populated; bypass-OFF baseline matches HEAD. | 60-frame capture: history-on path shows reduced temporal noise on settled frames. |
| **CL-3** | `PTDenoiseAtrousPass` 5-iteration à-trous spatial filter, variance-guided. | AC-1 partial — visible noise-floor reduction in motion. | Side-by-side capture (toggle on vs off) on UnitTest. |
| **CL-4** | Composition pass — re-modulate albedo, sum lobes, swap into tonemap input. | All visual ACs land. | Three-scene moving-camera captures. |
| **CL-5 (optional)** | Specular-NEE-at-primary lobe assignment refinement. | AC-1 polish. | Glossy-floor test capture. |

### CL-1 risks / open questions (resolve in implementation)

- `m_PrevViewMatrix` is declared but design pass did not verify it's updated every frame. CL-1 implementer must check and wire if needed.
- PathTracerPayload may not carry instanceID; needs adding for the mesh-id history-rejection channel in CL-2. Cheapest: pack into RT0's `.w` channel.
- `tonemap` reads `GPUPathTracerPass::GetResult()` directly — CL-4 needs to substitute this; not a CL-1 concern but flagged for CL-4 entry.
- Two compile-time toggles co-exist post-CL-1: `PT_HASH_GRID_CACHE_ENABLED` (TASK-77.1, OFF) and `PT_DENOISE_ENABLED` (this task, OFF). Confirm `#if` nesting is orthogonal — denoiser writes happen at `bounce == 0`, never inside cache-write `bounce >= 1` block.

<!-- SECTION:NOTES:END -->
