---
id: TASK-50
title: Name magic number constants in path tracer and lighting shaders
status: To Do
assignee: []
created_date: '2026-04-16 19:05'
labels:
  - code-quality
  - shaders
dependencies: []
references:
  - Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl
  - Source/Shaders/HLSL/RadianceCacheRayGen.hlsl
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/finalBlendPass.comp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** Multiple shaders use bare numeric literals for physically-motivated constants:
- GPUPathTracerRayGen.hlsl: `3.14159265f` instead of `PI`, ray epsilons (0.001, 0.002), F0 dielectric default (0.04), Russian roulette threshold (0.01/0.05), temporal blending weights
- RadianceCacheRayGen.hlsl: ray origin offset (0.001), TMin/TMax (0.01/1000.0), temporal weight (0.15)
- lightPass.comp: F0 dielectric (0.04), fog decay constant (8), heat map alpha values
- finalBlendPass.comp: ISO/exposure constants (100.0, 12.5, 1.2, 9.6)
- GPUPathTracerToneMap.hlsl: ACES Narkowicz fit coefficients (2.51, 0.03, 2.43, 0.59, 0.14) unnamed

**Fix:** Define named constants in common.hlsl or per-shader headers. Group by domain: ray tracing constants, PBR material constants, tone mapping constants, temporal filtering constants.
<!-- SECTION:DESCRIPTION:END -->
