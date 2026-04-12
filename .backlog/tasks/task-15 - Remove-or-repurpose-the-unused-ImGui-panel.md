---
id: TASK-15
title: Remove or repurpose the unused ImGui panel
status: To Do
assignee: []
created_date: '2026-04-12 18:36'
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
- [ ] #1 Decision made and documented
- [ ] #2 Either removed cleanly (no dead ImGui code) or scoped down to debug-only overlay
- [ ] #3 No regressions in windowed mode
<!-- AC:END -->
