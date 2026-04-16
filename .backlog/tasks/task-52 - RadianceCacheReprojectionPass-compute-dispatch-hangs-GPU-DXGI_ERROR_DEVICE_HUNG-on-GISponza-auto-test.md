---
id: TASK-52
title: >-
  RadianceCacheReprojectionPass compute dispatch hangs GPU
  (DXGI_ERROR_DEVICE_HUNG) on GISponza auto-test
status: To Do
assignee: []
created_date: '2026-04-16 20:18'
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
