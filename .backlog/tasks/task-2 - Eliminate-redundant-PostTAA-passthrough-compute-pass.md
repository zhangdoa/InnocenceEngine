---
id: TASK-2
title: Eliminate redundant PostTAA passthrough compute pass
status: Done
assignee: []
created_date: '2026-04-06 22:12'
updated_date: '2026-04-06 22:57'
labels:
  - shader
  - performance
  - pipeline
dependencies: []
references:
  - Source/Shaders/HLSL/postTAAPass.comp
  - Source/DefaultClient/RenderingClient/PostTAAPass.cpp
  - Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Source/Shaders/HLSL/postTAAPass.comp` is a pure passthrough — it copies `TAAResult.rgb` to output with alpha=1. This is an unnecessary full-screen GPU dispatch plus barrier transitions. FinalBlend could read TAA output directly, saving one compute pass. Requires changing FinalBlend's input binding and removing PostTAAPass from the pipeline.
<!-- SECTION:DESCRIPTION:END -->
