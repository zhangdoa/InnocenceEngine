---
id: TASK-21
title: >-
  Add ImGui runtime tweaks panel — expose movement speed, rotate speed, and
  other tunable parameters
status: Done
assignee: []
created_date: '2026-04-13 08:05'
updated_date: '2026-04-29 07:55'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Status flip retrofitted 2026-04-29 during TASK-201 audit — work landed in commit `ff28c0be` (`feat(logic): runtime-tweaks ImGui panel for player tunables (TASK-21)`) with follow-up `e7fcb5c7` (`fix(logic): silence per-tick TweakRegistry save log (TASK-21 follow-up)`). The runtime-tweaks ImGui panel ships InitialMoveSpeed / SprintMultiplier / RotateSpeed / MouseSensitivity sliders, persists to `Data/ExampleProject/Configs/PlayerSettings.json`, and ships a reusable TweakRegistry header for future tunable registration. This is the systemic gap that TASK-193's closure-staleness gate now catches at commit time.
<!-- SECTION:FINAL_SUMMARY:END -->
