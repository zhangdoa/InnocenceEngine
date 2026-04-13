---
id: TASK-21
title: >-
  Add ImGui runtime tweaks panel — expose movement speed, rotate speed, and
  other tunable parameters
status: To Do
assignee: []
created_date: '2026-04-13 08:05'
labels:
  - ui
  - imgui
  - usability
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Movement speed and rotation speed are currently hardcoded constants in Player.inl (m_InitialMoveSpeed=0.05, m_RotateSpeed=10.0). Add an ImGui panel that exposes these at runtime so the user can tune them without recompiling.

Scope:
- Add ImGui window (e.g. "Player Settings" or "Debug Tweaks")
- Sliders for move speed (base and sprint multiplier), rotate speed, mouse sensitivity
- Persist values to a config JSON so settings survive restarts
- Consider a general TweakVar registration mechanism to avoid hardcoding each variable in the UI
<!-- SECTION:DESCRIPTION:END -->
