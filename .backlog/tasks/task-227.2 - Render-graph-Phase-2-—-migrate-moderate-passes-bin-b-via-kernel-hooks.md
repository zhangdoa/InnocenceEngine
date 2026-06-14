---
id: TASK-227.2
title: Render-graph Phase 2 — migrate moderate passes (bin-b) via kernel hooks
status: In Progress
assignee:
  - code-impl
created_date: '2026-05-31 12:53'
updated_date: '2026-06-11'
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
### CURRENT STATE (2026-06-13b) — at-a-glance anchor for a fresh session

Keep-set is **14 pure-JSON graph passes**, ZERO render-pass C++ classes. Graph: 20 resources /
14 passes. The present chain renders a recognizable **sun-SHADOWED Sponza** (well-exposed, full tonal
range): OpaquePass GBuffer → SunShadowRTPass → LightPass → PreTAA → TAA → PostTAA → FinalBlend.

Primitives PROVEN (data-driven, no per-pass C++): per-frame dynamic resolution (PerFrameCBuffer /
PerFrameCBufferPrev / TransformBuffer frame-parity), dynamic dispatch (Static / ScreenTile /
TiledTwoLevel / DrawModelGroups), deferred screen RT, ordered transition prepass, TrackWriteState
(culling→indirect), named init/update hooks, frame-parity ping-pong (TAA), raster / multi-RT + depth +
indirect-draw + root-constants (OpaquePass GBuffer, 0b507daf), and **raytracing** (DispatchRays + RT
PSO/shader-table from RayGen/AnyHit/ClosestHit/Miss/ShadowMiss + TLAS root-SRV bind via the "TLAS"
dynamic name + IsTLASReady self-guard) — SunShadowRTPass, commit 7f8b9164.

LightPass (lightPassSimple.comp) is still MINIMAL: sun (now shadowed by SunShadowRT_Visibility t5) + flat
ambient; NO radiance-cache GI, NO tiled point lights. Swapping it for the full lightPass.comp is gated on
SSRC GI (t10) + LightCulling + point-light TLAS shadows.

GOTCHA (cost a long misdiagnosis): TestGIScene runs Main.exe with CWD = Bin/ (Split-Path BinDir -Parent
in Test-Engine.psm1), so the REAL capture is **Bin/gpu_output.png**, NOT Bin/RelWithDebInfo/gpu_output.png.
A direct Main.exe run from Bin/RelWithDebInfo writes its capture THERE instead. Always view
Bin/gpu_output.png for the TestGIScene result. Auto-exposure is NOT broken.

Primitives STILL NEEDED (fidelity):
- **SSRC** radiance-cache GI (t10 Illuminance) → indirect lighting; the full lightPass.comp consumes it.
- **point lights + LightCulling** (light grid t8 + index list t9; point shadows reuse the proven TLAS).

Verification bar: BuildWin exit 0; TestGIScene (non-GBV) exit 0 + 0 D3D12 errors + MAE ≤ 0.45
(currently 0.289); TestSuite green. GBV NOT clean — sole residual is the pre-existing **TASK-163**
FinalBlend present-target readback barrier (separate ticket).

RECOMMENDED NEXT: **SSRC GI** (biggest remaining fidelity jump — indirect bounce; large multi-pass) or
**point lights + LightCulling** (medium), then swap the minimal LightPass for the full lightPass.comp.
TASK-163 for a clean -gpu_validation run. Resume note in `InnocenceEngine` basic-memory mirrors this.

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

Fresh independent peer-review of ecf7aca9 (deferred-RT primitive + SkyPass migration) ran 2026-06-01 — verdict ADVISORY, commit stands (no revert). Two follow-up findings to carry forward so they aren't lost:

1. IsMultiBuffer parity divergence. Imperative SkyPass Result inherited IsMultiBuffer=true from GetDefaultRenderPassDesc().m_RenderTargetDesc (RenderingConfigurationService.cpp:30). Graph path builds the texture from JSON TextureDesc; TextureDescFromJson never sets IsMultiBuffer (RenderGraphSerializer.cpp:18-29) so it falls to struct default false (GraphicsPrimitive.h:120). Effect: RT goes from N-per-swapchain physical resources to 1. Internally consistent today (bind keys off the texture's own flag; writer+same-frame reader PreTAAPass both hit handle 0), so MAE stays in band. Becomes a read-after-write/temporal hazard if any FUTURE consumer reads 'Sky Pass Result' across a frame boundary. Decide: round-trip IsMultiBuffer through the serializer if multi-buffering should be data-expressible, else it was incidental.

2. SkyPass::Terminate deletes graph-owned resources (SkyPass.cpp:114-115, unchanged by migration). In graph path m_RenderPass/m_ShaderProgram/m_Result are the node's resources; RenderGraphService dtor is =default and frees only node structs, so node pointers dangle after SkyPass::Terminate. Shutdown-only single-delete (no double-free) — low impact now, widens as more passes migrate. Needs a teardown-ownership contract.

3. (non-defect) Dropped ClearRenderTargets — no-op for full-screen ComputeOnly dispatch writing every tile. Noted for completeness.

2026-06-01 — DESIGN (next increment, NOT yet built): graphics-CL state-transition PREPASS primitive. Audit + plan; no source diffs this stage.

=== AUDIT ===
Imperative shape (PreTAAPass.cpp:126-130, SSAOPass.cpp:220-223): a Graphics CL wraps an explicit ordered list — CommandListBegin(Graphics); TryToTransitState(input, WriteOnly->ReadOnly) [N inputs]; TryToTransitState(result, ReadOnly->WriteOnly); CommandListEnd. TryToTransitState (DX12FrameManagementService_RenderTargets.cpp:11-48) records a ResourceBarrier on the PASSED CL — it does NOT pick a queue; the 'must be graphics' rule is the HW constraint (compute queue can't do RT ReadOnly->WriteOnly), enforced by the CALLER choosing m_CommandListComp_Graphics. Direction is hand-authored per consuming pass.
Execution/sync OWNED BY THE CLIENT, not the graph (ExampleRenderingClient_ExecuteCommands_Rasterizer.cpp:64-78,138-153): per pass — Execute(GraphicsCL,Graphics); SignalOnGPU(Graphics); WaitOnGPU(Compute waits Graphics); Execute(ComputeCL,Compute); SignalOnGPU(Compute). RecordNode (RenderGraphService.cpp:183-202) only RECORDS the compute CL via the kernel; never executes/fences. Migrated SkyPass has NO prepass (imperative SkyPass never transitioned its own Result — the CONSUMER PreTAA does), so SkyPass records only compute, client does a single compute Execute (client:130-136; SkyPass.cpp:146-148). KEY: the transition prepass is a property of the CONSUMING pass.

=== FORK 1 (data shape) — DECISION: per-node 'Transitions' array, kernel-executed; NOT a distinct barrier node ===
Rejected distinct barrier-node type and fully-inferred barriers: service has no scheduler/topo-sort (m_Schedule is load order, RenderGraphService.h:27) and no per-resource last-state tracking, so DIRECTION cannot be inferred yet — authored either way; a standalone node doubles node count for zero inferred benefit. Inferred barriers (FrameGraph/Granite-style) are the eventual target once the scheduler lands; this is a non-blocking stepping stone. Precedent: deferred-RT put per-node behavior on a node property + engine hook (RenderGraphService.cpp:113-133).
Shape: each pass node gains an optional ORDERED array, entry = { Resource, From, To }; From/To are Accessibility enum strings (AccessibilityFromString already round-trips them). Authored in the SAME order as the imperative calls (inputs WriteOnly->ReadOnly first, result ReadOnly->WriteOnly last). Resource resolves via EXISTING FindResource (graph-owned + imported-by-name incl. PerFrameCBuffer special-case). Empty/absent => no prepass (SkyPass stays single-CL, unchanged).

=== FORK 2 (execution/sync) — DECISION: client keeps owning Execute/Signal/Wait; RecordNode records BOTH CLs ===
Rejected moving submission/fencing into RenderGraphService: all queue submission lives per-pass in the client, hand-sequenced via WaitIfActive against neighbors; relocating it is a larger refactor outside .2 scope. Pick: when a node has a non-empty Transitions array, RecordNode ALSO records the graphics-prepass CL (Begin(node->m_CommandList_Graphics); per-entry TryToTransitState; End) BEFORE the compute CL. The migrated pass's client block stays byte-for-byte the imperative graphics->Signal->compute-Wait->compute sequence — migration swaps only the RECORDING source, not submission topology.

2026-06-01 — DESIGN cont'd (state-transition prepass, part 2/2).

=== DATA SHAPE (JSON additions) ===
PassNodeDesc gains Inno::Array<TransitionDesc> m_Transitions; new struct TransitionDesc { std::string m_Resource; Accessibility m_From; Accessibility m_To; } in RenderGraphDesc.h. Serializer: TransitionTo/FromJson (mirror BindingTo/FromJson, RenderGraphSerializer.cpp:78-98) wired into PassTo/FromJson under key 'Transitions'; omit when empty (sparse-key convention for Imported/Size at :58-61). No new enum-string entries (Accessibility table already covers ReadOnly/WriteOnly).

=== PREPASS RECORDING LOCATION — code-organization ===
DefaultKernel::Record is compute-only (DefaultKernel.cpp:7-61). Options: (i) inline prepass loop in Record guarded by non-empty transitions; (ii) reusable free fn RecordTransitionPrepass(ctx, graphicsCL, fmService) at top of Record. PICK (ii): prepass is orthogonal to dispatch and shared VERBATIM by Default AND ScreenTile (ScreenTile derives from Default; deferred-RT cluster passes are all ScreenTile-dispatch AND need the prepass). Inlining forces a copy/awkward override. Helper in NEW RenderGraphTransitions.{h,cpp} (engine RenderGraph dir) — one capability per file, DefaultKernel.cpp stays under budget. RenderGraphPassContext carries only single m_CommandList (IRenderGraphKernel.h:17); add m_CommandList_Graphics, populated in RecordNode from node->m_CommandList_Graphics.

=== FILES CHANGED ===
- RenderGraphDesc.h: + TransitionDesc, + m_Transitions on PassNodeDesc.
- RenderGraphSerializer.cpp: + TransitionTo/FromJson, wire into Pass to/from.
- IRenderGraphKernel.h: + m_CommandList_Graphics on RenderGraphPassContext.
- RenderGraphService.cpp RecordNode: populate ctx.m_CommandList_Graphics.
- RenderGraphTransitions.{h,cpp} (NEW): RecordTransitionPrepass.
- DefaultKernel.cpp: call helper at top of Record when node has transitions.
- RenderGraphTransitionTests.cpp (NEW, TestSuite explicit list): round-trip.
- ExampleRenderGraph.json: + Transitions array on migrated consuming pass.
- PreTAAPass.cpp (recommended): SetupFromRenderGraph + g_UseRenderGraph seam like SkyPass; PrepareCommandList delegates to RecordNode; client block UNCHANGED.

=== RECOMMENDED NEXT TARGET: PreTAAPass ===
Simplest deferred-RT pass carrying the prepass. 3 transitions (LightPass luminance + Sky Pass Result WriteOnly->ReadOnly; own Result ReadOnly->WriteOnly); all 3 inputs already-existing graph/engine resources (Sky Pass Result graph-owned since ecf7aca9; LightPass luminance imports by name). No ping-pong, no raytracing, ScreenTile dispatch. Proves the primitive with a real graphics-queue ReadOnly->WriteOnly RT transition. SSAO deferred (extra Kernel buffer + noise tex + OpaquePass MRT reads = more bindings; after PreTAA).

2026-06-01 — DESIGN cont'd (state-transition prepass, part 3/3: interactions + verification).

=== SkyPass INTERACTION ===
SkyPass already migrated, writes 'Sky Pass Result', NO prepass — correct (it never transitioned its own output imperatively). When PreTAA migrates, PreTAA's prepass transitions 'Sky Pass Result' WriteOnly->ReadOnly via FindResource on the graph-owned texture. No change to SkyPass.

=== SINGLE- vs MULTI-BUFFER (review caveat #1) ===
Graph RTs are single-buffered (IsMultiBuffer drops in TextureDescFromJson, RenderGraphSerializer.cpp:18-29). TryToTransitState operates per-frame-index on the texture's own resources (DX12FrameManagementService_RenderTargets.cpp:14-16,44). For SAME-FRAME producer->consumer (Sky writes, PreTAA reads same frame) single-buffer + a correct WriteOnly->ReadOnly barrier suffices — no RAW hazard. This primitive does NOT fix cross-frame temporal hazards; a future CROSS-FRAME temporal consumer (TAA history / ping-pong) needs IsMultiBuffer round-tripped first. Ping-pong is the SEPARATE next primitive — do not conflate.

=== BUILD SEQUENCE ===
1. TransitionDesc + serializer + round-trip test (data only) -> TestSuite green.
2. ctx.m_CommandList_Graphics + RecordTransitionPrepass helper + DefaultKernel call (behavior, no migrated pass) -> build green.
3. PreTAA Transitions in JSON + migrate PreTAAPass behind g_UseRenderGraph; client block unchanged.
4. Verify (below).

=== RUNTIME VERIFICATION (what CAN exercise it) ===
Unlike deferred-RT resize (offscreen smoke can't trigger PostResize), the prepass runs EVERY FRAME. GISponza -gpu_validation -total_frames 120 exercises the graphics-CL ReadOnly->WriteOnly RT barrier every frame; GBV flags an illegal compute-queue transition or a missing/wrong barrier. So the 120-frame smoke is a REAL exercise of this primitive (contrast: deferred-RT was correct-by-construction only). TestGIScene MAE confirms PreTAA output parity vs imperative (clean-baseline band ~0.50; the 0.45 FAIL is pre-existing/accepted). Expected: BuildWin exit 0; TestSuite incl new round-trip; GISponza exit 0, 0 D3D12 errors, graph +1 pass. NOT verifiable here: actual window-resize RT re-creation (offscreen, as before); RenderDoc visual capture (parity by MAE band only).

NOT building this stage — design only. Handing back to dispatcher to route to code-impl. PING-PONG remains the separate primitive after this one. [task-stays-open]

2026-06-02 — Increment 3 LANDED (commit ce4a62c3, peer-reviewed PASS by fresh code-review agent). Built the graphics-CL state-transition prepass primitive as per-node data and migrated PreTAAPass. Shape exactly as designed: TransitionDesc{m_Resource,m_From,m_To} + Inno::Array<TransitionDesc> m_Transitions on PassNodeDesc; TransitionTo/FromJson mirror the binding serializer (sparse key, omitted when empty, enums via existing Accessibility table — no new enum entries); ctx.m_CommandList_Graphics added + populated in RecordNode; RecordTransitionPrepass in new RenderGraphTransitions.{h,cpp}; DefaultKernel calls it at top of Record when transitions non-empty (ScreenTileKernel inherits). PreTAAPass behind g_UseRenderGraph (imperative body verbatim under false), 3 transitions (luminance + Sky Pass Result WriteOnly->ReadOnly; own Result ReadOnly->WriteOnly), Result resolved lazily in PrepareCommandList; client submission/fencing block UNCHANGED.

POST-REVIEW HARDENING (folded into ce4a62c3, not a separate commit): RecordTransitionPrepass now PRE-VALIDATES every transition resource before opening the graphics CL and aborts the pass loud (Log Error, return false) if any is unresolved — was previously log-Warning + continue, which would dispatch with a missing barrier (review advisory-low). Also corrected the stale m_CommandList_Graphics comment (review nit).

Verified (main session): BuildWin exit 0 (Main + RenderTest); TestSuite 6/6 RenderGraph incl new transition round-trip, 0 failures; GISponza -gpu_validation -total_frames 120 exit 0, graph loads 11 resources/6 passes, PTReadback nonZero=921600, zero real D3D12 errors (only the pre-existing engine-wide non-fatal GBV Release-shader false positives on lightPass/skyPass/lightCulling/finalBlendPass — none on PreTAA; classifier upgrades real GBV errors to fatal exit-1 when total_frames>0, so exit 0 == zero real errors). The 120-frame GBV run is a REAL per-frame exercise of the graphics-CL ReadOnly->WriteOnly barrier. NOT verified: runtime window-resize RT re-creation (offscreen can't trigger PostResize; correct-by-construction); no committed capture A/B (flag-flip alone double-registers Pre-TAA Pass Result — parity argued by construction: identical transitions/bindings/dispatch, ClearRenderTargets no-op, unchanged submission). MAE band ~0.52 (the 0.45 FAIL is pre-existing/accepted).

CARRY-FORWARD (review, non-blocking): (1) IsMultiBuffer drops through TextureDescFromJson so graph RTs incl. Pre-TAA Pass Result are single-buffered — safe today (PreTAA->TAA same-frame, no cross-frame consumer) but must be round-tripped before any cross-frame/temporal consumer; (2) Terminate-ownership: migrated passes delete graph-owned resources in their Terminate (shutdown-only single-delete now, widens as more passes migrate — needs a teardown-ownership contract).

NEXT: ping-pong primitive (TAAPass history, SSRC Even/Odd — may obsolete TASK-128) and the remaining deferred-RT passes that also need this prepass (SSAO/PostTAA/FinalBlend/SSRC filter/spatial/integration). Per-frame import + dynamic dispatch + deferred-RT + state-transition prepass primitives all now exist; ping-pong is the last bin-b primitive. [task-stays-open]


2026-06-03 — Increment 4 LANDED (commit 8fd2e7a6, peer-reviewed PASS). Migrated SSAOPass — the first CLEAN-REUSE of the state-transition prepass primitive (no new primitive). 7 passes now graph-driven; graph loads 16 resources / 7 passes. SSAOPass behind g_UseRenderGraph: 2-entry prepass (SSAO_Noise WriteOnly->ReadOnly, SSAO_Result ReadOnly->WriteOnly), 8 bindings in layout-array order, SSAO_Result the lone graph-owned deferred screen RT; kernel buffer / noise texture / 2 samplers stay imperatively created+filled (SetupOwnedResources, both paths) and imported by name (graph never allocates them). SSAOPass.cpp split 273->178 with binding-layout in SSAOPass_Setup.cpp (300-gate). Engine: ResolveImportedResource now also resolves SamplerResourceService (new Find -> NamedObjectPool::Find); SSAO was the first pass importing samplers by name.

Verified (main session): BuildWin exit 0 (Main+RenderTest); TestSuite 110/110 incl. new SSAO-node round-trip (binding order + imported flags + transition order/dir); GISponza -gpu_validation -total_frames 120 exit 0, 16 res/7 passes, PTReadback nonZero=921600, 0 real D3D12 errors; TestGIScene MAE 0.493 (clean-baseline band). Not verified: window-resize RT re-creation offscreen; no committed capture A/B (parity by construction + MAE band + per-frame GBV barrier exercise).

NEXT: remaining CLEAN-REUSE passes (audited, ready, same pattern) = the SSRC chain (SSRCFilterHorizontal/Vertical, SSRCSpatialHorizontal/Vertical, SSRCIntegration) — all inputs statically named from prior passes/SSRCReprojection; deferred RTs (SSRCIntegration RT is probe-grid-sized). Migrate next, small batches. BLOCKED on new primitives: PostTAA + FinalBlend (NEEDS-DYNAMIC-INPUT: runtime renderingContext->m_input, no stable graph name; dynamic-dispatch covers size only), TAA (that + ping-pong Even/Odd). MotionBlur bypassed (returns false). [task-stays-open]


2026-06-09 — Core rebuild progress (first-principles render graph; supersedes the incremental retrofit). Commits on ecs-overhaul:
- 6c989947: executor collapsed to a single data-driven RecordPass; IRenderGraphKernel/DefaultKernel/ComputeCullingKernel/ScreenTileKernel deleted; dispatch is DispatchDesc Mode (Static/ScreenTile/TiledTwoLevel/DrawModelGroups); Kernel JSON field removed.
- e259d0e0: archived all non-graph-ready passes to RenderingClient/_Archive/ (87 renames); kept 8 graph passes; gutted client orchestration + the 3 passes' dead g_UseRenderGraph fallbacks. Factory CreateOrphanResources inits resources whose producer is archived (zeroed inputs). Degraded render intentional for core dev.
- d2059314: commit-guard fix — comment-essay-cap skips rename targets (false positive that blocked archival/move commits).
- ad51eb47: graph OWNS component+resource init (CreatePassNode inits node shader/renderpass/CLs; CreateResource inits graph-owned buffers+non-screen textures; screen RTs via writer RT-init-func or orphan loop). Every graph-owned resource/component initialized exactly once by the factory, never by a pass (no double-init, reviewed PASS). ComputeCullingPass de-specialized: its IndirectDrawCommandBuffer is graph-owned; the pass is a thin shell (adopt + RecordNode + GetResult-by-name); its bespoke dynamic-dispatch/empty-out/UAV-state logic is now node data (DrawModelGroups + TrackWriteState).

State of the "nuke the cpp" goal: the 8 keep-set pass classes are now thin shells — SetupFromRenderGraph (adopt node components) + empty/imported-only Initialize/Terminate + PrepareCommandList (FindNode+RecordNode) + GetResult/GetRenderPassComp accessors. They CANNOT be deleted yet because the client's per-pass submission (ExampleRenderingClient_ExecuteCommands_Rasterizer.cpp) still calls Pass::Get().GetCommandListComp()/GetRenderPassComp() for Execute/Signal/Wait.

NEXT: P4 — graph-owned submission (derive Execute/Signal/Wait per node from the schedule + reads-to-producer edges + queue) + named init/update hooks for the remaining pass-owned imported resources (SSAO kernel/noise/samplers, TiledFrustum dispatch-params). Once the graph owns submission, the keep-set pass classes become unreferenced and can be deleted, making migrated passes pure JSON. THEN migrate more passes (LightPass/OpaquePass need multi-RT output; PostTAA/FinalBlend need dynamic-input; TAA needs ping-pong). Known limitation: indirect buffer ElementCount hardcoded 1024 (=maxMeshes default) in JSON. Full plan: local plan file render-graph-rebuild.md + basic-memory note innocence-engine/render-graph/task-227-render-graph-state-resume-point-1. [task-stays-open]


2026-06-09 (cont.) — P4 graph-owned submission LANDED.
- ad51eb47 had a latent double-init: its graph-owned-init committed the graph side but the 7 passes' Initialize/Terminate guts (removing component init/delete) were left UNCOMMITTED (staging miss; the review validated the working tree which had them). Fixed in dac9eaa1.
- dac9eaa1: RenderGraphService::Render() owns the whole frame — records every ready scheduled node then submits+fences, deriving the topology from node data: producer waits (read->writer-node signal, e.g. BRDFLUTMS<-BRDFLUT, PreTAA<-Sky), graphics transition prepass dance, compute execute/signal, one-shot CPU-wait-once. Client collapsed: PrepareCommands resolves the present canvas by name; ExecuteCommands = Render() + capture/audit. ExecuteRasterizerPasses/ExecuteGIPasses archived. The 7 keep-set pass classes are now INERT SHELLS (Setup adopts node components for GetResult accessors; Initialize/Terminate do nothing for graph-owned things; imported owned-resource setup kept). Verified BuildWin exit 0; -total_frames 30 exit 0 / 0 [Error] / 18 res 8 passes. Submission parity + no-double-record confirmed by review before account-limit; init dissolution PASS prior.

KNOWN ISSUE (pre-existing since the archival e259d0e0, NOT P4): Main.exe -gpu_validation reports a ResourceBarrier before-state mismatch on PreTAAPass/Graphics prepass (before-state includes COPY_SOURCE). Cause: the present/capture target became "Pre-TAA Pass Result" when FinalBlendPass was archived; the screen-capture readback (HandleScreenCapture/WriteCaptureToFile/AlignTrackerForMidFrameReadback) transitions that graph RT to COPY_SOURCE, desyncing PreTAA's per-frame prepass tracker. Capture path + target are unchanged by P4 (barrier before-state is recorded from the prior frame's capture in both old and new flows), so it predates P4. RESOLVES when FinalBlendPass is re-migrated (capture target stops being a per-frame-transitioned graph RT) OR by giving capture a dedicated readback-state alignment for the canvas. Tracked here; non-blocking for core dev (prior smokes used -total_frames 30 without -gpu_validation).

NEXT (toward pure-JSON passes): P5 — move the remaining pass-owned IMPORTED resources to named init/update hooks registered with the graph (SSAO kernel/noise/samplers + SetupOwnedResources, TiledFrustum dispatch-params + RenderTargetsCreationFunc, ComputeCulling none left). Then the inert shells become fully unreferenced (graph loads the JSON in BRDFLUTPass::SetupFromRenderGraph -> move LoadGraph to client Setup) and can be DELETED -> migrated passes are pure JSON. THEN migrate more passes (LightPass/OpaquePass multi-RT; PostTAA/FinalBlend dynamic-input; TAA ping-pong). [task-stays-open]


2026-06-10 — P5 (partial) LANDED: 6 of 8 migrated passes are now PURE JSON NODES (no C++ class). Commit 9361b9f8.
- Deleted BRDFLUTPass, BRDFLUTMSPass, OpaqueCullingPass (+ComputeCullingPass base), SkyPass, PreTAAPass, LuminanceAveragePass (.h+.cpp). Their nodes remain in ExampleRenderGraph.json; the graph owns create/init/record/submit/fence.
- LoadGraph moved from BRDFLUTPass::SetupFromRenderGraph to client Setup (runs before the 2 remaining passes' Setup).
- Client lifecycle (Initialize/Update/Terminate/GetDispatchedPasses) reduced to SSAOPass + TiledFrustumGenerationPass. AuditDump repointed to graph (FindNode + GetResource by name).
- Verified: BuildWin exit 0; GISponza -total_frames 30 exit 0 / 0 [Error] / 18 res 8 passes; TestSuite RenderGraph unit tests 8/8. Reviewed PASS.
- Pre-existing unrelated: TestSuite AssetConversion integration test exits 9 (file-I/O; TestSuite does not link the client). Not caused by the rebuild.

REMAINING to finish "pure JSON" goal:
- SSAOPass + TiledFrustumGenerationPass still have C++ classes ONLY because they own IMPORTED resources carrying CPU init data: SSAO noise texture (random rotations) + sample-kernel buffer + 2 samplers (SetupOwnedResources + Initialize fill); TiledFrustum dispatch-params buffer (Initialize + per-frame Update upload). To delete them: add named init/update HOOKS registered with the graph (graph calls them when creating the node / per frame), move that resource gen+upload into client-registered callbacks, then delete the 2 classes.
- THEN migrate more passes (LightPass/OpaquePass need multi-RT output; PostTAA/FinalBlend need a dynamic-input primitive; TAA needs ping-pong). Re-migrating FinalBlend also clears the known GBV capture-barrier issue (present target stops being a per-frame graph RT).
[task-stays-open]


2026-06-10 (cont.) — NAMED HOOKS landed (commit 173a5b79): ALL 8 migrated passes are now pure JSON nodes; the rendering client has ZERO render-pass C++ classes. The core-rebuild goal ("just load the JSON, no cpp") is achieved for the keep-set.
- Hook mechanism: RenderGraphService::RegisterInitHook/RegisterUpdateHook (keyed by node name). LoadGraph runs each node's init hook once (after nodes+resources created); Render() runs a node's update hook each frame before recording it. Client registers hooks in Setup before LoadGraph.
- SSAONoisePass init hook = kernel + 4x4 noise + 2 samplers (moved from deleted SSAOPass). TiledFrustumGenerationPass init hook = dispatch-params + viewport-sized frustum buffer + extent; update hook = per-frame extent upload (moved from deleted class). Deleted SSAOPass.{h,cpp}+_Setup, TiledFrustumGenerationPass.{h,cpp}.
- Client: Setup = RegisterGraphHooks()+LoadGraph(); Initialize/Update empty; GetDispatchedPasses returns {}.
- code-review caught a real BLOCKER (fixed): SSAO kernel/noise must be PERSISTED (file-static, not lambda-locals) because resource Initialize is DEFERRED — the upload memcpy reads the pointer from the frame loop after the hook returns; locals = use-after-free. The deleted class kept them as members for the same reason.
- Known limitation: TiledFrustum viewport-derived buffer sized once (no offscreen resize hook yet).
- Verified: BuildWin exit 0; GISponza -total_frames 30 exit 0 / 0 [Error] / 18 res 8 passes (init+update hooks exercised).

FULL COMMIT SEQUENCE (core rebuild, ecs-overhaul): 6c989947 (no kernel types) -> d2059314 (gate fix) -> e259d0e0 (archive non-graph passes) -> ad51eb47 (graph owns init + ComputeCulling de-spec) -> dac9eaa1 (P4 graph owns submission) -> 9361b9f8 (delete 6 pass classes) -> 173a5b79 (named hooks, delete last 2). Plus docs commits.

NEXT (separate phase — NOT done): migrate MORE passes from _Archive back into the graph. Needs new primitives: LightPass/OpaquePass = multi-render-target output (graph node Writes model + serializer support >1 RT); PostTAA/FinalBlend = dynamic-input (runtime renderingContext->m_input, no stable graph name); TAA = ping-pong (frame-parity Even/Odd history). Re-migrating FinalBlend also clears the known GBV capture-barrier issue (present target stops being a per-frame graph RT). [task-stays-open]

2026-06-10 (cont.) — FIRST re-migration from _Archive: FinalBlendPass is back as a PURE JSON NODE (no C++ class restored). It needed NO new primitive: in the keep-set the PT chain is archived, so FinalBlend's input is statically "Pre-TAA Pass Result" (the present canvas) — the dynamic-input variance (PT vs raster) only existed when both chains were live. So FinalBlend is a clean-reuse of the SSAONoise pattern (deferred screen RT + graphics transition prepass + ScreenTile dispatch + static named reads).
- Shader finalBlendPass.comp: dropped the billboard (t1) + debug (t2) Texture2D inputs + their compositing. Those passes are archived; the imperative pass already left those descriptor slots unbound (null descriptors → sampled as 0), so removal is behavior-preserving for the keep-set. Remaining: b0 PerFrame, t0 input, u0 luminanceAverage, u1 result.
- JSON: added resource "Final Blend Pass Result" (screen deferred RT) + node "FinalBlendPass" (Compute, ScreenTile 8, reads PerFrameCBuffer/Pre-TAA Pass Result/LuminanceAverageGPUBuffer, writes Final Blend Pass Result, 4 bindings, 2 transitions). Present canvas repointed Pre-TAA→Final Blend in PrepareCommands.
- Capture readback (gutted at archival e259d0e0) restored, repointed off the deleted FinalBlendPass::Get() to the client's per-frame-resolved m_Canvas/m_CanvasOwner (null-guarded; members now = nullptr default).
- DRIVE-BY FIX (separate pre-existing GBV defect introduced by the e259d0e0 archival, surfaced because later core-rebuild commits dropped -gpu_validation): SSAONoisePass read the orphan OpaquePass_RT_0/RT_1 (OpaquePass archived) while still in UAV state → GBV "invalid for NON_PIXEL_SHADER_RESOURCE" fatal. Added their WriteOnly→ReadOnly transitions to the SSAONoise node (idempotent; From is advisory, engine transitions from actual state).
- Verified: BuildWin exit 0 (Main+RenderTest+TestSuite, finalBlendPass.comp recompiled); TestGIScene (non-GBV) exit 0, GISponza loaded, auto-terminated, 0 D3D12 errors, pixel MAE 0.103 — DOWN from the pre-existing ~0.50 because the captured/presented image is now the tonemapped FinalBlend output instead of raw Pre-TAA (real fidelity restoration, not just parity); TestSuite RenderGraph unit tests 8/8. Peer-reviewed PASS (2 non-blocking advisories, both addressed/pre-existing).
- CORRECTION to the earlier hope: re-migrating FinalBlend did NOT clear the GBV capture-barrier issue — it RELOCATED it from "Pre-TAA Pass Result" back to "Final Blend Pass Result". This is the known pre-existing TASK-163 (Final Blend readback state drift, autocapture path): the pre-archival imperative FinalBlend tripped the identical barrier (same present+capture target, same ReadWrite→WriteOnly transition). The present/capture-vs-graph-prepass tracker desync is inherent to whatever RT is the present+capture target; NOT introduced here. Non-GBV path is clean. GBV residual ⇒ TASK-163.
- NEXT: continue _Archive re-migration. SSRC chain (SSRCFilter/Spatial H+V, SSRCIntegration) is clean-reuse. Still need primitives: LightPass/OpaquePass = multi-RT output; PostTAA = dynamic-input (only needed once PT chain returns); TAA = ping-pong. [task-stays-open]

2026-06-11 — PING-PONG PRIMITIVE + TAA/PostTAA migrated (non-SSRC track). User steered away from the SSRC clean-reuse batch toward the primitive-needing passes. Findings that reshaped the plan: OpaquePass is a full raster/multi-RT subsystem whose payoff is INVISIBLE until LightPass; LightPass is entangled with raytracing (TLAS/RayQuery, SunShadowRT), Volumetric, LightCulling AND reads SSRC GI (t10 Illuminance from Radiance Cache) — so the visible-geometry payoff is blocked by the excluded SSRC. PostTAA turned out to be CLEAN-REUSE (static input in the keep-set, like FinalBlend — no real dynamic-input primitive). TAA is the one genuine, tractable, non-blocked primitive: frame-parity ping-pong. User chose TAA+PostTAA.
- NEW PRIMITIVE (ping-pong / frame-parity), additive + data-driven: ResourceDesc.m_PingPong (creates two screen textures "<Name> (Even)"/"(Odd)"); BindingDesc.m_PingPongHistory + TransitionDesc.m_PingPongHistory (resolve the OTHER-parity = previous-frame texture). FindResource(name) returns CURRENT-parity for a ping-pong logical name (parity = GetFrameCountSinceLaunch()%2==1 → Odd is current; bit-exact to archived TAAPass::GetResult/GetHistory). Logical name stays OUT of m_Resources (resolved per-frame via m_PingPong map); physical names go in for resize-delete. CreateScreenSizedTexture branches to (re)create both parities; RecordNode + transition prepass resolve history via PingPongTexture(...,true)/GetHistoryResource. Touches RenderGraphDesc.h, Serializer, Service(.h/.cpp/_Resources/_Transitions). New unit test RenderGraphSerializerTests_PingPong.cpp.
- JSON: +"TAA Pass Result" (PingPong), +"Post-TAA Pass Result" (deferred RT), +"OpaquePass_RT_3" (zeroed orphan motion-vector placeholder, mirrors RT_0/1 — becomes writer-owned once OpaquePass migrates). +TAAPass + PostTAAPass nodes; FinalBlend input repointed Pre-TAA → Post-TAA. New present chain: PreTAA → TAA → PostTAA → FinalBlend (22 resources / 11 passes). Shaders TAAPass.comp/postTAAPass.comp unchanged (already compiled).
- Verified: BuildWin + TestSuite exit 0; TestGIScene (non-GBV) exit 0, GISponza loaded + auto-terminated, 0 D3D12 errors, MAE 0.103 (unchanged — motion is a zeroed orphan so TAA degenerates to a history/current blend with no reprojection, PostTAA is pass-through; correct given no GBuffer); GBV ran to the LAST pass with NO new errors from the ping-pong textures (cross-frame parity barriers are clean because TryToTransitState ignores the From hint and uses actual per-physical-texture/per-frameIndex state) — only the pre-existing TASK-163 FinalBlend present-target barrier remains; RenderGraph unit 9/9 incl ping-pong round-trip. Peer-reviewed PASS (3 optional advisories, non-blocking).
- NEXT: SSRC chain (clean-reuse, audited) and/or the OpaquePass raster/multi-RT primitive (foundational; needs RT-output + depth + indirect-draw + root-constants across Desc/Serializer/Service/Recorder; populates the real GBuffer + motion vector, retiring the OpaquePass_RT_0/1/3 orphans; invisible until LightPass). LightPass blocked on SSRC GI + raytracing. [task-stays-open]

2026-06-12 — RASTER/MULTI-RT PRIMITIVE + OpaquePass migrated (commit 0b507daf, peer-reviewed ADVISORY — no blocking issues). First graphics-queue node in the graph. OpaquePass draws into a 4-RT + depth OutputMergerTarget via ExecuteIndirect (args = OpaqueCullingPass/IndirectDrawCommandBuffer); the engine auto-creates OpaquePass_RT_0..3 + _DS, which downstream SSAO (RT_0/1) and TAA (RT_3) resolve by name — the 3 zeroed-orphan RT resources were removed from JSON. Graph: 19 resources / 12 passes.
- NEW PRIMITIVE (additive, data-driven): BindingDesc.m_IsRootConstant + m_SubresourceCount (root-signature slot whose value rides the indirect command signature per draw; DX12 BindGPUResource no-ops it, recorder skips it). PassNodeDesc.m_Raster (RasterDesc: RT count, depth buffer, indirect draw, depth-stencil + cull state, CrossQueueExit::ToCommon, indirect-args buffer name). CreatePassNode raster branch configures a graphics OutputMerger pass (RT format/size = engine default, screen-sized, matching imperative). RenderGraphPassRecorder raster body: ClearRenderTargets → bind non-root-constant resources (empty name "" = intentional null bind for the bindless texture slot t3) → ExecuteIndirect. Submit executes the GRAPHICS CL for graphics-queue nodes (was hardcoded to compute CL). VS/PS shader paths + raster block round-trip through the serializer; new RenderGraphSerializerTests_Raster unit test.
- Dynamic per-frame resolution added for PerFrameCBufferPrev (GetPreviousFrameBuffer) + TransformBuffer (GetCurrentFrameTransformBuffer) — both parity-double-buffered like the existing PerFrameCBuffer special-case; import-by-name would pin the wrong parity.
- Verified invariants: DX12 root-sig param visibility defaults to ALL (m_ShaderStage does NOT gate it), and BindGPUResource ignores the stage arg — so the binding ShaderStage is documentation-only. ExecuteIndirect emits its own indirect-arg barrier; PostCLState=ToCommon emits the RT→COMMON barrier at CommandListEnd.
- Verified: BuildWin exit 0 (Main+RenderTest+TestSuite); TestSuite 113/113 incl new raster round-trip; non-GBV TestGIScene exit 0, GISponza loaded + auto-terminated, 0 D3D12 errors, MAE 0.103 (UNCHANGED — present FinalBlend chain still reads the zeroed LightPass luminance; the real GBuffer/motion feed SSAO + TAA-reprojection whose visible effect waits on LightPass); GBV -gpu_validation loads 19 res/12 passes, OpaquePass graphics CL records + ExecuteIndirect with no GBV error (only the standard indirect "Shader Patch Mode NONE" undervalidation warning); sole fatal GBV residual is the pre-existing TASK-163 FinalBlend present-target barrier. Peer review ADVISORY: 2 shader-stage findings, both non-defects (DX12 visibility=ALL + stage-arg-ignored, runtime-confirmed).
- NEXT: raytracing primitive (TLAS/RayQuery/shader tables) — unblocks SunShadowRT and a returning LightPass (which would light the new GBuffer = first visible payoff). The SSRC chain is NOT clean-reuse (see anchor). [task-stays-open]

2026-06-13 — MINIMAL LightPass migrated (commit b3351c7, peer-reviewed PASS, visually reviewed). First VISIBLE lit frame of the keep-set: the present chain now renders a recognizable sun-lit Sponza. Pure-JSON compute node reusing existing primitives (deferred-RT output, static GBuffer reads, sampler hook) — NO new engine code.
- New shader lightPassSimple.comp: DecodeGBuffer -> sun as an UNSHADOWED directional light via the shared AccumulateLightContribution + a small flat ambient (albedo*0.05) for legibility. Plain cs path (no RayQuery/TLAS). Degraded stand-in for the full lightPass.comp (which needs radiance-cache GI t10 + tiled point lights + inline-RayQuery shadows + hardware-RT sun visibility t13 — all excluded). Dropping them also removed the SSAO dependency, so no pass reorder. LightPass node reads PerFrameCBuffer + OpaquePass_RT_0/1/2 + BRDF LUT/MS, writes the existing "LightPass Luminance Result" orphan (PreTAA already reads it), ScreenTile TileSize 8. + a LightPass point-sampler init hook.
- VISUAL: Bin/gpu_output.png shows lit Sponza (stone walls/floor/columns). High-albedo surfaces (drapes) overexpose under AgX auto-exposure — expected for the direct-only/no-GI/no-shadow path, not a bug. Auto-exposure works correctly.
- GOTCHA recorded in the anchor: TestGIScene writes the real capture to Bin/gpu_output.png (Main.exe CWD = Bin/, the PARENT of the -BinDir RelWithDebInfo). Reading Bin/RelWithDebInfo/gpu_output.png (stale from a direct run) shows a flat 245 frame and caused a long false "broken auto-exposure / flat present" misdiagnosis before the blue-probe MAE jump (0.139->0.54) exposed the wrong-file read. Always view Bin/gpu_output.png.
- Verified: BuildWin exit 0; lightPassSimple.comp compiles; TestSuite 113/113; TestGIScene (non-GBV) exit 0, 0 D3D12 errors, MAE 0.139 (up from 0.103 — real luminance now contributes); GBV loads 19 res/13 passes, LightPass CL clean, only the pre-existing TASK-163 FinalBlend residual.
- NEXT: fidelity — raytracing (SunShadowRT first = real sun shadows, smallest RT entry) or SSRC GI; then swap the minimal LightPass for full lightPass.comp. [task-stays-open]

2026-06-13b — SunShadowRT RAYTRACING node migrated + sun shadows wired into LightPass (commit 7f8b9164, peer-reviewed PASS 0 bugs, visually reviewed). First raytracing pass in the graph; the present chain now renders a sun-SHADOWED, well-exposed Sponza. Graph: 20 resources / 14 passes. NOTE: the engine-side RT primitive + the SunShadowRT/LightPass JSON nodes + the round-trip test were pre-existing working-tree WIP (zhangdoa); this increment completed it by wiring lightPassSimple.comp to sample the visibility.
- RAYTRACING PRIMITIVE (additive, data-driven): BindingDesc.m_GPUBufferUsage (TLAS → root SRV, DX12 requires root-SRV for accel structures). DispatchMode::DispatchRays (recorder sizes to screen res, calls FrameManagementService::DispatchRays; engine self-guards on IsTLASReady → skipped until TLAS built, no early-frame UB). PassNodeDesc.m_UseRaytracing → RenderPassDesc.m_UseRaytracing (engine builds RT PSO + shader table from RayGen/AnyHit/ClosestHit/Miss/ShadowMiss paths, serialized in the Shader block). "TLAS" dynamic name → GPUBufferResourceService::GetTLASBuffer() in FindResource (engine-owned, built per frame outside the graph).
- JSON: +SunShadowRT_Visibility (screen R8/UByte UAV); +SunShadowRTPass node (reads PerFrameCBuffer + TLAS + OpaquePass_RT_0/1, writes the visibility, DispatchRays; bindings bit-match the archived SunShadowRTPass: b0 set0/0, t0 TLAS set1/0, t1 RT0 set1/1, t2 RT1 set1/2, u0 vis set2/0). LightPass reads SunShadowRT_Visibility at t5 (set1/idx5). lightPassSimple.comp adds the t5 SRV + multiplies the sun term by visibility (0=shadowed, 1=lit). New RenderGraphSerializerTests_Raytracing round-trip test.
- VISUAL: Bin/gpu_output.png shows a sun-shadowed Sponza with full tonal range (teal/orange drapes, stone, column) — the prior washed direct-only look resolves once shadows add contrast. MAE 0.139→0.289 (hard direct shadows diverge from the soft-GI reference; expected, still < 0.45).
- Verified: BuildWin exit 0; SunShadowRT* + lightPassSimple compile; TestSuite incl raytracing round-trip; TestGIScene exit 0, 0 D3D12 errors, MAE 0.289; GBV loads 20 res/14 passes, SunShadowRT DispatchRays clean, only the pre-existing TASK-163 residual.
- NEXT: SSRC GI (indirect bounce — biggest remaining fidelity jump, large multi-pass) or point lights + LightCulling; then swap minimal LightPass for full lightPass.comp. [task-stays-open]


2026-06-14 — Full LightPass swap: live `LightPass` node upgraded from minimal `lightPassSimple.comp` (11 bindings) to production `lightPass.comp` (18 bindings: 3 light CBs at b0/b1/b2, 11 image SRVs at t0-t6/t8/t10/t13 + 1 buffer SRV (LightIndexList) at t9, TLAS root-SRV at t14, point sampler s1, 2 RW UAV outputs u0/u1), 15 reads, 13 prepass transitions. Bypassed `LightPassFull` scaffold deleted.
- ROOT-CAUSE FIXES (two latent bugs the bypassed scaffold hid — it never compiled live): (1) the light/GI CBs were authored `ResourceAccess: ReadWrite`, which the engine maps to an **SRV** range (t-register) — `PointLightCBuffer` became SRV t1, colliding with `OpaquePass_RT_1` SRV t1 → root-sig serialization error. Fix: light CBs now `ResourceAccess: ReadOnly` → CBV (b-register). (2) 3 dead bindings dropped — `Voxelization CBuffer` (b4), `GI CBuffer` (b5), `LightPass/Volumetric Fog` (t11): declared in `lightPass.comp` but never read (DXC dead-strips; imperative `_Archive/LightPass.cpp` never bound them), and the graph's strict imported-resource resolver aborted on the unresolvable `Voxelization CBuffer`.
- A spurious engine `RegisterSpace = m_DescriptorSetIndex` patch was tried first and REVERTED — the engine flattens all bindings into DX12 register space 0 (HLSL `register(tN)` has no space qualifier); the real fix is the CB range-type, not register space.
- FILES: `Data/ExampleProject/RenderGraph/ExampleRenderGraph.json` (LightPass rewritten, scaffold deleted); `ExampleRenderingClient_Hooks.cpp` (sampler `LightPass` → `LightPass/PointSampler`, s1); `lightPassSimple.comp` (`git rm`); `Source/TestSuite/CMakeLists.txt` (+`RenderGraphSerializerTests_Tiled.cpp` — pre-existing latent bug: on disk + extern-referenced but never compiled → clean relink failed; +`_LightPassFull.cpp`); `RenderGraphSerializerTests.cpp` (wire new test); new `RenderGraphSerializerTests_LightPassFull.cpp` (round-trip mirroring the real 18-binding node). PRIMITIVES: zero engine-side changes.
- VERIFIED: BuildWin exit 0; lightPass.comp recompiles; TestSuite unit all PASS incl new LightPass round-trip + 4 tiled (newly compiled); TestGIScene exit 0, GISponza loads, auto-terminates, 0 D3D12 errors.
- KNOWN-ACCEPTED (user-confirmed "ship it"): renders near-black. `SunShadowRT_Visibility` reads **0 everywhere** (pre-existing — stale audit capture is min=max=mean=0; kills 100% of direct sun via `l_SunDirect *= visibility`), SSRC GI indirect noisy. The deleted simple shader masked both with a flat `albedo*0.05` ambient crutch the production shader lacks. MAE 0.615 (vs simple's ambient-masked 0.289) — expected, accepted.
- NEXT (user-directed new direction): **shader unit tests** — long-missing capability. SunShadowRT-visibility=0 also filed as the real blocker to a lit image (all lighting paths are wired; will light up once it produces). [task-stays-open]
<!-- SECTION:NOTES:END -->
