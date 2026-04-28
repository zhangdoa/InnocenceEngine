---
id: TASK-185
title: 'Output-texture viewer — pick any pass RT as overlay/window for diagnosis'
status: To Do
assignee: []
created_date: '2026-04-28 17:30'
labels:
  - editor
  - tooling
  - diagnostic
dependencies:
  - TASK-184
priority: medium
references:
  - Source/Engine/Services/EditorService.cpp
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced from the user's diagnostic-tooling priority directive 2026-04-28.**

Many diagnostic questions are answerable by looking at a specific pass's output texture (e.g. "is the SunShadowRT visibility texture all-1.0?"). Today this requires either RenderDoc capture or temporary code that dumps the texture to disk. Both are slow and break flow.

A runtime "render-target picker" lets the user pick any pass's output and see it overlaid/displayed in the editor — instant visual answer.

### What this delivers

1. An IPC endpoint on `EditorService.cpp` that lists all pass output textures (using `GetDispatchedPasses()` from TASK-171 + each pass's `GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[]`).
2. An editor panel (or part of an existing one) that lists the textures + picks one to display.
3. The display path: copy the picked texture to the swapchain (or a debug overlay region), or render it into a docked panel via the existing audit-dump-as-PNG path.

### Architecture options

- **Swapchain-overlay**: replaces the final composite output with the picked texture (full-screen). Cheapest implementation; one debug-write at end of LightPass or FinalBlendPass.
- **Editor-window**: editor reads the texture via the existing audit-dump infra, displays as a Vue `<img>` updated each frame. Cleaner UX but heavier IPC.

### Dependencies

- **TASK-184** (panel breakage fix) — without working editor panels, this can't ship a UI.
- TASK-171 `GetDispatchedPasses()` — already landed, lists all dispatchable passes.

### Why medium priority

High value but blocks on TASK-184. Sequenced after the panel-breakage fix.

### Owner

`graphics-api-expert` (texture-copy / debug-overlay path) + `editor-tooling-expert` (UI).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 IPC endpoint enumerates dispatchable-pass output textures
- [ ] #2 Editor panel exposes the list + picks one to display
- [ ] #3 Picked texture displays live (no save-to-disk roundtrip)
- [ ] #4 Toggle-off restores normal final-composite output
- [ ] #5 Live-engine spec covering the IPC roundtrip
- [ ] #6 Peer review per discipline
<!-- AC:END -->
