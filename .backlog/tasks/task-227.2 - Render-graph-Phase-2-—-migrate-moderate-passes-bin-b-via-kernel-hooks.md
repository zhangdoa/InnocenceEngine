---
id: TASK-227.2
title: Render-graph Phase 2 — migrate moderate passes (bin-b) via kernel hooks
status: In Progress
assignee:
  - code-impl
created_date: '2026-05-31 12:53'
updated_date: '2026-06-01 17:57'
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

2026-06-01 — Increment 2 (NOT yet committed; built+verified, pending peer review). Built bin-b primitive #3 = DEFERRED-RT, expressed as DATA (RFC D2 size-expression). Mechanism chosen (not a fork — engine-idiomatic): a texture resource declares "Size": "screen"; RenderGraphService defers its creation (m_DeferredScreenTextures) and, in CreatePassNode, installs an m_RenderTargetsInitializationFunc on the WRITER node's RenderPassComponent + sets m_Resizable=true + m_UseOutputMerger=false. So BOTH initial creation AND resize flow through the engine's existing RenderPassResourceService::InitializeOutputMergerTargets / FrameManagementService::PostResize loop — the same path the imperative m_RenderTargetsInitializationFunc used. CreateScreenSizedTexture deletes+re-Adds+Initializes at current screen res. Also built a 2nd reusable primitive: ScreenTileKernel (DefaultKernel + ResolveDispatch = floor(viewport/8), the full-screen dispatch ~9 passes share; tile size mirrors HLSL numthreads(8,8,1)). New "Size" column round-trips through serializer; new "ScreenTile" kernel in ResolveKernel registry.

MIGRATED SkyPass (coexistence seam g_UseRenderGraph, imperative body verbatim under false). SkyPass chosen as SIMPLEST deferred-RT pass: compute queue, single CL, deferred RT, NO ping-pong, NO graphics-CL state-transition prepass, NO raytracing. (SSAOPass/PostTAAPass were REJECTED for this increment — both carry a graphics-CL TryToTransitState prepass, which the brief flags as a separate primitive/fork.) Gotcha fixed: RenderPassResourceService::Initialize is DEFERRED (m_DeferredQueue → InitializeComponents), so the RT-init-func has NOT run by SkyPass::Initialize — m_Result is resolved lazily in PrepareCommandList instead (consumer PreTAAPass reads GetResult() per-frame AFTER SkyPass prepares, so safe). 5 passes now graph-driven (BRDFLUT, BRDFLUTMS, LuminanceAverage, OpaqueCulling, SkyPass).

FILE-SPLIT (file-size gate, both grew >300): RenderGraphService.cpp → +RenderGraphService_Resources.cpp (FindResource/ResolveImportedResource/CreateResource/CreateScreenSizedTexture). RenderGraphSerializerTests.cpp → +RenderGraphSerializerTests_ScreenSized.cpp (added to TestSuite CMakeLists explicit list). All 4 touched .cpp/.h now <300.

VERIFIED: BuildWin exit 0 (Main + TestSuite); TestSuite 5/5 RenderGraphSerializer incl new screen-sized round-trip, Failed:0; GISponza Main.exe -gpu_validation -total_frames 120 exit 0, 0 [Error] lines, graph loads 9 resources/5 passes; TestGIScene run-to-run band 0.47–0.52 MAE (matches clean-baseline ~0.50; the 0.45 FAIL is pre-existing/accepted), D3D12 errors 0 both runs. NOT verified: actual window resize (offscreen smoke can't trigger PostResize — the resize PATH is wired+correct-by-construction but not exercised at runtime); RenderDoc visual capture (parity judged by MAE band only). GBV barrier-layout 'false positive (non-fatal)' warnings appear engine-wide (LightPass/LightGrid/Sky/etc.), pre-existing class, exit still 0.

NEXT INCREMENT FORK (ping-pong / graphics-prepass): the remaining deferred-RT passes (SSAO, PostTAA, PreTAA, FinalBlend, SSRC filter/spatial/integration) ALL carry a graphics-CL state-transition prepass (CommandListBegin(Graphics)+TryToTransitState+CommandListEnd before the compute body) because the compute queue can't transition the result RT ReadOnly→WriteOnly. That prepass is a distinct primitive (a 'graphics state-transition prepass' kernel hook or node attribute). PING-PONG (TAAPass history, SSRC Even/Odd) is a further separate primitive. Both deferred to the next increment as flagged.

2026-06-01 — Increment 2 committed ecf7aca9 (deferred-RT primitive: screen-relative size as data via the writer node's RT-init-func + engine PostResize path; ScreenTileKernel floor(viewport/tile); SkyPass migrated, lazy null-guarded m_Result; two 300-line splits). 5 passes now graph-driven. Build green, TestSuite 5/5, GISponza smoke exit 0 (9 resources/5 passes), TestGIScene MAE in baseline band. Runtime window-resize NOT exercised (correct-by-construction: init-func re-reads resolution each call).

REVIEW CAVEAT: the fresh code-review agent was cut off by the account session limit (no findings returned). This increment had MAIN-SESSION SELF-REVIEW only (verified m_Result null-safety, deferred-RT resize re-read, ScreenTileKernel floor==imperative; fixed a stale 'ceil' comment). A fresh independent peer-review pass on ecf7aca9 is STILL PENDING — run it when the limit resets.

Comment-discipline: stripped tracker/RFC/phase/bin refs from all render-graph comments (commit d8773d74) and added a content check to the comment-essay-cap gate (commit 168ff700, harness) so they cannot recur — the old gate only capped run length, not content.

NEXT (the framed fork, needs decision): every remaining deferred-RT pass (SSAO/PreTAA/PostTAA/FinalBlend/SSRC filter/spatial/integration) carries a GRAPHICS-CL state-transition prepass (compute queue can't transition the result RT ReadOnly->WriteOnly) — a distinct primitive (graphics state-transition hook/attribute). PING-PONG (TAAPass history, SSRC Even/Odd) is a further separate primitive (may obsolete TASK-128). Both belong to the next increment.
<!-- SECTION:NOTES:END -->
