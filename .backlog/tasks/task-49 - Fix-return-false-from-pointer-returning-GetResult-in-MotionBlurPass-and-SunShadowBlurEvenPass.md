---
id: TASK-49
title: >-
  Fix return false from pointer-returning GetResult() in MotionBlurPass and
  SunShadowBlurEvenPass
status: Done
assignee: []
created_date: '2026-04-16 19:05'
updated_date: '2026-04-16 21:05'
labels:
  - bugfix
  - rendering
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/MotionBlurPass.cpp
  - Source/ExampleProject/RenderingClient/SunShadowBlurEvenPass.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** `GetResult()` returns `GPUResourceComponent*` but uses `return false;` instead of `return nullptr;` in MotionBlurPass.cpp:140,143 and SunShadowBlurEvenPass.cpp:136,139. The compiler silently converts `false` to a null pointer, but this is a type confusion bug that's misleading and may behave differently across compilers.

**Fix:** Replace `return false;` with `return nullptr;` in all pointer-returning functions.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-16 (completion):** Fixed `return false;` → `return nullptr;` in five `GPUResourceComponent* ::GetResult()` functions. The task description listed two (MotionBlurPass, SunShadowBlurEvenPass); an AST-style sweep across `Source/ExampleProject/RenderingClient/` surfaced three more with the same pattern:
- `SunShadowBlurOddPass::GetResult()` (mirror of Even)
- `SunShadowGeometryProcessPass::GetResult()`
- `TransparentBlendPass::GetResult()`

Engine/Editor trees scanned with the same pattern — no further occurrences outside `Source/ExampleProject/RenderingClient/`. Build clean, RenderTest exit 0.
<!-- SECTION:NOTES:END -->
