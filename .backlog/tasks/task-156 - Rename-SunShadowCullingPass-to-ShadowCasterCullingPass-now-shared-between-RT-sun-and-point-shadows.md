---
id: TASK-156
title: 'Rename SunShadowCullingPass → ShadowCasterCullingPass (now shared between RT sun + point shadows)'
status: To Do
assignee: []
created_date: '2026-04-27 14:30'
labels:
  - rendering
  - cleanup
  - naming
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/SunShadowCullingPass.h
  - Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-138 phase 2 closure, 2026-04-27.**

After TASK-138 deleted the sun CSM rasterizer path, `SunShadowCullingPass` retained because TASK-66's `PointShadowGeometryProcessPass.cpp:222` reuses its indirect-draw command buffer. The "Sun" prefix is now misleading — the class produces shadow-caster culling output consumed by point/sphere shadow passes, not by anything sun-related.

### Required fix

- Rename class + file: `SunShadowCullingPass` → `ShadowCasterCullingPass` (or similar — pick a name that reflects its actual role across the shadow types it now serves).
- Update consumers: only `PointShadowGeometryProcessPass.cpp:222` per current state; verify with grep.
- Update shader file `sunShadowCulling.comp` to a matching name if it exists.

### Why low priority

Pure naming hygiene; no behavioural change. Could be batched with any other shadow-related cleanup.

### Owner

`rendering-researcher` (render-pass C++ + shader naming).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Class + file renamed; consumers updated; build green
- [ ] #2 Shader file (if applicable) renamed to match
- [ ] #3 Grep for old name across repo returns zero hits in tracked source (WIP/* matches OK)
<!-- AC:END -->
