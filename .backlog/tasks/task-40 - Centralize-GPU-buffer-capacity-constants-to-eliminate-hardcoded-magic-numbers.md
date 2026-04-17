---
id: TASK-40
title: Centralize GPU buffer capacity constants to eliminate hardcoded magic numbers
status: Done
assignee: []
created_date: '2026-04-16 08:03'
updated_date: '2026-04-17 03:06'
labels:
  - structural
  - rendering
  - gpu-safety
dependencies: []
references:
  - 'Source/ExampleProject/RenderingClient/SunShadowCullingPass.cpp:28'
  - 'Source/ExampleProject/RenderingClient/OpaqueCullingPass.cpp:28'
  - 'Source/Engine/Services/RenderingConfigurationService.cpp:23'
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?** IndirectDrawCommandBuffer capacity (512) must be >= maxMeshes (1024), but this was never enforced — the two values lived in different files with no link between them.

**What structural weakness allowed it?** GPU buffer sizes are hardcoded magic numbers in individual render pass Setup() functions, disconnected from the system-wide RenderingCapability that defines the actual limits. Any new pass that allocates a per-model buffer must independently know the right capacity.

**What improvement moves toward orthogonality and explicit contracts?** All per-model GPU buffer capacities should derive from RenderingCapability.maxMeshes (or a central constant). A static_assert or runtime check should verify that IndirectDrawCommandBuffer.ElementCount >= DrawCallService model buffer capacity. This prevents future passes from introducing the same class of overflow bug.

Immediate fix was applied in commit 0a841cfa — both culling passes now read maxMeshes from RenderingConfigurationService. This task tracks the broader structural improvement of making this pattern systematic across all GPU buffer allocations.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):** Every per-model GPU buffer in `DrawCallService` already derived its capacity from `RenderingCapability.maxMeshes` / `maxMaterials` (landed in commit 0a841cfa). This CL closes the remaining gap by adding a runtime overflow guard at upload time.

`DrawCallServiceImpl::Update` now clamps each `Upload(buffer, vector, 0, N)` call to `min(vector.size(), buffer->m_ElementCount)` via a file-local `l_clamp` lambda. A size overflow logs a warning naming the buffer, the produced count, the capacity, and the drop count — making a silent CPU/GPU-side overfill (the TASK-44 class of bug) loudly visible at its source.

Applied to:
- `GPUModelDataBuffer` (CPU vector: `m_GPUModelDataVector`)
- `TransformBuffer` current-frame ping-pong (`m_TransformBufferVector`)
- `MaterialCBuffer` (`m_MaterialCBVector`)

**Validation:** Build clean. RenderTest exit 0. Integration test produces zero overflow warnings on the 10-frame UnitTest→GISponza auto-test, confirming current counts fit within the configured capacities.

A stricter compile-time link (a shared `inline constexpr uint32_t kMaxMeshes` that every capacity derives from, plus `static_assert(IndirectDrawCommandBuffer.ElementCount >= kMaxMeshes)`) would need `RenderingCapability` to be a constexpr source of truth instead of a runtime-loaded config; that's a bigger decision than this CL.
<!-- SECTION:NOTES:END -->
