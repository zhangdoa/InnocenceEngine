---
id: TASK-73
title: >-
  Window resize doesn't resize the swap chain / render targets — image stays at
  old size
status: Done
assignee: []
created_date: '2026-04-18 18:16'
updated_date: '2026-04-19 19:23'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Diagnosis

The resize infrastructure is architecturally complete: WM_SIZE → HIDService::WindowResizeCallback → RenderingConfigurationService::SetScreenResolution + FrameManagementService::Resize (sets m_needResize) → Present() checks m_needResize → ExecuteResize() → PreResize/ResizeImpl/PostResize.

**Working pieces:**
- WM_SIZE handler in WinWindowService::SendEvent wires to HIDService::WindowResizeCallback correctly
- m_needResize is atomic_bool; Present() always runs (outside the IsLoading guard)
- PreResize iterates all resizable render passes and deletes their OutputMergerTargets
- ResizeImpl releases swap chain COM refs (m_swapChainImages.clear()), calls ResizeBuffers, re-acquires back buffers
- PostResize calls CreateOutputMergerTargets + InitializeOutputMergerTargets (fires per-pass init callbacks) + OnOutputMergerTargetsCreated + recreates PSO for all resizable passes
- Per-resolution passes (TAAPass, FinalBlendPass, TiledFrustumGenerationPass, PostTAAPass) all have m_RenderTargetsInitializationFunc that recreates their textures at the new resolution via GetDefaultRenderPassDesc()
- m_Canvas in ExampleRenderingClient is updated every frame via FinalBlendPass::GetResult(), so it reflects the new texture after resize

**Missing / silent-failure risks identified:**
1. ResizeBuffers return value is NOT checked — if it fails (e.g. swap chain references not fully released), the code silently proceeds and GetSwapChainImages picks up old-sized buffers
2. No Success-level logging anywhere in the resize path — only Verbose — making it impossible to confirm the path fires in normal test runs
3. m_RenderTargetsRemovalFunc (ReleaseSwapChainImages) is set but NEVER called — dead callback — harmless but misleading

**Fix plan:**
- Add Log(Success, ...) at key resize checkpoints (Resize() flag set, ExecuteResize start, ResizeImpl swap chain resize complete, PostResize complete)
- Check and log ResizeBuffers HRESULT
- Remove dead m_RenderTargetsRemovalFunc plumbing (low priority, not blocking)

**Render thread / main thread threading model:**
- Rendering runs on TaskScheduler thread 2; WM_SIZE fires on main thread
- m_needResize is atomic_bool — ordering is safe (SetScreenResolution write happens-before m_needResize store; acquire-side read in Present sees both)
- m_IsResizing briefly true/false in WindowResizeCallback; rendering task skips frame only if it catches that window (harmless 1-frame gap)
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Changes

Three engine source files modified, all in a single commit `6239929b` on branch `ecs-overhaul`:

**Source/Engine/Services/DX12/DX12FrameManagementService.cpp** — `ResizeImpl`:
- Check HRESULT from `IDXGISwapChain::ResizeBuffers`; log Error and return false on failure. Previously ignored, so a swap-chain resize failure would leave `m_swapChainImages` pointing at the old (wrong-size) back buffers — `PostResize` would then re-bind them and the rendered image would stay at the old size.
- Replaced Verbose log with Success-level log that includes the new resolution.
- Removed two dead local variables (`l_semaphoreValue`, `l_globalSemaphore`, `l_previousFrame`) that were computed but never used.

**Source/Engine/Services/Common/FrameManagementServiceImpl.cpp** — `Resize` and `ExecuteResize`:
- Added `Log(Success, ...)` in `Resize()` (flag set) and at the start/end of `ExecuteResize()`.

**Source/Engine/Platform/WinWindow/WinWindowService.cpp** — `WM_SIZE` handler:
- Added `Log(Success, ...)` logging the new width×height when WM_SIZE fires.

## Tests run

| Test | Command | Exit code |
|------|---------|-----------|
| RenderTest tier-1 | `RenderTest.exe -offscreen -test draw_instanced` | 0 |
| Main.exe tier-2 | `Main.exe -offscreen -total_frames 10` | 0 |
| InteractiveTest full | `InteractiveTest.ps1 -Scenario full` | PASS |

## What was NOT verified

End-to-end visual correctness of the resize is not verified by the automated tests above — none of them exercise WM_SIZE. A live windowed resize requires a human (or a Win32 `SetWindowPos`/`MoveWindow` call from a separate process) to trigger it and confirm the log lines appear and the rendered image updates. The instrumentation added in this CL makes that manual verification straightforward: resize the window and look for "WinWindowService: WM_SIZE", "FrameManagementService::Resize requested", "ExecuteResize begin/complete", and "ResizeBuffers succeeded" in the log output.
<!-- SECTION:FINAL_SUMMARY:END -->
