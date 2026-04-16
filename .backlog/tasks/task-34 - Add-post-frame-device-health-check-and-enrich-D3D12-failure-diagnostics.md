---
id: TASK-34
title: Add post-frame device health check and enrich D3D12 failure diagnostics
status: Done
assignee: []
created_date: '2026-04-14 14:59'
updated_date: '2026-04-16 20:42'
labels:
  - reliability
  - diagnostics
  - DX12
  - testing
dependencies:
  - TASK-33
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Problem

The engine silently continues after GPU device removal. Exit code 0 is returned even when the device is hung/removed. All four test tiers "pass" while the GPU is dead. This was discovered during a hard debug session investigating path tracer readback failures — the root cause (TDR from heavy DispatchRays) was invisible because:

1. No proactive device health check after frame submission
2. D3D12 failure paths log generic messages without HRESULT or device-removed reason
3. Auto-test exit code doesn't reflect device health

## Three changes

### 1. Post-frame device health check
After every `Present()` or GPU wait in `FrameManagementServiceImpl::Update()`, call `GetDeviceRemovedReason()`. If device is removed, log immediately with frame number. This turns silent corruption into a loud, localizable failure.

### 2. Enrich all D3D12 failure paths
Sweep `DX12Context.cpp` and DX12 service implementations. Every `FAILED(hr)` check should log:
- The HRESULT value
- `GetDeviceRemovedReason()` result
- Resource name/size where applicable
One instance was already fixed in `CreateReadBackHeapBuffer` — apply the same pattern everywhere.

### 3. Auto-test exit code must reflect device health
The auto-test termination path in `ExampleRenderingClient` and `WorldSystem` should check device status before returning exit code 0. A "successful" run with a removed device must exit non-zero.

## Files
- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — post-frame health check
- `Source/Engine/Services/DX12/DX12Context.cpp` — failure path enrichment (partially done)
- `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` — failure paths
- `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` — failure paths
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — exit code logic
- `Source/ExampleProject/LogicClient/World.inl` — auto-test termination
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Device removal is detected and logged within 1 frame of occurrence, with frame number
- [x] #2 All FAILED(hr) checks in DX12 code log HRESULT and device-removed reason
- [x] #3 Auto-test exits non-zero when device is removed during the run
- [x] #4 RenderTest and integration test still pass on healthy GPU
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16:** DRED infrastructure now in place (commit 76a030dd). DRED is enabled before device creation, DumpDRED helper available, and breadcrumb logging added to CreateReadBackHeapBuffer failure path. The post-frame health check aspect of this task remains open.

**2026-04-16 (completion):** All four ACs verified.
- AC#1: FrameManagementServiceImpl logs `frame=N swapIndex=K` on device removed.
- AC#2: Added `LogD3D12CreateFailure` helper in `DX12Helper_Common.h` and `WaitOnFenceWithDiagnostics` on `DX12GraphicsHardwareService`. Applied across DX12Context (6 sites), DX12RenderPassResourceService (5 sites), DX12CommandListResourceService (2 sites), DX12FrameManagementService (1 site), plus the 3 fence-wait branches in `WaitOnCPU`.
- AC#3: Auto-test exits 1 on device removal (confirmed via TASK-52 TDR repro).
- AC#4: RenderTest regression passes (exit 0).

Structural follow-up filed as TASK-53 (DRED dump deduplication).
<!-- SECTION:NOTES:END -->
