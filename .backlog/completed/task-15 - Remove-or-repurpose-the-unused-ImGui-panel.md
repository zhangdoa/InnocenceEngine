---
id: TASK-15
title: Remove or repurpose the unused ImGui panel
status: Done
assignee: []
created_date: '2026-04-12 18:36'
updated_date: '2026-04-12 23:12'
labels:
  - editor
  - cleanup
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The in-engine ImGui panel is partially functional but provides no meaningful value now that a dedicated editor is being built. Options: (a) remove it entirely, (b) keep it as a lightweight debug overlay (FPS, GPU memory, active render passes) that complements rather than duplicates the editor. Decision should be made once the editor's capabilities are clearer.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision made and documented
- [x] #2 Either removed cleanly (no dead ImGui code) or scoped down to debug-only overlay
- [x] #3 No regressions in windowed mode
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Decision: scope down to debug-only overlay. Removed showWorldExplorer() and showLightComponentPropertyEditor() — both were dead code (selectedComponent was never set so the property editor never executed). Also removed unused EntityRegistry.h/AssetService.h includes and fixed a %d/%zu format mismatch. Kept showApplicationProfiler() (FPS, render toggles, scene load/save, ray trace button) and showConcurrencyProfiler() (thread timeline). Build clean, RenderTest exit 0.
<!-- SECTION:FINAL_SUMMARY:END -->
