---
id: TASK-49
title: >-
  Fix return false from pointer-returning GetResult() in MotionBlurPass and
  SunShadowBlurEvenPass
status: To Do
assignee: []
created_date: '2026-04-16 19:05'
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
