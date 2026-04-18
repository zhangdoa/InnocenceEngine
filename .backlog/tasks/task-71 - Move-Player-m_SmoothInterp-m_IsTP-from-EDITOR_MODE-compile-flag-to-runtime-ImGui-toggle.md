---
id: TASK-71
title: >-
  Move Player m_SmoothInterp / m_IsTP from EDITOR_MODE compile flag to runtime
  ImGui toggle
status: To Do
assignee: []
created_date: '2026-04-18 17:35'
labels:
  - imgui
  - player
  - tech-debt
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Source/ExampleProject/LogicClient/Player.inl` lines 67–73 select `m_SmoothInterp` and `m_IsTP` (third-person vs first-person camera) via `#ifdef EDITOR_MODE`, forcing a rebuild to flip modes. Both are runtime behaviours — toggle them from the ImGui debug panel instead, so switching cost goes from "minutes (rebuild)" to "one click."

Depends on the ImGui debug panel scaffold (TASK-21/62 — currently still To Do). Once that panel exists, add two bool toggles and delete the `#ifdef`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 m_SmoothInterp and m_IsTP default values still match current EDITOR_MODE / non-EDITOR_MODE behavior, but are mutable at runtime
- [ ] #2 Both toggleable from the ImGui debug panel
- [ ] #3 #ifdef EDITOR_MODE block in Player.inl removed
<!-- AC:END -->
