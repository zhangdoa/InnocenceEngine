---
id: TASK-58
title: FrameManagementService reads RenderDoc capture config directly
status: Done
assignee: []
created_date: '2026-04-18 09:00'
updated_date: '2026-04-18 17:34'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
`FrameManagementService::Update` no longer reads `getInitConfig().captureFrame`. Added `SetPreFrameCallback` / `SetPostFrameCallback` hooks; Engine.cpp wires the RenderDoc BeginCapture/EndCapture via those hooks only when `-capture_frame N` is provided. The frame manager now owns frame pacing only; capture is a client-side debug-tool concern.
<!-- SECTION:FINAL_SUMMARY:END -->
