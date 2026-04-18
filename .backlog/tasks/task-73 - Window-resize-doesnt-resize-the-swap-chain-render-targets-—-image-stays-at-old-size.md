---
id: TASK-73
title: >-
  Window resize doesn't resize the swap chain / render targets — image stays at
  old size
status: To Do
assignee: []
created_date: '2026-04-18 18:16'
labels:
  - bug
  - window
  - swap-chain
  - rendering
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Resizing the main window leaves the rendered image at the original resolution — visible as a blocky stretched quad that keeps the previous pixel count. The resize path exists (`FrameManagementService::Resize` + a `m_needResize` flag) but evidently isn't wired end-to-end. Expected: window resize → swap chain resize → all size-dependent render targets (G-buffer, tiled frustum, TAA history, path-tracer accumulation) resize → next frame renders at the new resolution.

Investigate:
- Who sets `m_needResize` and when (WM_SIZE handler in WinWindowService? WinDXWindowSurface?).
- Does `FrameManagementService::Update` consume the flag and actually call `ResizeImpl`?
- Does `ResizeImpl` (DX12FrameManagementService) only resize the swap chain, or also iterate output merger targets owned by render passes?
- TAA / accumulation / per-resolution buffers — who reinitializes these? If they read a cached screen resolution, invalidate on resize.

Low priority for CI/offscreen flow (uses fixed resolution), high nuisance for interactive dev.
<!-- SECTION:DESCRIPTION:END -->
