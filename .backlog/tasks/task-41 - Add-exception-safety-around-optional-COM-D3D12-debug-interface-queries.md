---
id: TASK-41
title: Add exception safety around optional COM/D3D12 debug interface queries
status: In Progress
assignee: []
created_date: '2026-04-16 16:00'
updated_date: '2026-04-16 16:11'
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
