---
id: TASK-52
title: >-
  RadianceCacheReprojectionPass compute dispatch hangs GPU
  (DXGI_ERROR_DEVICE_HUNG) on GISponza auto-test
status: To Do
assignee: []
created_date: '2026-04-16 20:18'
updated_date: '2026-04-17 01:27'
labels:
  - bug
  - gpu
  - path-tracer
  - tdr
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Running `Main.exe -mode 0 -renderer 0 -offscreen -total_frames 10` reproducibly hangs the GPU shortly after GISponza loads at frame 5.

DRED output pinpoints the hang:

- `RadianceCacheReprojectionPass/Compute_CommandList` is the **first** compute list stuck — `LastCompleted=0/2` with op=46 (DISPATCH) marked `>>LAST>>`.
- All upstream compute work (`SunShadowCullingPass`, `OpaqueCullingPass`) completed (2/2).
- All downstream compute work (`RadianceCacheRaytracingPass`, filter horizontal/vertical, integration, SSAO, tiled frustum, light culling, light pass, sky, pre-TAA, TAA, luminance histogram/average, final blend) stuck pending after the reprojection dispatch.
- HRESULT = `-2005270522` = `0x887A0006` = `DXGI_ERROR_DEVICE_HUNG` (GPU TDR).
- DRED Page Fault: `VA=0x197902336`.

**Evidence of escalating stall:**

- `[Inno::Thread::ExecuteTask] Task "Rendering Execution Task" took 504323us ... 142995us ... 1562280us ...`
- Followed by `[Inno::ITask::TryToExecute] it's been too long (1000ms) since the task was alive`.

This blocks the `-capture_frame N` path past frame ~4 (RenderDoc captures after frame 5 are empty because the device is already removed).

**Shader / pass files:**

- `Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp`
- `Source/Shaders/HLSL/RadianceCacheReprojection.comp`

**Next investigation steps:**

1. Inspect the reprojection compute for unbounded loops, out-of-range texture reads, or stale SRV/UAV bindings after GISponza loads (scene reload path previously touched this — see TASK-36).
2. Rerun with `-gpu_validation` and capture the first D3D12 validation error on the reprojection dispatch.
3. Take a RenderDoc capture at frame 4 (last good frame before hang) to compare the reprojection inputs against frame-on-frame stability.
4. Check if `RadianceCacheIntegrationResult` or `RadianceCacheReprojectionResult` textures have proper creation / initial state before first dispatch.

**Related:**

- TASK-33 (DispatchRays TDR, Done) — different pass but same root category.
- TASK-36 (Radiance Cache Integration Result UAV barrier, Done) — adjacent fix.
- TASK-43 (compute early-return before GroupMemoryBarrier, To Do) — could overlap.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 — DMB experiment narrows the root cause.**

After completing TASK-54 (adding `DeviceMemoryBarrier()` to all 20 compute shaders that were missing it), the TDR **still reproduces at frame 8** (3/3 runs, exit=1). However, the DRED signature changed:

| Before DMB (initial report)                                           | After DMB                                                            |
|-----------------------------------------------------------------------|----------------------------------------------------------------------|
| Breadcrumb[2]: `RadianceCacheReprojectionPass/Compute_CommandList`    | Breadcrumb[2]: `OpaquePass/Graphics_CommandList`                     |
| Queue: ComputeCommandQueue                                            | Queue: DirectCommandQueue (Graphics)                                 |
| op=46 DISPATCH                                                        | LastCompleted=0/14 (graphics draw stream)                            |
| PageFaultVA = 0x197902336                                             | PageFaultVA = 0x198426624                                            |

**Conclusion:** Cross-queue UAV memory visibility is **ruled out** as the root cause — if that had been the bug, DMB would have fixed or at least bypassed it. The failure has moved *earlier* in the frame (OpaquePass runs before RadianceCacheReprojection), which means the underlying defect is in the graphics-pipeline state used by OpaquePass, and only surfaced later (in Reprojection) before DMB landed because the out-of-order writes happened to mask it.

**Narrowed hypotheses (in order of likelihood):**
1. **Stale descriptor after scene transition.** OpaquePass binds per-material textures and per-mesh vertex/index buffers. If GISponza loading freed a UnitTest resource whose descriptor OpaquePass still references, the first draw call that reaches that descriptor page-faults.
2. **Indirect draw command buffer miswrite.** OpaquePass consumes the buffer produced by OpaqueCullingPass via `ExecuteIndirect`. A stale entry (count, offset, or out-of-range vertex buffer handle) would fault inside the GPU's command processor.
3. **Render-target binding refers to a freed texture.** Less likely, since RTs aren't scene-scoped.

**Next investigation steps:**
- Run with `-gpu_validation` to see if GBV flags a specific binding / ExecuteIndirect argument.
- RenderDoc capture at frame 7 (last good frame) vs frame 8 to compare OpaquePass's bindings and indirect draw buffer contents.
- Grep OpaquePass and its dependencies for any lookup via raw pointer into a component that the SceneService unloading path may have freed.

TASK-54 is now Done; TASK-52 remains open with these narrowed hypotheses.
<!-- SECTION:NOTES:END -->
