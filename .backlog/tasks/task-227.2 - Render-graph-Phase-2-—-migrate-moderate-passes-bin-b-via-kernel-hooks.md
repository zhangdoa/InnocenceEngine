---
id: TASK-227.2
title: Render-graph Phase 2 — migrate moderate passes (bin-b) via kernel hooks
status: To Do
assignee: []
created_date: '2026-05-31 12:53'
labels:
  - rendering
  - render-graph
  - refactor
dependencies:
  - TASK-227.1
parent_task_id: TASK-227
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Phase 2 of the TASK-227 render-graph umbrella. Migrate the bin-(b) "moderate" passes — those needing a small registered kernel overriding `ResolveDispatch` (dynamic dispatch from texture dims) and/or `CreateRenderTargets` (deferred RT, ping-pong). Ping-pong becomes a node attribute (may obsolete TASK-128). Depends on Phase 1 (TASK-227.1) proving the DefaultKernel path at scale and the kernel-hook interface being exercised.

Bin-(b) set (~16, from RFC doc-1 §9): SSRCReprojectionPass, SSRCRaytracingPass, SSRCTemporalPass, LuminanceHistogramPass, PreTAAPass, TAAPass, SSAOPass, LightCullingPass, TransparentGeometryProcessPass, AnimationPass, OpaquePass (IndirectDraw + CrossQueueExit::ToCommon + root constant), SunShadowRTPass (raytracing), FinalBlendPass, PTNRDFormatConvertPass, PTNRDDenoisePass, PTNRDCompositionPass.

Likely sub-divides: deferred-RT cluster, ping-pong cluster, raytracing cluster, NRD-convert cluster. Decompose when Phase 1 lands.

Design reference: backlog doc-1 (TASK-227 Render-Graph Design RFC), kernel interface in §6.
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
