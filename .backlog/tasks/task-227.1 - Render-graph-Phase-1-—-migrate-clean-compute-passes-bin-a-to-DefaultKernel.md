---
id: TASK-227.1
title: Render-graph Phase 1 — migrate clean compute passes (bin-a) to DefaultKernel
status: To Do
assignee: []
created_date: '2026-05-31 12:53'
updated_date: '2026-05-31 13:27'
labels:
  - rendering
  - render-graph
  - refactor
dependencies:
  - TASK-227
parent_task_id: TASK-227
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Phase 1 of the TASK-227 render-graph umbrella. Migrate the bin-(a) "clean" passes — fixed shader, static binding table, static dispatch, no deferred-RT/ping-pong — to declarative graph nodes using the `Default` kernel (zero per-pass C++ Setup). Depends on the Phase-0 POC (TASK-227) landing the RenderGraph module, serializer, compiler, RenderGraphService, IRenderGraphKernel + DefaultKernel.

Bin-(a) set (~19, from RFC doc-1 §9): BRDFLUTMSPass, BillboardPass, SkyPass, LuminanceAveragePass, MotionBlurPass, PostTAAPass, BSDFTestPass, TiledFrustumGenerationPass, PTHashGridCache{Purge,Update,MipCascade}Pass (conditional node inclusion for the `if constexpr (ENABLED)` gate), SSRCFilter{Horizontal,Vertical}Pass, SSRCSpatial{Horizontal,Vertical}Pass, SSRCIntegrationPass, TransparentBlendPass, OpaqueCullingPass (+ ComputeCullingPass base). (BRDFLUTPass itself is the Phase-0 POC.)

One-at-a-time per RFC D5: each pass migrated, built, runtime-smoked, visual-parity-checked before the next. May sub-divide into smaller batches if the diff grows unwieldy.

Design reference: backlog doc-1 (TASK-227 Render-Graph Design RFC).
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-31 — Phase-0 POC review (doc-1 design) surfaced two forward risks to handle when DefaultKernel scales beyond the single-binding BRDFLUTPass case:
- DefaultKernel.cpp binds resources by loop index `i`, NOT the declared `m_DescriptorIndex`. Byte-identical for contiguous 0..N layouts (BRDFLUT), but a pass whose bindings aren't declared in layout-index order will bind to the wrong slot. Fix: bind via `m_Bindings[i].m_DescriptorIndex` (or enforce/document declaration-order == layout-index).
- RenderGraphSerializer TextureDescFromJson serializes 9 fields; CPUAccessibility, BorderColor, UseSharedHandle, sampler filter/wrap modes are dropped (left at struct defaults). Fine for the Persistent/single-binding Phase-0 scope; any Phase-1 pass that sets those fields needs them added to the schema + serializer or it silently narrows.
<!-- SECTION:NOTES:END -->
