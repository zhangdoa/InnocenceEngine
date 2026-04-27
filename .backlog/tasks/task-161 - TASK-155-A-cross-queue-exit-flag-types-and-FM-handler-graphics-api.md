---
id: TASK-161
title: 'TASK-155-A: cross-queue-exit flag (types + FM handler, graphics-api)'
status: Done
assignee: []
created_date: '2026-04-27 12:00'
updated_date: '2026-04-27 18:10'
labels:
  - graphics
  - dx12
  - bug
  - validation
dependencies:
  - TASK-155
parent_task_id: TASK-155
priority: medium
references:
  - Source/Engine/Common/GraphicsPrimitive.h
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-155 (F2 producer-side cross-queue exit barrier).** Owner: `graphics-api-expert`.

Engine-layer plumbing only — adds the declarative flag and the FM-service handler that emits the barrier at end of CL. No call sites flip yet (B owns producer-flag-set + consumer-deletion).

See parent TASK-155 Implementation Notes for full root-cause analysis (record-vs-execute order drift, GBV error text, F2 rationale). Cite that section rather than re-deriving.

### What this subtask delivers

1. **Type addition** in `Source/Engine/Common/GraphicsPrimitive.h`: extend `RenderPassDesc` with a `m_PostCLState` field — either `Accessibility m_PostCLState = Accessibility::ReadOnly;` (re-use existing enum, sentinel value `CrossQueueTransition`) or a dedicated `enum class CrossQueueExit { None, ToCommon };` per the agent's preference. Default value MUST be a no-op so existing render passes are unaffected.
2. **Handler** in `DX12FrameManagementService::CommandListEnd` (graphics-queue path): if the bound render pass's `m_RenderPassDesc.m_PostCLState` requests cross-queue exit, emit `ChangeRenderTargetStates(Accessibility::WriteOnly, Accessibility::CrossQueueTransition)` at end of CL — same call shape that consumers use today (parent task notes line 115).
3. **No producer/consumer call-site changes** — those land in TASK-162. After this subtask, build is green and behavior is unchanged (no pass sets the flag yet).

### Why split this way

Engine-layer types + DX12 FM service are both `graphics-api-expert` scope. Bundling avoids a cross-agent round-trip for a flag that is meaningless without its handler. Render-pass call-sites (`OpaquePass::Setup` flip + four consumer deletions) are `rendering-researcher` scope and land as TASK-162 once this subtask is committed.

### Why no GBV pass yet

After this subtask the warning is unchanged (no producer sets the flag). GBV validation is TASK-162's gate. Build green is sufficient here.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `RenderPassDesc::m_PostCLState` (or equivalent) added in `GraphicsPrimitive.h` with no-op default; cite the chosen naming in the commit message
- [x] #2 `DX12FrameManagementService::CommandListEnd` emits `ChangeRenderTargetStates(WriteOnly, CrossQueueTransition)` when the flag is set on a graphics-queue render pass; no-op otherwise
- [x] #3 Build green (RelWithDebInfo, both DX12 and VK paths still compile — VK path may stub the field if its FM service inspects `RenderPassDesc`)
- [x] #4 Engine still runs cleanly via `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` (warning count unchanged from baseline — TASK-162 will reduce it)
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
**2026-04-27 (graphics-api-expert)**: Implemented per parent TASK-155 line 113-117 F2 spec.

### Naming chosen
**Dedicated `enum class CrossQueueExit { None, ToCommon }`** in `Source/Engine/Common/GraphicsPrimitive.h`, declared at namespace-`Type` scope just above `RenderPassDesc`. New field: `RenderPassDesc::m_PostCLState = CrossQueueExit::None`.

Rejected alternative: re-using `Accessibility` with `ReadOnly` default + `CrossQueueTransition` sentinel. Reasoning — `Accessibility` is for resource-binding semantics; overloading it for "post-CL queue handoff intent" muddies the type and admits nonsensical values (`CopySource`, `ReadWrite`) the handler would need to reject. A single-purpose enum is clearer at the call site (`l_RenderPassDesc.m_PostCLState = CrossQueueExit::ToCommon` reads as exactly what it does) and the cost is one explicit map in the handler — trivially `ChangeRenderTargetStates(WriteOnly, CrossQueueTransition)` invoked when the value is `ToCommon`.

### Handler shape
`DX12FrameManagementService::CommandListEnd` (`Source/Engine/Services/DX12/DX12FrameManagementService.cpp`): when `m_PostCLState == ToCommon` AND `m_GPUEngineType == Graphics`, emit `ChangeRenderTargetStates(renderPass, commandList, Accessibility::WriteOnly, Accessibility::CrossQueueTransition)` BEFORE `Close()`. The graphics-queue guard is a defence-in-depth — `ChangeRenderTargetStates` already filters non-graphics passes at line 847, but the handler-level guard makes the intent legible without reading the helper.

`ChangeRenderTargetStates` walks all color outputs + depth-stencil (if `m_AllowDepthWrite`), invoking `TryToTransitState` per texture. The `CrossQueueTransition` accessibility forces target state to `D3D12_RESOURCE_STATE_COMMON` (line 265-266) and updates `m_CurrentState[frameIndex] = COMMON` — this is the producer-side state mutation that, in execute order, will match what the GPU actually sees, fixing the record-vs-execute drift documented in TASK-155.

### Verification
- **Build**: full sln RelWithDebInfo built green for the active configuration (DX12 path enabled, `INNO_RENDERER_VULKAN` not set). VK target has a pre-existing `GraphicsResourceService.h` include error from commit `9b81cc24` (IGraphicsService split refactor) — unrelated to TASK-161 and not produced by this change. The new field is a defaulted struct member that no VK source references; even if VK were enabled the field cannot break it. `add_library(VKGraphicsService)` does not run in this build.
- **GBV smoke** (`Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12`): exit code 1, 1 D3D12 ERROR (the pre-existing OpaquePass_RT_0 cross-queue tracker mismatch, identical to TASK-155 line 67-73), 1562 D3D12 WARNINGs (pre-existing live-object warnings on shutdown). Baseline preserved exactly — no caller flips the flag yet, so the handler is dead code by design until TASK-162.

### Files changed
- `Source/Engine/Common/GraphicsPrimitive.h` — added `enum class CrossQueueExit { None, ToCommon }` and `RenderPassDesc::m_PostCLState` field.
- `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` — extended `CommandListEnd` with the F2 producer-side cross-queue exit barrier emission.

### Hand-off to TASK-162
TASK-162 (`rendering-researcher`) is now unblocked. Single line to flip in `Source/ExampleProject/RenderingClient/OpaquePass.cpp::Setup`:
```cpp
l_RenderPassDesc.m_PostCLState = CrossQueueExit::ToCommon;
```
Plus deletion of the four consumer-side `TryToTransitState(..., WriteOnly, CrossQueueTransition)` blocks (SunShadowRTPass.cpp:173-174, RadianceCacheReprojectionPass.cpp:224-226, RadianceCacheRaytracingPass.cpp:220-223, SSAOPass.cpp:220-221). Note: `CrossQueueExit` is at `namespace Inno::Type` scope, accessible via the `using namespace Inno::Type;` at the bottom of `GraphicsPrimitive.h`.
<!-- SECTION:NOTES:END -->
