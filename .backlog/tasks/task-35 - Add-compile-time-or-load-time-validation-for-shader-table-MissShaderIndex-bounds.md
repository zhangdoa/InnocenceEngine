---
id: TASK-35
title: >-
  Add compile-time or load-time validation for shader table MissShaderIndex
  bounds
status: To Do
assignee: []
created_date: '2026-04-14 17:12'
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
- [ ] #1 Mismatched MissShaderIndex produces a visible error at dispatch time, not a silent GPU hang
- [ ] #2 Existing valid passes (RadianceCache, GPUPathTracer) pass validation without false positives
<!-- AC:END -->
