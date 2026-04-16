---
id: TASK-44
title: Fix _com_error exception after D3D12 device removal during GISponza rendering
status: Done
assignee: []
created_date: '2026-04-16 16:08'
updated_date: '2026-04-16 18:25'
labels:
  - reliability
  - DX12
  - TDR
  - performance
dependencies:
  - TASK-34
  - TASK-41
references:
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Symptom:** `_com_error at 0x0000000415EFE690` thrown after "D3D12: Removing Device." during GISponza rendering. Rendering Execution Task takes 901ms before device removal. Occurs on RTX 3070 Laptop GPU via VNC.

**Sequence:**
1. GISponza materials initialize (frame 5+)
2. Rendering task takes 901ms (dangerously close to 2s TDR)
3. D3D12 device removed (TDR or workload timeout)
4. Subsequent COM call on dead device throws `_com_error`

**Key context:**
- Debug layer was disabled by default (commit ff731944) but TDR still occurs — the workload itself is too heavy for the laptop GPU
- The `_com_error` is unhandled — the engine should catch device removal gracefully and shut down cleanly instead of throwing
- VNC remoting may add additional overhead or affect presentation timing
- Previous buffer overflow fix (commit 0a841cfa) resolved one TDR cause but the total GPU workload (ExecuteIndirect + DispatchRays + full post-processing) still exceeds TDR on weaker hardware

**Two distinct problems:**
1. GPU workload too heavy for laptop GPU — need performance budget or adaptive quality
2. Device removal not handled gracefully — COM exceptions propagate instead of clean shutdown

**Related:** TASK-34 (post-frame device health check), TASK-41 (COM exception safety)
<!-- SECTION:DESCRIPTION:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Root Cause
GPU memory visibility race in compute→graphics indirect draw pipeline. Compute culling shader UAV writes could retire and fence could signal before writes propagated through GPU memory hierarchy (L2 cache → global memory). Graphics queue ExecuteIndirect then read stale/uninitialized indirect draw data, causing page faults at non-deterministic addresses → TDR.

## Fix
- Added `DeviceMemoryBarrier()` after UAV writes in `sunShadowCulling.comp` and `opaqueGPUCulling.comp`
- Updated CPU-side resource state tracking in `SunShadowCullingPass.cpp` and `OpaqueCullingPass.cpp` (SetCurrentState to UAV after dispatch) so graphics passes issue correct UAV→INDIRECT_ARGUMENT barriers
- Removed dead `IsInShadowFrustum()` placeholder and redundant GPU-side validation guards

## Evidence
- `isVisible = true` (compiler optimizes to direct stores) → TDR 100%
- `objectIndex < 47` (keeps conditionals, delays retirement) → no TDR
- GPU validation (slows execution) → no TDR
- `DeviceMemoryBarrier()` after writes → no TDR 100%

## Commit
bff0931b
<!-- SECTION:FINAL_SUMMARY:END -->
