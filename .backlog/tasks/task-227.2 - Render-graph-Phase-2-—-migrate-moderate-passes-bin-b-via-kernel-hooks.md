---
id: TASK-227.2
title: Render-graph Phase 2 — migrate moderate passes (bin-b) via kernel hooks
status: In Progress
assignee:
  - code-impl
created_date: '2026-05-31 12:53'
updated_date: '2026-06-01 13:06'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-06-01 — Activated early (ahead of strict dep order) because TASK-227.1 proved bin-a is exhausted at 2 passes: the bulk of the umbrella's LOC payoff sits behind bin-b kernel infra. Groundwork already landed under .1 (commit 74215eb3): Buffer-resource type + external-resource-import in the graph schema/loader/serializer + GPUBufferResourceService::Find.

CORE bin-b problem #1 = PER-FRAME DYNAMIC RESOURCE RESOLUTION. Nearly every clean compute pass binds the DOUBLE-BUFFERED PerFrameCBuffer at slot 0: PerFrameDataService::GetCurrentFrameBuffer() returns a different GPUBufferComponent each frame by frame-parity ('PerFrameCBuffer'/'PerFrameCBufferPrev'). Current import-by-name resolves ONCE at node creation -> would pin one buffer -> stale every other frame. Need per-frame re-resolution at RecordNode time (kernel-hook or a 'dynamic import' resource flag).

#2 = DYNAMIC DISPATCH: IRenderGraphKernel::ResolveDispatch override computing thread-group counts from resource dims / CPU state (OpaqueCullingPass ceil(modelCount/64), SSRC* from texture dims). #3 = deferred-RT (CreateRenderTargets hook) for SSRC/TAA/SSAO.

First verifiable bin-b migrations once #1+#2 land: LuminanceAveragePass (static Dispatch(1,1,1), owns 1 buffer + reads 2 external incl. PerFrameCBuffer) and OpaqueCullingPass (dynamic dispatch + post-dispatch UAV state-tracker side effect). Then SSRC cluster with deferred-RT.

ADVISORY (from .1 review A1): RenderGraphEnumStrings AccessibilityToString round-trips only {Immutable,ReadOnly,WriteOnly,ReadWrite}; drops m_CopySource/m_CopyDestination/m_CrossQueue. No current buffer sets them; extend the table when a pass needing copy/cross-queue accessibility is migrated.

2026-06-01 — Increment 1 landed (commit 8a28928f, peer-reviewed PASS). Built the 3 bin-b primitives' first two: (1) per-frame dynamic resolution (FindResource special-cases built-in 'PerFrameCBuffer' -> GetCurrentFrameBuffer(), re-fetched each RecordNode), (2) dynamic dispatch via ComputeCullingKernel (ResolveDispatch ceil(modelCount/64) + empty-model early-out + post-dispatch UAV state-tracker). Deferred-RT (#3) still not built (SSRC/TAA/SSAO cluster). Migrated LuminanceAveragePass (Default kernel, buffer-owning + dynamic import) and OpaqueCullingPass (ComputeCulling kernel). 4 passes now graph-driven (BRDFLUT, BRDFLUTMS, LuminanceAverage, OpaqueCulling).

BUG FIXED this increment: CreatePassNode resolved m_PrimaryOutput eagerly at graph-load, logging a false 'imported resource not found' for OpaqueCulling's late-created imported output buffer; now resolves only graph-owned writes, imported resolve lazily at RecordNode. BUILD-FIX: RenderGraph/CMakeLists.txt file(GLOB CONFIGURE_DEPENDS) — a new .cpp in the globbed dir caused a link error because CMake didn't re-glob without a reconfigure; CONFIGURE_DEPENDS re-globs each build.

Verified: build exit 0; TestSuite 5/5 RenderGraphSerializer (incl. ComputeCulling round-trip) 0 failures; GISponza -gpu_validation -total_frames 120 exit 0, 0 D3D12 errors, graph loads 8 resources/4 passes; TestGIScene pixel MAE within clean-baseline band (no regression).

Review advisories (non-blocking, no change made): ComputeCullingKernel ResolveDispatch '>0?:1' guard + OutputBuffer dual-bound check are defensive dead code (harmless). NEXT: deferred-RT (CreateRenderTargets) primitive for the SSRC/TAA/SSAO cluster; then continue migrating compute passes that need only per-frame-import + dynamic-dispatch.

SEPARATE PRE-EXISTING ISSUE (surface, not chase; user aware/accepts — 'ignore the MAE'): GISponza TestGIScene MAE baseline ~0.50 exceeds the 0.45 threshold on clean ecs-overhaul HEAD, unrelated to TASK-227. Candidate for its own task if the threshold/reference is ever revisited.
<!-- SECTION:NOTES:END -->
