---
id: TASK-162
title: 'TASK-155-B: OpaquePass flag set + consumer call deletion (rendering-researcher)'
status: To Do
assignee: []
created_date: '2026-04-27 12:00'
labels:
  - graphics
  - dx12
  - bug
  - validation
dependencies:
  - TASK-161
parent_task_id: TASK-155
priority: medium
references:
  - Source/ExampleProject/RenderingClient/OpaquePass.cpp
  - Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp
  - Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp
  - Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp
  - Source/ExampleProject/RenderingClient/SSAOPass.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask B of TASK-155 (F2 producer-side cross-queue exit barrier).** Owner: `rendering-researcher`.

Render-pass-layer call-site changes only. Depends on TASK-161 landing first (the flag + FM handler must exist before any pass sets it). After this subtask, `OpaquePass` owns its cross-queue exit barrier and the four consumer-side `CrossQueueTransition` calls are gone.

See parent TASK-155 Implementation Notes for root-cause / GBV-error / F2 rationale.

### What this subtask delivers

1. **Set the flag** in `OpaquePass::Setup` (`Source/ExampleProject/RenderingClient/OpaquePass.cpp:46`): set `l_RenderPassDesc.m_PostCLState = <CrossQueueExit-sentinel>` using whatever name TASK-161 chose. Cite TASK-161 in the commit body for the naming source.
2. **Delete four consumer call sites** (each is `TryToTransitState(..., m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition)`):
   - `SunShadowRTPass.cpp:173-174` — RT_0, RT_1
   - `RadianceCacheReprojectionPass.cpp:224-226` — RT_0, RT_1, RT_3
   - `RadianceCacheRaytracingPass.cpp:220-223` — RT_0, RT_1, RT_2, RT_3
   - `SSAOPass.cpp:220-221` — RT_0, RT_1
3. **Validate**: `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` — the `OpaquePass_RT_0` cross-queue tracker mismatch error must be gone. Engine should reach frame 30 cleanly with zero `D3D12 ERROR` / `D3D12 WARNING` lines from the cross-queue tracker.
4. **Audit-pass closure**: TASK-155 AC #3 (audit pass on other RT pass resources for the same bug class) is already enumerated in the parent's Implementation Notes — all four sites cluster around OpaquePass and are deleted by this subtask. Confirm in the closure note that no other `CrossQueueTransition` callsite outside the four exists; if a new caller has appeared since the audit, file a follow-up.

### Sequencing

Hard-blocks on TASK-161 — the flag has to exist as a type and the FM handler has to read it before flipping any caller. Do NOT dispatch in parallel with TASK-161; the producer-side write would hit a missing field.

### Validation gate

This subtask owns the GBV pass. AC #2 of parent TASK-155 closes here.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `OpaquePass::Setup` sets `m_PostCLState` to the cross-queue-exit sentinel introduced in TASK-161
- [ ] #2 Four consumer call sites listed above are deleted (SunShadowRTPass / RadianceCacheReprojectionPass / RadianceCacheRaytracingPass / SSAOPass)
- [ ] #3 Build green (RelWithDebInfo, DX12)
- [ ] #4 `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` — zero `D3D12 ERROR` and zero cross-queue `D3D12 WARNING` lines; engine reaches frame 30 cleanly
- [ ] #5 Confirm no `CrossQueueTransition` caller exists outside the four deleted sites; if any has appeared since the parent-task audit, file a follow-up task and link
<!-- AC:END -->
