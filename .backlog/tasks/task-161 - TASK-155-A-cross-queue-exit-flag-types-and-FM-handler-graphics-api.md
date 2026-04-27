---
id: TASK-161
title: 'TASK-155-A: cross-queue-exit flag (types + FM handler, graphics-api)'
status: To Do
assignee: []
created_date: '2026-04-27 12:00'
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
- [ ] #1 `RenderPassDesc::m_PostCLState` (or equivalent) added in `GraphicsPrimitive.h` with no-op default; cite the chosen naming in the commit message
- [ ] #2 `DX12FrameManagementService::CommandListEnd` emits `ChangeRenderTargetStates(WriteOnly, CrossQueueTransition)` when the flag is set on a graphics-queue render pass; no-op otherwise
- [ ] #3 Build green (RelWithDebInfo, both DX12 and VK paths still compile — VK path may stub the field if its FM service inspects `RenderPassDesc`)
- [ ] #4 Engine still runs cleanly via `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` (warning count unchanged from baseline — TASK-162 will reduce it)
<!-- AC:END -->
