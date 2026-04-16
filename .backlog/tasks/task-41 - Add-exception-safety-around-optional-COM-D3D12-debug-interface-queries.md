---
id: TASK-41
title: Add exception safety around optional COM/D3D12 debug interface queries
status: Done
assignee: []
created_date: '2026-04-16 16:00'
updated_date: '2026-04-16 20:42'
labels:
  - reliability
  - DX12
  - COM-safety
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
  - Source/Engine/Services/DX12/DX12Context.cpp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?** `D3D12GetDebugInterface` and `DXGIGetDebugInterface1` are called assuming they return HRESULT gracefully, but on some configurations (missing debug runtime, VNC remoting, restricted environments) these COM calls throw `_com_error` exceptions instead.

**What structural weakness allowed it?** No exception safety around optional diagnostic COM interfaces. The DRED initialization, debug callback setup, and PIX detection all use COM QueryInterface patterns that can throw.

**What improvement?** Wrap all optional COM interface queries (DRED, debug layer, PIX, info queue) in try-catch blocks. A missing debug interface should log a warning and continue, never crash. This is the likely cause of `_com_error` exceptions seen on VNC-remoted sessions.

Files: `DX12GraphicsHardwareService.cpp` (CreateDebugCallback, CreatePhysicalDevices DRED block), `DX12Context.cpp` (DRED dump in CreateReadBackHeapBuffer).
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16 (completion):** Scope satisfied by prior commits (76a030dd, e4ff5f71). Try/catch coverage verified for every optional COM interface query:
- CreateDebugCallback — D3D12GetDebugInterface, QueryInterface, DXGIGetDebugInterface1 (PIX).
- CreatePhysicalDevices DRED block — ID3D12DeviceRemovedExtendedDataSettings1.
- CreatePhysicalDevices info queue — ID3D12InfoQueue/InfoQueue1, RegisterMessageCallback.
- HasGPUError / DumpGPUDiagnostics — GetDeviceRemovedReason.
- DumpDRED static helper — DRED QueryInterface + breadcrumb/page-fault outputs.
- DX12Context::CreateReadBackHeapBuffer DRED post-mortem — same DRED chain.

Confirmed by current integration run (TASK-52 repro): DRED dump fires cleanly on TDR with no unhandled `_com_error` escape.
<!-- SECTION:NOTES:END -->
