---
id: TASK-6
title: 'Radiance cache quality: align with AMD GI 1.0 reference'
status: To Do
assignee: []
created_date: '2026-04-07 09:26'
updated_date: '2026-04-07 13:24'
labels:
  - rendering
  - GI
  - long-term
dependencies: []
references:
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Source/Shaders/HLSL/RadianceCacheReprojection.comp
  - Source/Shaders/HLSL/RadianceCacheFilterHorizontal.comp
  - Source/Shaders/HLSL/RadianceCacheIntegration.comp
  - Source/Shaders/HLSL/lightPass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The radiance cache implementation has quality gaps compared to AMD's GI 1.0 reference. Several issues have been fixed, remaining work focuses on probe placement and advanced features.

**Fixed (April 2026):**
- Bilateral filter: replaced circular radiance-similarity weight with depth-based weight (both H and V passes)
- HDR clamps: removed irradiance clamp of 5.0 in lightPass.comp and output clamps in both filter passes
- Firefly suppression: replaced absolute variance threshold with scale-invariant relative variance
- SH temporal accumulation: added EMA blending (90% history, 10% current) in integration pass
- Sampling: replaced hash-based random with R2 quasi-random sequence
- Jitter: confirmed radianceCacheJitter IS set via PerFrameDataService (original description was wrong)
- RadianceCacheIntegration clamp raised from 10 to 1000 for HDR

**Remaining:**
- Probe placement needs rework (user notes the paper is difficult to follow, current placement may be incorrect)
- Screen-space short-range tracing before full ray cast
- Probe validity/confidence classification
- Multi-resolution cascades for different GI scales
- L2 SH (9 coefficients) for better directional resolution (currently L1 with 4 coefficients)
- World-space probe grid improvements for off-screen GI
<!-- SECTION:DESCRIPTION:END -->
