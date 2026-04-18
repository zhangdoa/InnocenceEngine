---
id: TASK-57
title: DX12GraphicsHardwareService hardcodes project-specific capture path
status: To Do
assignee: []
created_date: '2026-04-18 09:00'
labels:
  - architecture
  - dx12
  - renderdoc
  - fix-at-right-layer
dependencies: []
references:
  - Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`TryLoadRenderDocAPI` calls `SetCaptureFilePathTemplate("C:/GitRepo/InnocenceEngine/Build/captures/frame")` and `LoadLibraryA("C:/Program Files/RenderDoc/renderdoc.dll")` directly — a low-level backend embedding project paths and installation assumptions. Violates the "fix at the right layer" principle in CLAUDE.md.

Resolve by routing through an engine-level service (e.g. `IOService::getBuildDirectory()` + a capture subdirectory, plus a config entry for the RenderDoc install path override). The DX12 service should take a string it received, not build one from string literals.
<!-- SECTION:DESCRIPTION:END -->
