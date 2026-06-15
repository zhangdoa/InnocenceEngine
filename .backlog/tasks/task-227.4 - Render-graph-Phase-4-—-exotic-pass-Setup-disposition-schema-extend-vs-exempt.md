---
id: TASK-227.4
title: Render-graph Phase 4 — exotic pass Setup disposition (schema-extend vs exempt)
status: Done
assignee:
  - code-impl
created_date: '2026-05-31 12:54'
updated_date: '2026-06-15'
labels:
  - rendering
  - render-graph
  - refactor
dependencies:
  - TASK-227.3
parent_task_id: TASK-227
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Phase 4 of the TASK-227 render-graph umbrella. Per-pass decide, for each bin-(c) exotic pass, whether to extend the graph schema to absorb its Setup or leave it a permanent exemption (TASK-227 AC#4 — migrate OR explicitly exempt with reason). By Phase 3 these already dispatch via opaque-kernel nodes; this phase is about their *Setup* (P1), not ordering (P2).

Bin-(c) REALIZED inventory (per 2026-06-15 graph audit of `Data/ExampleProject/RenderGraph/ExampleRenderGraph.json`):
   - 24 `BypassEnabled=false` passes actively participating in the present chain (see TASK-227 AC#4 realized inventory).
   - 18 `BypassEnabled=true` passes (zero-fed, kept in graph to preserve upstream topology): TransparentBlend, MotionBlur, TransparentGeometryProcess, Animation, Billboard, Debug, BSDFTest, Volumetric{GeometryProcess,IrradianceInjection,RayMarching,Visualization}, PTHashGridCache{UpdateTiles,PurgeTiles,MipCascadeBuild}, PT, PTNRD{FormatConvert,Denoise,Composition}. Each is a PURE-JSON graph node; the imperative `*Pass.cpp` source is REMOVED FROM DISK.
   - Of the 18 bypassed: 5 are the doc-1 §9 bin-c cases (PTPass / NRDIntegrationAdapter / VolumetricPass / DebugPass / AnimationPass). Of those: **PTPass is bin-c; LightPass migrated to production; NRDIntegrationAdapter folded into PTNRD*; VolumetricPass = the 4 Volumetric* passes; DebugPass is the no-op debug overlay; AnimationPass has per-object dynamic bind per RFC §6.6**.
   - Per-pass disposition (formal):
      - **PTPass** — bypassed (PT chain archived). PROPOSED PERMANENT EXEMPTION (per RFC §6, opaque-kernel pattern handles dispatch but Setup stays imperative). Conditional binding count (19–26 via `if constexpr`), scene-load callbacks, material-buffer rebuild, TLAS poll — too entangled for the bin-c schema extension. Permanent exempt.
      - **LightPass** — **MIGRATED** (commit `b3351c7`/df40414a, 18 bindings) on the production shader. Latent SSRC source (the 19th binding the full shader has when SSRC is live) is the one carry-forward: schema-extension to mark bindings as "if-resource-present" would close it; the keep-set currently doesn't feed SSRC GI. Disposition: **schema-extension candidate** (deferrable until SSRC GI re-migration lands).
      - **PTNRD* cluster** (FormatConvert, Denoise, Composition) — bypassed (PT chain archived). PROPOSED PERMANENT EXEMPTION.
      - **Volumetric* cluster** (GeometryProcess, IrradianceInjection, RayMarching, Visualization) — bypassed (PT chain archived). PROPOSED PERMANENT EXEMPTION.
      - **PTHashGridCache* cluster** (UpdateTiles, PurgeTiles, MipCascadeBuild) — bypassed (PT chain archived, runtime-disabled under `ENABLED=false` conditional compile). PROPOSED PERMANENT EXEMPTION.
      - **DebugPass** — bypassed (no-op debug overlay, not in keep-set). PROPOSED PERMANENT EXEMPTION.
      - **AnimationPass** — bypassed (per-object dynamic bind; deferred per RFC §6.6). PROPOSED PERMANENT EXEMPTION.
      - **BillboardPass** — bypassed (DEAD; body commented out). EXEMPT.
      - **MotionBlurPass** — bypassed (DEAD; `PrepareCommandList` returns false). EXEMPT.
      - **TransparentBlendPass / TransparentGeometryProcessPass** — bypassed (PT chain archived; both depend on `TransparentBlend Input`). PROPOSED PERMANENT EXEMPTION.
      - **BSDFTestPass** — bypassed (test pass, never enabled in production). EXEMPT.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

2026-06-15 — Closed. The 18 bypassed passes' per-pass disposition is now filed in the description (PT cluster / Volumetric cluster / PTHashGridCache / Debug / Animation / Billboard / MotionBlur / Transparent / BSDFTest — all PROPOSED PERMANENT EXEMPTION with reason; LightPass migrated with the conditional-SSRC binding as the lone schema-extension candidate). All 18 imperative `*Pass.cpp` files REMOVED FROM DISK, so the migration is irreversible — the disposition table is the final record. Umbrella AC#4 references this disposition (see TASK-227 2026-06-15 realized inventory). No engine-side code change. Reopen if/when the SSRC GI re-migration lands and the conditional-SSRC binding in LightPass needs to be expressed as schema.
<!-- SECTION:NOTES:END -->


## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
