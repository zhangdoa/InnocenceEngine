---
id: TASK-50
title: Name magic number constants in path tracer and lighting shaders
status: Done
assignee: []
created_date: '2026-04-16 19:05'
updated_date: '2026-04-18 11:43'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Named the most-reused constants in f1360657:

**Added to `common.hlsl`** — `TWO_PI`, `INV_PI`, `F0_DIELECTRIC` (0.04), `RAY_EPSILON` (0.001), `RAY_MAX_DISTANCE` (1e6), `RR_THROUGHPUT_THRESHOLD` (0.01).

**Replaced in `GPUPathTracerRayGen.hlsl` and `lightPass.comp`** — every recurring `3.14159265f`, `2.0f * 3.14159265f`, `float3(0.04,...)`, ray TMin/TMax/epsilons, and the Russian-roulette threshold. Values are bit-identical; only the names change.

Scope intentionally narrowed to those two files. Follow-ups on the task list if anyone wants to continue the hygiene pass:
- `RadianceCacheRayGen.hlsl` — same ray epsilon / tmax set applies
- `finalBlendPass.comp` — ISO / luminance adaptation coefficients (a different physics domain; better expressed as a single named block)
- `GPUPathTracerToneMap.hlsl` — ACES Narkowicz fit coefficients (should live as a single `ACES_FILMIC_*` block, not scattered scalars)

Regression tiers pass: RenderTest, scene reload, and `-test gpu_path_tracer` all exit 0.
<!-- SECTION:FINAL_SUMMARY:END -->
