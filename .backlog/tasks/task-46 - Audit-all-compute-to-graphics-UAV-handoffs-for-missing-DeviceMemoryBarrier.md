---
id: TASK-46
title: Audit all compute-to-graphics UAV handoffs for missing DeviceMemoryBarrier
status: To Do
assignee: []
created_date: '2026-04-16 18:30'
labels:
  - reliability
  - DX12
  - GPU
dependencies: []
references:
  - Source/Shaders/HLSL/sunShadowCulling.comp
  - Source/Shaders/HLSL/opaqueGPUCulling.comp
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?**
TASK-44 revealed that GPU fence signaling does not guarantee UAV write visibility — DeviceMemoryBarrier() is required in the shader before wavefront retirement. This contract is not documented anywhere in the engine, and no other compute shaders have been audited for the same pattern.

**Structural weakness:** There is no engine-level enforcement or documentation of the rule: "any compute shader that writes a UAV buffer consumed by another queue must issue DeviceMemoryBarrier() after writes." Each shader author must independently know this.

**Improvement:**
1. Audit every compute shader that writes a UAV consumed cross-queue (grep for RWStructuredBuffer/RWByteAddressBuffer writes followed by cross-queue consumption)
2. Add DeviceMemoryBarrier() where missing
3. Document the rule in code-standards.md GPU section
4. Consider whether the engine's compute dispatch wrapper should warn when a UAV buffer's next consumer is on a different queue
<!-- SECTION:DESCRIPTION:END -->
