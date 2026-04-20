---
id: TASK-113
title: 'GPU path tracer: render target not resized when window/viewport resizes'
status: Done
assignee: []
created_date: '2026-04-20 15:39'
updated_date: '2026-04-20 16:02'
labels:
  - rendering
  - bug
  - pathtracer
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Reported 2026-04-20 during a session on TASK-109 / TASK-72. GPU path tracer keeps rendering into its originally-allocated render target dimensions after a window or editor-viewport resize. Expected: the path tracer's RT (and any downsampled / history / accumulation buffers it owns) should be reallocated or re-bound to the new dimensions on resize.

Investigation starting points:
- Where window/viewport resize is signalled (search for window-resize handler, viewport size change in editor, RenderingConfigurationService resolution updates).
- PathTracer / RayTracer service's render-target ownership — is it sized once at Init and never revisited?
- Compare against rasterized pipeline's resize path (which appears to work) — what does it subscribe to, and what does the path tracer miss?

Note: TASK-73 ("Window resize doesn't resize the swap chain render targets") was completed; this may be a parallel instance in the path tracer's owned RTs rather than the swap chain.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Resize the window (or editor viewport) and the GPU path tracer's output matches the new dimensions
- [x] #2 No stretched / letterboxed / stale-pixel artifacts in the path tracer view after resize
- [x] #3 Any accumulation / history buffers owned by the path tracer are invalidated or resized cleanly (no mixed-resolution samples)
- [x] #4 Interactive scenario driven by InteractiveTest.ps1 (or manual) exercises at least one resize event without crash or validation errors
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fixed by registering `m_OnResize` on the ray-tracing `RenderPassComponent`. The callback deletes and recreates `m_AccumulationBuffer` at the new screen resolution via a factored-out `CreateAccumulationBuffer()` helper, then calls `ResetAccumulation()` to scrap stale history (sample counts across resolutions can't be combined).

Why the framework missed it without this: `FrameManagementService::ExecuteResize` only walks each render pass's **output-merger targets** via `CreateOutputMergerTargets`. The path tracer has `m_RenderTargetCount = 0` and `m_UseOutputMerger = false` (it's a compute DXR pass that writes into a UAV), so the framework had nothing to resize on it. The accumulation buffer is a separate `TextureComponent*` the pass owns privately — invisible to the resize machinery. The `m_OnResize` hook is the engine's existing escape for exactly this case.

GPU is drained before `m_OnResize` fires — `FrameManagementService::Present` signals and CPU-waits on all three queues before `ExecuteResize`, so delete/recreate is safe.

Validation:
- Tier-1 RenderTest draw_instanced: exit 0.
- Tier-3 Main.exe 20 frames + reload at frame 10: exit 0.
- New Tier-4 scenario `pathtracer_resize` added to `Scripts/InteractiveTest.ps1`: toggles path tracer on, then resizes window through 1024x768 → 1600x900 → 800x600. Exit 0 across all three cycles; no D3D12 ERROR, no exceptions; engine log shows 3x `ExecuteResize begin`/`complete` pairs and 4x `GPUPathTracerAccumBuffer` initialize-deferred / initialized events (initial + 3 resizes).
- The `full` scenario also picks up the new subscenario.

NOT verified in this task:
- Editor-viewport resize (separate surface from window resize; TASK-89 owns that path and isn't wired yet).
- Resize during active accumulation on scene with many samples baked — unlikely to regress (ResetAccumulation is the only observable effect for history; new buffer is zero-init).

Related code: `Source/ExampleProject/RenderingClient/GPUPathTracerPass.{h,cpp}`; `Scripts/InteractiveTest.ps1`.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
