---
id: TASK-222
title: 'FinalBlend readback transition blocks PNG output during -gpu_validation runs'
status: To Do
assignee: []
created_date: '2026-05-10'
labels:
  - rendering
  - bug
  - readback
  - validation
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
At engine shutdown, `DX12TextureResourceService::ReadTextureBackToCPU` emits a `ResourceBarrier` on `Final Blend Pass Result_DefaultHeap_Texture_Frame2` from `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` (0x8) to a state the engine state-tracker recorded as `D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE | COPY_SOURCE` (0x8C0). The mismatch fires as a D3D12 ERROR under `-gpu_validation`:

```
D3D12 ERROR: ID3D12CommandQueue1::ExecuteCommandLists: Using ResourceBarrier on
Command List ('ReadTextureBackToCPU_Transition'): Before state (0x8: UAV) of resource
('Final Blend Pass Result_DefaultHeap_Texture_Frame2') (subresource: 0) specified by
transition barrier does not match with the state (0x8C0: NON_PIXEL_SHADER_RESOURCE|
PIXEL_SHADER_RESOURCE|COPY_SOURCE) specified in preceding ResourceBarrier or as InitialState
```

Stack: `Inno::DX12GraphicsHardwareService::Execute` ← `DX12TextureResourceService::ReadTextureBackToCPU` ← `WriteCaptureToFile` ← `TryWriteAutoCapture` ← `FinalizeGPUResults` ← `Engine::Terminate`.

## Pre-existing scope

Confirmed pre-existing per regression bisect (TASK-77.4 CL-3 work, 2026-05-09): error fires identically at commit `53331e1f` (CL-2, before any CL-3 main work). Not a CL-3 regression.

## Why this matters now (low-priority but worth tracking)

The error is shutdown-only and has no runtime / per-frame impact. It does, however, **silently block PNG capture output** when the engine is launched with `-gpu_validation`. The auto-capture path runs after the offending barrier; the validation-layer fatal-exit code prevents the PNG from being flushed to disk.

Practical impact: visual-validation runs (per skill `visual-validation`, every rendering CL needs Layer-1 captures) cannot combine `-gpu_validation` and capture in a single launch. Workflow becomes: one launch with `-gpu_validation` for the smoke check, a separate launch without it for the capture. Doubles the wall-clock cost of visual-validation discipline.

Surfaced repeatedly across multiple CLs in TASK-77.4 closure work; filed here so the dispatcher pattern of "two launches per validation cycle" gets cleaned up.

## Likely root cause (unverified)

Engine state-tracker records the FinalBlend pass result texture's state as `0x8C0` (a composite of three SRV/COPY-source flags) somewhere in the rendering loop. The actual D3D12 runtime sees the resource at `0x8` (UAV) when shutdown fires the readback transition. Likely candidates:
- A rendering pass writes to the texture as UAV but doesn't notify the state-tracker.
- The state-tracker records the "intended" state for an upcoming SRV bind that never happens because shutdown intervenes.
- A frame-loop barrier sequence aliases multiple states and the state-tracker keeps the union rather than the post-frame final.

## Deliverables

- Identify the engine state-tracker site that records `0x8C0` for the FinalBlend result.
- Reconcile with the actual D3D12 state at shutdown (either fix the state-tracker, or insert a transition before the readback that brings the actual state into agreement with the recorded state).
- Re-run with `-gpu_validation -total_frames 30` and confirm zero D3D12 errors at shutdown + PNG output appears in `Build/captures/`.

## Cross-references

- TASK-77.4 CL-3 — the CL during which this was repeatedly surfaced; not introduced by it (regression-bisected to pre-CL-2 territory).
- Skill `visual-validation` — the discipline this bug obstructs.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Engine launch with `-gpu_validation -total_frames 30` produces zero D3D12 errors at shutdown — log quoted in summary
- [ ] #2 Same launch produces a `Build/captures/` PNG — path quoted in summary
- [ ] #3 Pre-existing integration tests covering rendering / shutdown remain green
- [ ] #4 Final summary lists what was NOT verified
<!-- DOD:END -->
