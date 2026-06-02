---
id: TASK-227.1
title: Render-graph Phase 1 — migrate clean compute passes (bin-a) to DefaultKernel
status: Done
assignee:
  - code-impl
created_date: '2026-05-31 12:53'
updated_date: '2026-06-01 09:16'
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

2026-06-01 — First increment landed (NOT committed; awaiting peer review). Migrated BRDFLUTMSPass (graph node #2): 2 Image bindings — idx0 reads BRDF LUT (intra-graph, produced by BRDFLUTPass node), idx1 writes own BRDF MS LUT. First multi-binding + first has-Reads node coexisting in same graph file. Coexistence seam mirrors BRDFLUTPass: constexpr g_UseRenderGraph; imperative body verbatim under false branch; SetupFromRenderGraph only FindNode (graph already loaded by BRDFLUTPass::Setup which runs earlier). Initialize/Terminate unchanged (Initialize on adopted graph pointers — POC pattern). JSON: added BRDF MS LUT resource + BRDFLUTMSPass node. ADVISORY 1 RESOLVED but advisory prescription was WRONG and NOT applied literally: verified vs DX12FrameManagementService_Bind.cpp + DX12RenderPassResourceService_RootSignature.cpp that BindGPUResource 5th arg = ROOT-PARAMETER/layout-array index, NOT a descriptor slot; root sig maps array slot i -> root param i; m_DescriptorIndex = HLSL register, m_DescriptorSetIndex = register space, neither consulted at bind time; 'bind to m_DescriptorIndex' would bind WRONG root param when register!=array pos (SkyPass binding[1]: arr idx 1, DescriptorIndex 0). POC's BindGPUResource(...,i) was already correct. Fix: made array-index contract explicit + guard m_BoundResources.size()==m_Bindings.size(). DO-NOT-CHANGE: bind by binding position, never m_DescriptorIndex. ADVISORY 2: NO field added (BRDFLUTMSPass sets only the 8 already-serialized fields; per task, no speculative fields). External-resource-reference NOT needed yet (read is graph-owned); required by next batch — deferred. STOP cond (b): SkyPass (dynamic dispatch + deferred RT + ClearRenderTargets + ext CBuffer); LuminanceAveragePass (Buffer resources — graph Texture-only — + Update() gate + ClearRenderTargets + ext histogram); PostTAAPass (graphics-CL state-transition prepass + deferred RT + dynamic dispatch + rendering-context input); MotionBlurPass (body commented out, returns false — DEAD, skip). VERIFICATION (main-session): BuildWin exit 0; TestSuite 0 failures, RenderGraphSerializer green; GISponza smoke -total_frames 120 exit 0 0 D3D12 errors; +second run -gpu_validation exit 0 debug-layer clean; parity audit HDR flag-flip oracle: BRDF LUT MAE=0 (no regression), BRDF MS LUT MAE=0 (bit-identical); captures Build/captures/TASK-227.1/. NOT verified: only 1 pass migrated; Buffer-resource support, external-import, dynamic-dispatch kernel, deferred-RT kernel all unimplemented.

2026-06-01 — CORRECTION to the 2026-05-31 advisory #1 (it was WRONG, verified against DX12 source + confirmed by peer review). FrameManagementService::BindGPUResource's 5th arg `resourceBindingLayoutDescIndex` is the binding-layout ARRAY index, which DX12 maps directly to the root-parameter index (DX12RenderPassResourceService_RootSignature.cpp builds l_rootParameters[i] for layout slot i). `m_DescriptorIndex` is the HLSL register (BaseShaderRegister t0/u0), NOT a bind-time slot; `m_DescriptorSetIndex` (register space) is not consulted in DX12 bind/root-sig at all. So DefaultKernel binding by loop index `i` was already CORRECT; binding by m_DescriptorIndex would collide on root param 0 whenever register != array position (e.g. SkyPass binding[1]: array-slot 1, DescriptorIndex 0). LOAD-BEARING INVARIANT for the rest of Phase 1: DefaultKernel binds by binding-layout array position, never m_DescriptorIndex. Documented in DefaultKernel.cpp comment + size-mismatch guard added.

2026-06-01 — bin-(a) inventory from doc-1 §9 was OPTIMISTIC; refined against actual pass bodies this increment. Genuinely-clean DONE: BRDFLUTPass (POC), BRDFLUTMSPass. RE-CLASSIFIED out of bin-a: SkyPass + PostTAAPass = bin-b (dynamic dispatch + deferred RT); LuminanceAveragePass needs Buffer-resource support (graph is Texture-only today) + has an Update() gate; MotionBlurPass is DEAD/unwired (body commented out) — skip/delete candidate, not a migration. PREREQS the next bin-a passes need before they can migrate: (1) Buffer resource type in the graph schema/loader, (2) external-resource-import (a Reads entry naming a resource produced by a still-imperative pass resolves to the live engine resource by name, not graph-created). These two unblock most of the remaining batch.

2026-06-01 — Activated early (ahead of strict dep order) because TASK-227.1 proved bin-a is exhausted at 2 passes: the bulk of the umbrella's LOC payoff sits behind bin-b kernel infra. Groundwork already landed under .1 (commit 74215eb3): Buffer-resource type + external-resource-import in the graph schema/loader/serializer + GPUBufferResourceService::Find.

CORE bin-b problem to solve first = PER-FRAME DYNAMIC RESOURCE RESOLUTION. Nearly every clean compute pass binds the DOUBLE-BUFFERED PerFrameCBuffer at slot 0: PerFrameDataService::GetCurrentFrameBuffer() returns a different GPUBufferComponent each frame by frame-parity (registered as 'PerFrameCBuffer'/'PerFrameCBufferPrev'). The current import-by-name resolves ONCE at node creation and would pin one buffer -> stale data every other frame. Need per-frame resolution at RecordNode time (a kernel-hook or a 'dynamic import' resource flag that re-resolves each frame).

Second bin-b primitive = DYNAMIC DISPATCH: IRenderGraphKernel::ResolveDispatch override computing thread-group counts from resource dims / CPU state (e.g. OpaqueCullingPass ceil(modelCount/64), SSRC* from texture dims). Third = deferred-RT (CreateRenderTargets hook) for the SSRC/TAA/SSAO cluster.

First verifiable bin-b migrations once the above land: LuminanceAveragePass (static Dispatch(1,1,1), owns 1 buffer + reads 2 external incl. PerFrameCBuffer) and OpaqueCullingPass (dynamic dispatch + post-dispatch UAV state-tracker side effect). Recommend implementing per-frame resolution + ResolveDispatch first, migrate those two, then the SSRC cluster with deferred-RT.

ADVISORY (from .1 review, A1): RenderGraphEnumStrings AccessibilityToString round-trips only {Immutable,ReadOnly,WriteOnly,ReadWrite}; drops m_CopySource/m_CopyDestination/m_CrossQueue. No current buffer sets them, but a migrated pass needing copy/cross-queue accessibility will silently degrade through serialization — extend the table when that case arrives.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
bin-(a) clean-pass migration complete. The doc-1 §9 estimate of ~19 DefaultKernel-only passes was optimistic; against actual pass bodies the true bin-a set is just BRDFLUTPass (POC, TASK-227) + BRDFLUTMSPass (committed be9a33fe). Every other candidate is dead code (BillboardPass, MotionBlurPass, TransparentBlendPass — bodies return false/commented), runtime-disabled (PTHashGridCache* under ENABLED=false), or genuinely bin-b (SkyPass/PostTAAPass/SSRC*/OpaqueCulling/TiledFrustum/LuminanceAverage — all need per-frame double-buffered PerFrameCBuffer resolution and/or dynamic dispatch). Reclassified to TASK-227.2.

Also landed two graph-infra prereqs the bin-b batch needs (committed 74215eb3): Buffer-resource support + external-resource-import (resolve a still-imperative pass's output by name to the live engine handle, fail-loud). Pinned the load-bearing DefaultKernel invariant: bind by binding-layout array position (== root parameter), never m_DescriptorIndex (HLSL register) — verified against DX12 root-sig, corrected a wrong review advisory.

Verified across increments: build green; TestSuite 106/106 (3 RenderGraph round-trip tests); GISponza smoke exit 0, 0 D3D12 errors; BRDFLUTPass + BRDFLUTMSPass bit-identical (MAE=0) vs imperative. Commits: be9a33fe, ffa399f6, 74215eb3.
<!-- SECTION:FINAL_SUMMARY:END -->
