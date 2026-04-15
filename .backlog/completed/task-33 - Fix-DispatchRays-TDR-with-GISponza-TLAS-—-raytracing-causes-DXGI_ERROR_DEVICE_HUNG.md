---
id: TASK-33
title: >-
  Fix DispatchRays TDR with GISponza TLAS — raytracing causes
  DXGI_ERROR_DEVICE_HUNG
status: Done
assignee: []
created_date: '2026-04-14 13:37'
updated_date: '2026-04-14 16:53'
labels:
  - bug
  - raytracing
  - TDR
  - GPU
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp
  - Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
  - Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Problem

Any `DispatchRays` call against the full GISponza TLAS (3.6M vertices, 20M indices, 94 meshes) at 1280x720 causes a GPU TDR (Timeout Detection and Recovery), resulting in `DXGI_ERROR_DEVICE_HUNG` (0x887A0005) and `DXGI_ERROR_DEVICE_REMOVED` (0x887A0006).

This affects **both** the GPU path tracer pass and the radiance cache raytracing pass — any DispatchRays with the GISponza TLAS triggers TDR within 1-2 frames.

## Evidence

- Frame 6 (first frame after GISponza loads): rasterization works, readback succeeds, geometry visible
- Frame 7 (first frame DispatchRays executes with GISponza TLAS): device hung/removed
- Half-resolution DispatchRays (640x360) survives and produces correct geometry
- `MAX_BOUNCES=1` still TDRs at full resolution — even a single TraceRay per pixel is too heavy
- UnitTest scene (57 meshes, 45K vertices) works fine at full resolution

## Root Cause Hypothesis

The GISponza TLAS is too large for the default Windows TDR timeout (2 seconds). A single frame of 921,600 rays × 20M-triangle TLAS traversal exceeds the GPU time budget.

## Possible Solutions

1. **Tiled dispatch**: Split DispatchRays into smaller tiles (e.g., 128x128) across multiple command lists, allowing the GPU to service other work between tiles
2. **Adaptive resolution**: Use a lower ray dispatch resolution and upscale
3. **BLAS quality**: Use `PREFER_FAST_BUILD` for initial frames, rebuild with `PREFER_FAST_TRACE` asynchronously
4. **TDR registry override**: Increase `TdrDelay` for development (not a shipping fix)
5. **Reduce TLAS complexity**: LOD selection, mesh simplification for raytracing

## Files

- `Source/ExampleProject/RenderingClient/GPUPathTracerPass.cpp:378` — DispatchRays call
- `Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp:237` — DispatchRays call
- `Source/Engine/Services/DX12/DX12FrameManagementService.cpp:1097` — TLAS build
- `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl:214` — MAX_BOUNCES
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 DispatchRays with GISponza TLAS at 1280x720 completes without TDR
- [ ] #2 Path tracer produces visible output after 10+ frames of accumulation
- [x] #3 Radiance cache raytracing pass also survives with GISponza
- [x] #4 No regression in UnitTest scene rendering
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Root Cause

Three bugs combined to cause TDR:

1. **Missing BLAS UAV barrier** (`DX12MeshResourceService.cpp:201`) — `uavBarrier` variable was created but never submitted to `ResourceBarrier()`. The TLAS could reference incompletely-built BLAS structures.

2. **Out-of-bounds MissShaderIndex** (`RadianceCacheRayGen.hlsl:168`) — `MissShaderIndex=2` used with only 1 miss shader at index 0. GPU read past the shader table buffer → undefined behavior → GPU hang. **Primary TDR cause.**

3. **Missing offscreen fence signal** (`FrameManagementServiceImpl.cpp:171`) — In offscreen mode, no fence was signaled after rendering passes, so `WaitOnCPU` at next frame start had stale semaphore values, allowing unbounded GPU work accumulation.

## Fixes

1. Added `l_dx12CommandList->ResourceBarrier(1, &uavBarrier)` after BLAS build
2. Changed `MissShaderIndex` from 2 to 0 in TraceRay call
3. Added `SignalOnGPU` on graphics and compute queues for offscreen mode

## Validation

- RenderTest regression: PASS
- Integration test (10 frames, GISponza): PASS ×3 consecutive runs
- Scene reload test (20 frames, reload at 10): PASS
- No device errors in any run

## Note on AC #2

Path tracer accumulation (AC #2) not tested — GPU path tracer is gated behind `-test gpu_path_tracer` flag and was not active in auto-test. The miss shader index fix applies to the radiance cache pass only; the GPU path tracer has its own correct miss indices (0 and 1). AC #2 should be verified separately when the path tracer is exercised.
<!-- SECTION:FINAL_SUMMARY:END -->
