---
id: TASK-163
title: 'GBV: Final Blend Pass Result readback state-drift (autocapture path)'
status: To Do
assignee: []
created_date: '2026-04-27 19:00'
labels:
  - graphics
  - dx12
  - bug
  - validation
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12TextureResourceService.cpp
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by TASK-155 (`00a2cf52`).** Previously masked by the OpaquePass cross-queue tracker mismatch ERROR (GBV `_Exit(1)` aborted the run earlier). With TASK-155's F2 fix landed, this ERROR is now the first GBV ERROR in `-gpu_validation -total_frames 30 -renderer DX12`.

### Symptom

```
D3D12 ERROR: ID3D12CommandQueue::ExecuteCommandLists on
'ReadTextureBackToCPU_Transition' command list:
Before state of resource (0x...:'Final Blend Pass Result_DefaultHeap_Texture_Frame0')
specified by transition barrier:
  NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE | COPY_SOURCE
does not match the state the resource was actually in:
  UNORDERED_ACCESS
```

### Resource / call site

- **Resource**: `Final Blend Pass Result_DefaultHeap_Texture_Frame0`.
- **Recording site**: `DX12TextureResourceService::ReadTextureBackToCPU` records the `ReadTextureBackToCPU_Transition` CL with a stale `BeforeState`.
- **Trigger**: `ExampleRenderingClientImpl::TryWriteAutoCapture` (`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp:858, 915-927`) — one-shot autocapture invoked at end of test run.

### Likely root cause

`m_CurrentState` tracker for `Final Blend Pass Result_DefaultHeap_Texture_Frame0` is not updated to `UNORDERED_ACCESS` after `FinalBlendPass` writes to it (compute UAV write). The readback path's "transition from current state to `COPY_SOURCE`" then reads the wrong before-state.

This is the same bug **class** as TASK-155 (record-vs-execute `m_CurrentState` drift in the engine-layer state tracker), but a different **instance**:
- TASK-155 was record-order-vs-execute-order across multiple graphics CLs in the same frame (consumer recorded before producer; same queue).
- TASK-163 is missing-state-update on a UAV write — `m_CurrentState` is never advanced to `UNORDERED_ACCESS` after the compute write, so the next consumer (the readback) sees a stale snapshot.

### Why pre-existing

The autocapture path runs only on the last frame of `-total_frames N`. The OpaquePass GBV ERROR (TASK-155) fired on frame 0, so autocapture was never reached. With F2 landed, frame 30 runs to completion and the autocapture ERROR is now the first ERROR.

### Bisect evidence

Stash-bisect performed during TASK-162: stashing the consumer deletions against `d734ce91` reproduces the OpaquePass cross-queue ERROR (frame 0); applying TASK-162 exposes this `Final Blend Pass Result` ERROR (frame 30 readback). The autocapture ERROR is **not** introduced by TASK-162 — it was always there, just hidden.

### Why medium priority

GBV ERROR — same severity bracket as TASK-155. Real symbol the validator surfaced; per `feedback_no_dismissing_tool_noise.md`, do not learn to ignore. Not engine-fatal (autocapture is opt-in), but every additional cross-queue/UAV-readback path landed without fixing this will compound the diagnostic surface — same reasoning as TASK-155's medium bid.

### Owner

`graphics-api-expert` — engine-layer DX12 state tracker. Out of `rendering-researcher` scope (per their task invariant #3, surface engine-layer issues).

### Investigation pointers

- Where does `FinalBlendPass` advance `m_CurrentState` for its compute output? If the compute UAV-write path skips `m_CurrentState` mutation, that is the gap.
- Does `DX12TextureResourceService::ReadTextureBackToCPU` consult `m_CurrentState` or hard-code a specific `BeforeState`? If hard-coded, the autocapture path is incompatible with anything but a specific producer state.
- Is there a parallel issue in the VK path? (likely yes, but VK is not currently in the build; not a blocker.)

### Logs

`Build/task162_postfix_gbv.log` (post-TASK-162, pre-TASK-163) shows the ERROR text deterministically.

### Relation to TASK-155

Same class of bug (engine-layer `m_CurrentState` drift); different instance (UAV-write missing-update vs cross-queue record-vs-execute). The canonical fix for the entire class is **F1 (graph-aware deferred-barrier resolver)** documented in TASK-155 — multi-month scope, not in flight. TASK-163 should be solvable as a localised fix at the FinalBlendPass write site or the readback before-state derivation, similar to TASK-155's F2.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Root cause confirmed — locate the missing `m_CurrentState` update or stale before-state derivation
- [ ] #2 Fix lands; `-gpu_validation -total_frames 30 -renderer DX12` reaches frame 30 with zero `D3D12 ERROR` lines (autocapture path included)
- [ ] #3 Audit pass — survey other UAV-write paths that feed a readback or cross-queue consumer for the same missing-update pattern
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
<!-- SECTION:NOTES:END -->
