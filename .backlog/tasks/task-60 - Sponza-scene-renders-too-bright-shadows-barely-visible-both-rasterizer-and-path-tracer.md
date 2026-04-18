---
id: TASK-60
title: >-
  Sponza scene renders too bright / shadows barely visible (both rasterizer and
  path tracer)
status: To Do
assignee: []
created_date: '2026-04-18 09:50'
labels:
  - bug
  - rendering
  - sponza
  - lighting
  - scene-data
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User report (2026-04-18): when the auto-test loads `GISponza.InnoScene`, the output looks uniformly bright — curtain folds and pillar bases that should be in shadow are not visibly darker. The noise pattern is also strong enough that geometry is visible but contrast is flat.

**Key facts:**
- Same symptom in both the rasterizer and the GPU path tracer — not a path-tracer-only bug.
- `UnitTest.InnoScene` renders correctly in the same session (shadows and lighting look right).
- Hypothesis: Sponza scene's light intensities / RGB values / material values don't match the exposure + tonemap assumptions the engine's pipeline is calibrated for. The Sun JSON has `LuminousFlux = 100000`, same as UnitTest's — so the problem is more likely in *where* the light points, material range, or the combination of Sponza-sized geometry with the physical-camera exposure calc in `GPUPathTracerToneMap.hlsl`.

**Next investigation steps:**
1. Load `GITestBox` as a third reference (different intensities/layout) and compare output to isolate whether the problem is scene-data or pipeline.
2. Dump the tonemap input (AccumBuffer or GBuffer-light result) pixel statistics for Sponza vs UnitTest — if Sponza's raw HDR is all saturating the ACES knee, exposure is the issue.
3. Check Sponza's sun direction — if the sun points away from the scene or shadows land outside the frustum, fill lighting would look uniform.
<!-- SECTION:DESCRIPTION:END -->
