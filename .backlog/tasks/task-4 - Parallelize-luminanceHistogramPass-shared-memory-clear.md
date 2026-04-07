---
id: TASK-4
title: Parallelize luminanceHistogramPass shared memory clear
status: Done
assignee: []
created_date: '2026-04-06 22:13'
updated_date: '2026-04-06 22:57'
labels:
  - shader
  - performance
dependencies: []
references:
  - Source/Shaders/HLSL/luminanceHistogramPass.comp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
In `Source/Shaders/HLSL/luminanceHistogramPass.comp`, lines 42-49 use a single thread (groupThreadID 0,0) to clear 256 shared memory entries in a loop. The thread group is 16x16=256 threads — each thread could clear one entry in parallel. Minor performance improvement.
<!-- SECTION:DESCRIPTION:END -->
