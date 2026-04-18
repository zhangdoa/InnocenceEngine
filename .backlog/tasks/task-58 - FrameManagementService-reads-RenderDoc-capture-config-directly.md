---
id: TASK-58
title: FrameManagementService reads RenderDoc capture config directly
status: To Do
assignee: []
created_date: '2026-04-18 09:00'
labels:
  - architecture
  - rendering-client
  - renderdoc
  - fix-at-right-layer
dependencies: []
references:
  - Source/Engine/Services/Common/FrameManagementServiceImpl.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`FrameManagementService::Update` peeks at `g_Engine->getInitConfig().captureFrame` to decide when to fire `BeginCapture`/`EndCapture`. The frame manager should not know about RenderDoc or the command-line flag — it owns frame pacing, not debug-tool triggers.

Resolve by having the rendering client (or a dedicated `CaptureController`) register a per-frame callback that the frame manager invokes. The callback owner decides when to request a capture; the frame manager just calls the hook.
<!-- SECTION:DESCRIPTION:END -->
