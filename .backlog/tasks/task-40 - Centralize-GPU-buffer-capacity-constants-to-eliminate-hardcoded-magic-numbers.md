---
id: TASK-40
title: Centralize GPU buffer capacity constants to eliminate hardcoded magic numbers
status: To Do
assignee: []
created_date: '2026-04-16 08:03'
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
