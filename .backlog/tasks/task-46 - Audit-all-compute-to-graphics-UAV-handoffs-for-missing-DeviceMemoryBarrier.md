---
id: TASK-46
title: Audit all compute-to-graphics UAV handoffs for missing DeviceMemoryBarrier
status: Done
assignee: []
created_date: '2026-04-16 18:30'
updated_date: '2026-04-16 20:56'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16 (completion — documentation + audit):**

Rule already documented in `Documents/code-standards.md` §8 "Cross-queue UAV writes require DeviceMemoryBarrier" (line 249-256). Audit complete — 20 of 22 compute shaders under `Source/Shaders/HLSL/` currently lack `DeviceMemoryBarrier()` after UAV writes; only `opaqueGPUCulling.comp` and `sunShadowCulling.comp` comply (added in TASK-44).

The per-shader insertion work has been split out to **TASK-54** because:
- Each shader needs per-control-flow-path review (early returns, multiple UAV-write branches).
- Visual regression validation is required (per `feedback_onscreen_testing.md`, not only `-offscreen`), which is best done interactively rather than blind.
- Scope fits a standalone CL rather than being bundled with documentation.

Engine-level warning consideration (point 4 of the task): deferred. The current `CommandListResourceService::Initialize` pattern (Graphics CL + Compute CL per pass) makes every compute-queue UAV writer cross-queue-adjacent by construction, which means a warning would fire on essentially every pass. A more targeted mechanism (e.g. tracking which UAVs are read by a different queue's next consumer in the command graph) would require a rendering-graph model that doesn't yet exist — out of scope.
<!-- SECTION:NOTES:END -->
