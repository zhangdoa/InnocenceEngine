---
id: TASK-43
title: Audit all compute shaders for early-return before GroupMemoryBarrier
status: Done
assignee: []
created_date: '2026-04-16 16:00'
updated_date: '2026-04-16 21:06'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16 (completion):** Audit + documentation.

**Audit result (`Source/Shaders/HLSL/*.comp`):** Every shader that uses `GroupMemoryBarrierWithGroupSync` is already compliant — no early `return` before any `*WithGroupSync` call.

| Shader                              | *WithGroupSync barriers | Early returns before? |
|------------------------------------|------------------------:|----------------------|
| RadianceCacheReprojection.comp      | 3                       | no (fixed in 8d1ff230) |
| RadianceCacheIntegration.comp       | 2                       | no                   |
| lightCulling.comp                   | 4                       | no                   |
| luminanceAveragePass.comp           | 2                       | no                   |
| luminanceHistogramPass.comp         | 2                       | no                   |

Shaders listed in the task that use only `DeviceMemoryBarrier` (no `WithGroupSync`) — `sunShadowCulling.comp`, `opaqueGPUCulling.comp`, `tileFrustum.comp`, `SSAONoisePass.comp`, `RadianceCacheFilter*.comp` — are exempt because `DeviceMemoryBarrier` is a memory fence, not a cross-thread sync; early returns are legal for them.

**Documentation:** added "No early return before `GroupMemoryBarrierWithGroupSync`" to `Documents/code-standards.md` §8 right after the existing DMB rule, with the `earlyExit` flag pattern and `RadianceCacheReprojection.comp` as the reference exemplar.
<!-- SECTION:NOTES:END -->
