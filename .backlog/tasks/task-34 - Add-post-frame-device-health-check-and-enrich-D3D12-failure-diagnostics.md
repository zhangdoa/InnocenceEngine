---
id: TASK-34
title: Add post-frame device health check and enrich D3D12 failure diagnostics
status: To Do
assignee: []
created_date: '2026-04-14 14:59'
updated_date: '2026-04-14 15:30'
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
- [ ] #1 Device removal is detected and logged within 1 frame of occurrence, with frame number
- [ ] #2 All FAILED(hr) checks in DX12 code log HRESULT and device-removed reason
- [ ] #3 Auto-test exits non-zero when device is removed during the run
- [ ] #4 RenderTest and integration test still pass on healthy GPU
<!-- AC:END -->
