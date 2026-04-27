---
id: TASK-155
title: 'GBV: OpaquePass_RT_0 cross-queue tracker mismatch (pre-existing)'
status: Done
assignee: []
created_date: '2026-04-27 02:00'
updated_date: '2026-04-27 19:00'
labels:
  - graphics
  - dx12
  - bug
  - validation
dependencies: []
references:
  - Source/Engine/Services/DX12/
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-138 phase 1, 2026-04-27.**

GPU-Based Validation reports a cross-queue tracker mismatch on `OpaquePass_RT_0` resource. Verified pre-existing via stash-bisect: failure reproduces against the pre-TASK-138 baseline as well, so it is not a regression introduced by the RT-shadow scaffold.

### Symptom

`-gpu_validation -total_frames N` reports a cross-queue resource state tracking violation involving `OpaquePass_RT_0`. The resource is touched on multiple queues; D3D12 GBV's tracker is reporting that the state at queue B's first use does not match the state queue A left it in (or that an explicit transition is missing between cross-queue uses).

### Why pre-existing yet not previously caught

Likely the warning exists across the whole pre-TASK-138 history but was overlooked in prior sessions because:
- GBV warnings don't fail the run — engine continues to render.
- Most validation runs were short-frame smokes that may not exercise the cross-queue path enough to flush the warning into log scrape.

### Required fix

Audit `OpaquePass_RT_0` resource lifetime across the queue boundary. Either:
- Insert the missing cross-queue state transition / fence wait at the boundary site.
- Move the consumer to the same queue as the producer if the cross-queue use is incidental.
- Fix the engine-side cross-queue tracker reconciliation (the engine may track resource state independently of GBV; if those drift, only one is wrong).

### Why medium priority

GBV warnings are not engine-fatal, but each one is a real symbol the validator surfaced — `feedback_no_dismissing_tool_noise.md` says don't learn to ignore. As more RT/compute work lands (TASK-138 phase 2 swap, future point-shadow RT extension), cross-queue state-tracking bugs will become harder to diagnose, not easier. Better to fix while the surface is small.

### Owner

`graphics-api-expert` — owns DX12 services subtree and resource state machinery.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Root cause identified — cross-queue producer/consumer pair for `OpaquePass_RT_0` enumerated, missing transition or tracker drift located (audit `ce0a5947`; bucket-(c) record-vs-execute order drift)
- [~] #2 Fix lands; `-gpu_validation -total_frames 30` — **partial/qualified**: the OpaquePass_RT_0 cross-queue tracker mismatch ERROR is fully eliminated by F2 (`d734ce91` + `00a2cf52`). Frame 30 is NOT reached cleanly because a previously-masked autocapture-readback ERROR on `Final Blend Pass Result_DefaultHeap_Texture_Frame0` (different resource, different code path: `DX12TextureResourceService::ReadTextureBackToCPU` ← `ExampleRenderingClientImpl::TryWriteAutoCapture`) is now exposed. Same class of bug (record-vs-execute `m_CurrentState` drift) but different instance. Filed as **TASK-163** for `graphics-api-expert`.
- [x] #3 Audit pass — 4 callers all clustered around OpaquePass; deleted in TASK-162; post-fix grep over `Source/` confirms zero `CrossQueueTransition` consumer call sites remain
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
**2026-04-27**: First dispatch attempt (graphics-api-expert) hit quota wall after ~50 min / 181 tool uses with no commits landed. Working tree clean post-attempt — investigation context lost. Re-dispatch recommended after quota refresh; consider tighter scoping (e.g. start with read-only audit pass, then propose the fix in a separate dispatch) to avoid the same wall.

**2026-04-27 (read-only audit, graphics-api-expert)**: Root cause identified. Bucket **(c) — engine-side cross-queue tracker drift**, with a specific mechanism: **the engine tracks resource state in CL-record-order, but the GPU sees barriers in CL-execute-order. These two orders disagree for `OpaquePass_RT_*` because every cross-queue consumer records its `CrossQueueTransition` barrier BEFORE OpaquePass records its own RT-bind barrier, while at execute time OpaquePass's CL is submitted first.**

### Exact GBV error
From `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` (run from `Bin/RelWithDebInfo/`, exits with code 1 on first error):

```
D3D12 ERROR: ID3D12CommandQueue1::ExecuteCommandLists: Using ResourceBarrier on Command List
(0x...:'OpaquePass/Graphics_CommandList'): Before state (0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT])
of resource (0x...:'OpaquePass_RT_0_DefaultHeap_Texture_Frame0') (subresource: 0) specified by
transition barrier does not match with the state (0x4: D3D12_RESOURCE_STATE_RENDER_TARGET)
specified in preceding ResourceBarrier or as InitialState
```

### Producer / consumer cross-queue boundary
- **Producer** (writes RENDER_TARGET): `OpaquePass::PrepareCommandList` → `BindRenderPassComponent` → `ChangeRenderTargetStates(ReadOnly→WriteOnly)` → `TryToTransitState(RT_0, gfxList, ReadOnly, WriteOnly)`. `Source/ExampleProject/RenderingClient/OpaquePass.cpp:157` and `Source/Engine/Services/DX12/DX12FrameManagementService.cpp:94`. RT_0 initial state from `DX12TextureResourceService.cpp:61` is `m_WriteState = RENDER_TARGET` (`DX12Helper_Texture.cpp:428`, `TextureUsage::ColorAttachment` branch). `m_CurrentState` is per-frame, sized to `m_swapChainImageCount` because `IsMultiBuffer = true` (`RenderingConfigurationService.cpp:34`).
- **Consumers (cross-queue, on Graphics CL → Compute queue)**:
  - `SunShadowRTPass.cpp:173-174` — RT_0, RT_1
  - `RadianceCacheReprojectionPass.cpp:224-226` — RT_0, RT_1, RT_3
  - `RadianceCacheRaytracingPass.cpp:220-223` — RT_0, RT_1, RT_2, RT_3
  - `SSAOPass.cpp:220-221` — RT_0, RT_1

  All call `TryToTransitState(..., m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition)`. Per `DX12FrameManagementService.cpp:265-266`, `IsCrossQueue` forces `l_newState = COMMON` and updates `m_CurrentState[frameIndex] = COMMON` (line 283).

### The drift mechanism (record vs execute order)
**`PrepareCommandList` (record) order** in `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:321-342`:
1. SunShadowRTPass (line 321) — records `RT_0: RENDER_TARGET → COMMON` on its graphics CL; engine `m_CurrentState[0] = COMMON`.
2. OpaquePass (line 325) — records `RT_0: COMMON → RENDER_TARGET` on its graphics CL (reads stale `m_CurrentState[0] = COMMON`); engine `m_CurrentState[0] = RENDER_TARGET`.
3. RadianceCacheReprojectionPass, RadianceCacheRaytracingPass, GIDenoisePass, GIFilterH, GIFilterV, SSAOPass, LightPass — further mutate `m_CurrentState`.

**`Execute` (submission) order** in `ExampleRenderingClient.cpp:472-500`:
1. OpaquePass at line 476 — its CL reaches the queue first. The CL's recorded barrier `BeforeState=COMMON` is checked against the GPU's actual state, which is the resource's **InitialState = RENDER_TARGET** (no prior barrier has run). GBV: "Before state (COMMON) does not match … RENDER_TARGET specified … as InitialState" — this is the observed error.
2. SunShadowRTPass at line 493 — its CL (recorded with `BeforeState=RENDER_TARGET, AfterState=COMMON`) would also be wrong (the OpaquePass CL would have left state in some indeterminate flavour), but the run aborts on the first ERROR before this is reported.

`DX12GraphicsHardwareService::Execute` (line 307) submits immediately via `ExecuteCommandLists(1, ...)` — there is no batching that could re-order graphics submissions.

### Why this is bucket (c) and not (a) or (b)
- (a) is wrong: a missing transition would manifest as "before state X does not match what the previous-on-this-queue CL left it in". Here the discrepancy is against `InitialState`, on frame 0 — there is no upstream cross-queue producer to insert a missing barrier from.
- (b) is wrong: the consumer queue is correctly a compute queue (RT/dispatch work), and the sample of the GBuffer textures from compute is the intended cross-queue use; moving consumers off compute defeats the point.
- (c) is correct: the engine's state-tracker (`m_CurrentState`) mutates at *record time* and is consulted by later record-time barrier emission, but submitted barriers run in *execute order*. When a consumer pass records before its producer pass on the same queue, the producer reads stale state from the consumer and emits an incorrect `BeforeState`.

The contributing detail is that `TryToTransitState`'s `sourceAccessibility` parameter is **never read** (`DX12FrameManagementService.cpp:250-287`); the function trusts `m_CurrentState[frameIndex]` exclusively. The "WriteOnly" source hint that callers pass into the cross-queue transition is documentation, not a check.

### Recommended fix
**Site**: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp:250` (`TryToTransitState(TextureComponent*, ...)`) and `:289` (`TryToTransitState(GPUBufferComponent*, ...)`).

**Option F1 — make `m_CurrentState` execute-order-correct via deferred barriers** (preferred). Stop recording `ResourceBarrier(...)` directly into the per-pass CL inside `TryToTransitState`. Instead, queue the *intent* (which resource, which target state, recording slot index) and have a frame-level resolver — invoked between `PrepareCommands` and the first `Execute` of that frame — walk the actual CL submission order, compute the correct `BeforeState` per resource per submission, and patch barriers into a small set of dedicated transition CLs that the dispatcher submits at the right boundaries. This is the same problem solved by D3D12 render-graphs (e.g. AMD GPUOpen RPS, frostbite render-graph paper); the canonical solution is "track resource state along the submission timeline, not the recording timeline." Heaviest fix, but matches the actual graph semantics.

**Option F2 — declarative producer-first invariant + explicit barrier API at the producer side** (lighter). Forbid consumers from calling `TryToTransitState(...CrossQueueTransition)` on a producer's RT. Instead, give the producer pass an "I have completed writing — drop to COMMON" hook (e.g. a `CommandListEnd_TransitionForCrossQueue` or a `RenderPassDesc::m_CrossQueueExitState`). The producer records the `RENDER_TARGET → COMMON` barrier at the *end* of its own CL, where it is contiguous with its own state mutations and `m_CurrentState` updates. Consumers on the next queue then implicitly promote from COMMON without recording any barrier on the graphics queue at all. Removes 11 lines of cross-queue transition calls in the consumer passes (`SunShadowRTPass.cpp:173-174`, `RadianceCacheReprojectionPass.cpp:224-226`, `RadianceCacheRaytracingPass.cpp:220-223`, `SSAOPass.cpp:220-221`) and centralises the contract in the producer. Lower cost, narrower correctness story, but does not generalise to arbitrary multi-producer/multi-consumer DAGs.

**Recommendation: F2 first** — the current cross-queue topology in `ExampleRenderingClient.cpp:321-342` is a single producer (OpaquePass) feeding several compute consumers, which is exactly the shape F2 is designed for. F1 is the right destination if the topology grows (multi-producer, queue ping-pong). Open a separate task (TASK-155-followup) to migrate to F1 if/when a future RT pass needs to write a resource that another graphics-queue pass later reads.

**Specific code change for F2** (no edits applied in this dispatch):
- Add `Accessibility m_PostCLState = Accessibility::ReadOnly;` (or a dedicated `enum class CrossQueueExitState`) to `RenderPassDesc` — `Source/Engine/Common/GraphicsPrimitive.h` (low-level-expert subtree).
- In `DX12FrameManagementService::CommandListEnd` for graphics-queue render passes, if `renderPass->m_RenderPassDesc.m_PostCLState == CrossQueueExit`, emit `ChangeRenderTargetStates(WriteOnly, CrossQueueTransition)` at end of CL. The producer's `m_CurrentState` is now correctly `COMMON` from the producer's own record, which is also the producer's execute order — the bug class disappears.
- In `OpaquePass::Setup` (`Source/ExampleProject/RenderingClient/OpaquePass.cpp:46`, rendering-researcher subtree), set `l_RenderPassDesc.m_PostCLState = CrossQueueExit;`.
- Delete the four call sites in the consumer passes listed above (SunShadowRTPass, RadianceCacheReprojection, RadianceCacheRaytracing, SSAOPass — all `*Pass.cpp` files, rendering-researcher subtree).

The fix touches both subtrees; dispatch should be a coordinated pair (graphics-api-expert for FM service + GraphicsPrimitive enum, rendering-researcher for pass-side changes), not parallel.

### Audit — other RT pass resources with the same bug class (enumerated, not analysed)
Same record-order-vs-execute-order pattern applies to any resource whose `TryToTransitState(..., CrossQueueTransition)` is called on the graphics CL of a pass that records *before* the producer pass and executes *after* it. Surveyed `Source/ExampleProject/RenderingClient/*.cpp` for `CrossQueueTransition` callers:
1. `RadianceCacheRaytracingPass.cpp:220-223` — OpaquePass RT_0, RT_1, RT_2, RT_3 (same producer; same bug class — would also error if it got past frame 0).
2. `RadianceCacheReprojectionPass.cpp:224-226` — OpaquePass RT_0, RT_1, RT_3 (same).
3. `SSAOPass.cpp:220-221` — OpaquePass RT_0, RT_1 (same).
4. `SunShadowRTPass.cpp:173-174` — OpaquePass RT_0, RT_1 (the trigger of the observed error).

All four sites cluster around a single producer (OpaquePass). No other pass in the rendering client exposes its outputs via `CrossQueueTransition` from a non-producer pass — the F2 fix at OpaquePass therefore covers the entire current bug surface. If/when point-shadow RT or any future graphics-queue pass becomes a cross-queue producer, the same `m_PostCLState = CrossQueueExit` declaration generalises.

### Closure for THIS dispatch
- No source code edits.
- Recommendation documented.
- Status remains In Progress — a separate dispatch (graphics-api-expert + rendering-researcher coordinated) will apply F2 and close AC #2 / #3.

### Decomposition (2026-04-27, producer)
F2 fix decomposed into two agent-scoped subtasks (per `.claude/disciplines/task-decomposition.md`; precedent: TASK-66 → TASK-147/148/149/150 sequential chain). Sequential, not parallel — types must land before any caller flips.

- **TASK-161** (`graphics-api-expert`) — engine-layer types + FM-service handler. `RenderPassDesc::m_PostCLState` field in `GraphicsPrimitive.h` + `DX12FrameManagementService::CommandListEnd` consumer of the flag. No call sites flip; build green; GBV unchanged.
- **TASK-162** (`rendering-researcher`) — depends on TASK-161. `OpaquePass::Setup` flips the flag + four consumer-side `CrossQueueTransition` calls deleted (SunShadowRTPass, RadianceCacheReprojectionPass, RadianceCacheRaytracingPass, SSAOPass). Owns the GBV-clean validation gate (parent AC #2) and audit-coverage confirmation (parent AC #3).

Dispatch order: TASK-161 → commit → TASK-162. Each commitable in a single dispatch with build green at boundary; only TASK-162 needs the GBV pass.

### Final Summary (2026-04-27, producer-close)

F2 fix landed across two commits:
- `d734ce91` (TASK-161, graphics-api-expert) — `enum class CrossQueueExit { None, ToCommon }` + `RenderPassDesc::m_PostCLState` field in `GraphicsPrimitive.h`; `DX12FrameManagementService::CommandListEnd` handler emits `ChangeRenderTargetStates(WriteOnly, CrossQueueTransition)` at end of CL when flag is set on a graphics-queue pass.
- `00a2cf52` (TASK-162, rendering-researcher) — `OpaquePass::Setup` flips the flag; deletes 11 consumer-side `CrossQueueTransition` calls across 4 passes (SunShadowRTPass, RadianceCacheReprojectionPass, RadianceCacheRaytracingPass, SSAOPass).

**Outcome on parent ACs**:
- AC#1 root-cause: closed by audit `ce0a5947`. Bucket (c) — engine `m_CurrentState` mutates in record order; D3D12 sees barriers in execute order; the two disagree when a consumer pass records its CL before its producer pass on the same queue.
- AC#2 GBV clean: closed for the OpaquePass cross-queue class; the tracker-mismatch ERROR on `OpaquePass_RT_0` is fully eliminated. The run does not reach frame 30 cleanly because a previously-masked autocapture-readback ERROR on `Final Blend Pass Result` is now exposed. Same bug class (record-vs-execute m_CurrentState drift), different instance. Filed as **TASK-163** (graphics-api-expert, medium).
- AC#3 audit: closed by TASK-162's grep — zero consumer call sites remain post-fix.

**Class-vs-instance**: F2 is a per-pass declarative flag that closes one instance of the wider class. The canonical fix is **F1 (graph-aware deferred-barrier resolver)** — track resource state along the *submission* timeline, not the *recording* timeline; same problem solved by render-graph papers (Frostbite, AMD GPUOpen RPS). F1 is multi-month scope and not in flight; deferring until point-shadow RT or another graphics-queue cross-queue producer makes the topology multi-producer.

**Masking note (parallels TASK-160)**: GBV's `_Exit(1)` on first ERROR meant the autocapture ERROR was invisible until the OpaquePass ERROR was eliminated. Same shape as TASK-160 (LogService Error early-exit hides downstream batch failures). Surfacing this as a class to the team via the retrospective; no separate task because the GBV `_Exit(1)` is a D3D12 runtime contract we do not own.

**Logs**: `Build/task162_baseline_gbv.log`, `Build/task162_gbv.log`, `Build/task162_postfix_gbv.log`.
<!-- SECTION:NOTES:END -->
