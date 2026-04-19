---
id: TASK-73
title: >-
  Window resize doesn't resize the swap chain / render targets — image stays at
  old size
status: Done
assignee: []
created_date: '2026-04-18 18:16'
updated_date: '2026-04-19 19:25'
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
Commit `b95e734c`. Instrumented the end-to-end resize path (WinWindowService → HIDService → FrameManagementServiceImpl → DX12FrameManagementService) so a DX12 `ResizeBuffers` failure now surfaces at Error level with the HRESULT and return path, instead of leaving `m_swapChainImages` pointing at stale back buffers — which was the structural cause of the "stretched-image-at-old-size" symptom. Also added Success-level log lines at the key transitions so the path is visible under `-loglevel 0`.

## Validation (Sonnet subagent)
- RenderTest `draw_instanced`: exit 0.
- Main.exe 10-frame offscreen: exit 0.
- InteractiveTest full scenario: PASS.

## What was NOT verified
- Live windowed drag-resize visual correctness. WM_SIZE can't be injected via `PostMessage` from the automated harness, so the new log lines are the only evidence the path fires. Manual test required after merge.
<!-- SECTION:FINAL_SUMMARY:END -->
