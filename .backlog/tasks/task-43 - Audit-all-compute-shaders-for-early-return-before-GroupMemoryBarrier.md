---
id: TASK-43
title: Audit all compute shaders for early-return before GroupMemoryBarrier
status: To Do
assignee: []
created_date: '2026-04-16 16:00'
labels:
  - structural
  - shaders
  - correctness
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?** All threads in a workgroup must reach `GroupMemoryBarrierWithGroupSync`. Early returns before barriers cause undefined behavior — some threads skip the barrier while others wait forever, leading to hangs or silent corruption.

**What structural weakness allowed it?** HLSL has no compile-time check for barrier reachability. The early-return pattern is natural in C++ but illegal before barriers in compute shaders. No project guideline or review checklist catches this.

**What improvement?** Audit all `.comp` shaders for early returns before `GroupMemoryBarrierWithGroupSync`. Establish a coding pattern: use an `earlyExit` flag instead of `return`, branch on it after barriers. Document this in the shader coding guidelines.

Fixed in `RadianceCacheReprojection.comp` (commit 8d1ff230). Need to check: `sunShadowCulling.comp`, `opaqueGPUCulling.comp`, `luminanceHistogramPass.comp`, `luminanceAveragePass.comp`, `lightCulling.comp`, `tileFrustum.comp`, `SSAOPass.comp`, all RadianceCache filter passes.
<!-- SECTION:DESCRIPTION:END -->
