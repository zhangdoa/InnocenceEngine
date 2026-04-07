---
id: TASK-1
title: Fix "bassPass" typo in finalBlendPass.comp
status: Done
assignee: []
created_date: '2026-04-06 22:12'
updated_date: '2026-04-06 22:57'
labels:
  - shader
  - cleanup
dependencies: []
references:
  - Source/Shaders/HLSL/finalBlendPass.comp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The variable names in `Source/Shaders/HLSL/finalBlendPass.comp` use `bassPass` instead of `basePass` throughout. Not a correctness issue but reduces readability. Simple find-and-replace.
<!-- SECTION:DESCRIPTION:END -->
