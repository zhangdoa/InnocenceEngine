---
id: TASK-3
title: Fix luminanceAveragePass history buffer overwrite at index 0
status: Done
assignee: []
created_date: '2026-04-06 22:13'
updated_date: '2026-04-06 22:57'
labels:
  - shader
  - bug
  - auto-exposure
dependencies: []
references:
  - Source/Shaders/HLSL/luminanceAveragePass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
In `Source/Shaders/HLSL/luminanceAveragePass.comp`, line 68 writes `out_average[0] = adaptedLuminance` (the moving average result). But line 57 also writes the current frame's raw luminance to `out_average[bufferIndex]` where `bufferIndex = frameIndex % 8`. When `bufferIndex == 0`, the raw value is immediately overwritten by the adapted average, losing one history sample. This creates a subtle periodic glitch every 8 frames. Fix: store the adapted average in a separate location (e.g., index 8) or use a dedicated output buffer.
<!-- SECTION:DESCRIPTION:END -->
