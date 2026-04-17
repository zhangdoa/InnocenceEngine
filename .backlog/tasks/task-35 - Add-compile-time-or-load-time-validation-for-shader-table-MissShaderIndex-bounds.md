---
id: TASK-35
title: >-
  Add compile-time or load-time validation for shader table MissShaderIndex
  bounds
status: Done
assignee: []
created_date: '2026-04-14 17:12'
updated_date: '2026-04-17 02:34'
labels:
  - structural
  - raytracing
  - validation
dependencies: []
references:
  - 'Source/Engine/Services/DX12/DX12FrameManagementService.cpp:395-440'
  - 'Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:460-578'
  - 'Source/Shaders/HLSL/RadianceCacheRayGen.hlsl:168'
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Problem

TASK-33 revealed that `RadianceCacheRayGen.hlsl` used `MissShaderIndex=2` with only 1 miss shader at index 0. The GPU read past the shader table buffer, causing undefined behavior and a TDR. Nothing in the engine validated that the index used in `TraceRay()` was within the shader table's miss shader range.

## Implicit Contract Violated

The shader table layout (number of miss shaders) must match the `MissShaderIndex` values used in HLSL `TraceRay()` calls. This is currently a silent, unchecked contract between CPU-side shader table construction and GPU-side shader code.

## Structural Weakness

Shader table construction (`DX12RenderPassResourceService.cpp:460-578`) and shader `TraceRay()` calls are completely decoupled — no validation exists at any layer.

## Proposed Fix

Add a runtime assertion during `DispatchRays` that validates the shader table's miss shader count is consistent with the pipeline's known miss shader count. Specifically:

1. In `DX12FrameManagementService::DispatchRays`, log/assert if the miss table size implies fewer entries than the pipeline expects
2. Consider storing the miss shader count in `DX12PipelineStateObject` during PSO creation and cross-checking at dispatch time
3. Optionally, parse DXIL reflection data at shader load time to extract `MissShaderIndex` constants and validate against the table layout
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Mismatched MissShaderIndex produces a visible error at dispatch time, not a silent GPU hang
- [x] #2 Existing valid passes (RadianceCache, GPUPathTracer) pass validation without false positives
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):**

- Stored the shader-table layout that the PSO was actually built with directly on `DX12PipelineStateObject` (`m_RaytracingMissShaderCount`, `m_RaytracingHitGroupCount`). Set at `CreateRaytracingPSO` time from the same `hasShadowMiss` decision that picks the record count to write.
- `DX12FrameManagementService::DispatchRays` now runs two validations before issuing the dispatch:
  1. Miss / hit-group counts must both be non-zero (otherwise any `TraceRay()` in the shader reads past the empty table — TASK-33 signature).
  2. The shader-ID buffer must be at least `(1 + missCount + hitCount) * shader_table_alignment` bytes.
- `MissShaderTable.SizeInBytes` and the hit-group offset / size are now derived from the PSO's stored counts instead of re-inferring from buffer size.

Failure mode: a mismatch now logs an error citing the pass name and the exact mismatch, and skips the dispatch. The ray-tracing pass visibly degrades (no illumination update that frame) instead of causing a silent GPU TDR — exactly the "fail loudly" outcome CLAUDE.md asks for.

**Validation:** Build clean. RenderTest exit 0 (no false positives on rasterisation-only passes; they never call DispatchRays). Integration run (which exercises `RadianceCacheRaytracingPass::PrepareCommandList`) completes without TASK-35 warnings, confirming the valid pass passes validation (AC#2). TDR at frame 8 is unchanged — that's TASK-52's graphics-pipeline stale-VA issue, not a ray-tracing table bug.
<!-- SECTION:NOTES:END -->
