---
id: TASK-162
title: 'TASK-155-B: OpaquePass flag set + consumer call deletion (rendering-researcher)'
status: Done
assignee: []
created_date: '2026-04-27 12:00'
updated_date: '2026-04-27 18:25'
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
- [x] #1 `OpaquePass::Setup` sets `m_PostCLState` to the cross-queue-exit sentinel introduced in TASK-161
- [x] #2 Four consumer call sites listed above are deleted (SunShadowRTPass / RadianceCacheReprojectionPass / RadianceCacheRaytracingPass / SSAOPass)
- [x] #3 Build green (RelWithDebInfo, DX12)
- [~] #4 `OpaquePass_RT_*` cross-queue tracker mismatch ERROR is eliminated. A previously-masked, unrelated engine-layer ERROR on `Final Blend Pass Result` via `ReadTextureBackToCPU_Transition` (autocapture path) is now exposed and prevents the frame-30 clean exit; surfaced for graphics-api-expert per task invariant #3 (engine-layer issue out of scope here)
- [x] #5 Zero `CrossQueueTransition` callers remain outside the deletion targets — grep clean across `Source/`
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:NOTES:BEGIN -->
**2026-04-27 (rendering-researcher)**: Single-CL flag flip + four consumer deletions per the TASK-161 hand-off.

### Files changed
- `Source/ExampleProject/RenderingClient/OpaquePass.cpp` — added `l_RenderPassDesc.m_PostCLState = CrossQueueExit::ToCommon;` in `Setup`. Type reachable as a bare identifier via the `using namespace Inno::Type;` at the bottom of `GraphicsPrimitive.h`, transitively included through `RenderPassComponent.h` → `IRenderPass.h` → `OpaquePass.h`.
- `Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp` — deleted RT_0/RT_1 `TryToTransitState(..., WriteOnly, CrossQueueTransition)` calls (formerly lines 173-174). The five-line comment header above them documented the now-deleted transitions and was deleted with them per `comment-discipline.md` (comments document present state, not history).
- `Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp` — deleted RT_0/RT_1/RT_3 calls (formerly lines 224-226) and the three-line "transition OpaquePass RTs to COMMON" comment block.
- `Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp` — deleted RT_0/RT_1/RT_2/RT_3 calls (formerly lines 220-223) and the two-line "Use graphics CL to transition resources to COMMON" comment block.
- `Source/ExampleProject/RenderingClient/SSAOPass.cpp` — deleted RT_0/RT_1 calls (formerly lines 220-221).

### Verification

**AC#1, #2, #3, #5** — direct: file edits + post-edit grep `CrossQueueTransition` over `Source/` returns only the type definition (`Accessibility::CrossQueueTransition` static in `GraphicsPrimitive.h`/`Accessibility.cpp`) and the producer-side emit in `DX12FrameManagementService.cpp` (line 523, the TASK-161 handler). Zero consumer call sites. Build green via `cmake --build Build --config RelWithDebInfo --target Main`.

**AC#4 — partial / qualified pass.** Ran `Bin/RelWithDebInfo/Main.exe -gpu_validation -total_frames 30 -renderer DX12` against three states; logs preserved under `Build/`:

| State | First D3D12 ERROR | Cross-queue tracker error | Reaches frame 30 |
|------|------|------|------|
| Baseline d734ce91 (TASK-161 head, pre-TASK-162) — `task162_baseline_gbv.log` | `OpaquePass/Graphics_CommandList`: `OpaquePass_RT_0` Before-state COMMON vs preceding-state RENDER_TARGET | YES | No (fatal exit) |
| Post-TASK-162 (this CL) — `task162_postfix_gbv.log` | `ReadTextureBackToCPU_Transition`: `Final Blend Pass Result_DefaultHeap_Texture_Frame0` Before-state `NON_PIXEL_SHADER_RESOURCE\|PIXEL_SHADER_RESOURCE\|COPY_SOURCE` vs preceding-state UNORDERED_ACCESS, originating in `DX12TextureResourceService::ReadTextureBackToCPU` ← `ExampleRenderingClientImpl::TryWriteAutoCapture` | NO (eliminated) | No (different fatal exit point) |

The TASK-155 root error (`OpaquePass_RT_*` cross-queue tracker mismatch) is fully eliminated by F2. The remaining ERROR is unrelated and engine-layer:
- **Resource**: `Final Blend Pass Result` (not an OpaquePass RT, not cross-queue).
- **Site**: `DX12TextureResourceService::ReadTextureBackToCPU` recording a transition CL ("ReadTextureBackToCPU_Transition") with a stale before-state vs the texture's actual GPU state at execute time.
- **Trigger**: `ExampleRenderingClientImpl::TryWriteAutoCapture` (one-shot autocapture invoked at end of test run; previously masked because the cross-queue ERROR aborted the run earlier).
- **Scope**: engine-layer (`Source/Engine/Services/DX12/DX12TextureResourceService.cpp` + the autocapture readback path) — outside `rendering-researcher`'s pass-call-site scope per task invariant #3.

Per the task spec ("If the fix surfaces an engine-layer issue, surface to graphics-api-expert rather than chase"), surfacing rather than expanding scope. **Recommend producer file a `graphics-api-expert`-owned follow-up**: `DX12TextureResourceService::ReadTextureBackToCPU` records a transition CL with a stale before-state for the autocapture path on FinalBlendPass result. Likely root cause: the `m_CurrentState` tracker for `Final Blend Pass Result_DefaultHeap_Texture_Frame0` is not updated to `UNORDERED_ACCESS` after FinalBlendPass writes to it (compute UAV write), so the readback path's "transition from current state to COPY_SOURCE" sees the wrong before-state. Reproduces deterministically on this CL with `-gpu_validation -total_frames 30 -renderer DX12`. Three log artefacts under `Build/`: `task162_baseline_gbv.log`, `task162_gbv.log`, `task162_postfix_gbv.log` (the two post-fix logs are equivalent — second was a re-run after stash-pop rebuild, both show the same `Final Blend Pass Result` ERROR).

### Audit grep (AC#5) — clean

```
$ grep -rn 'CrossQueueTransition' Source/
Source/Engine/Common/Accessibility.cpp:11: ... static definition
Source/Engine/Common/GraphicsPrimitive.h:82: ... static declaration
Source/Engine/Services/DX12/DX12FrameManagementService.cpp:263: ... documentation comment
Source/Engine/Services/DX12/DX12FrameManagementService.cpp:523: ... TASK-161 producer-side handler emit
```

No consumer call sites remain. No new `CrossQueueTransition` caller has appeared since the parent-task audit.

### TASK-155 closure note
Parent TASK-155 AC#2 (the GBV-validation gate) is **resolved for the OpaquePass cross-queue class**. The newly-exposed `Final Blend Pass Result` autocapture-readback ERROR is a separate root cause that was masked by the OpaquePass error and now requires its own follow-up. Producer should: (a) close TASK-155 as Done with a closure note pointing here for the F2 outcome, and (b) file the autocapture-readback follow-up against `graphics-api-expert`.
<!-- SECTION:NOTES:END -->
