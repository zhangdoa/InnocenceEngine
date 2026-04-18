---
id: TASK-62
title: ImGui debug panel — scene picker, dev-time knobs
status: To Do
updated_date: '2026-04-18 10:30'
assignee: []
created_date: '2026-04-18 10:25'
labels:
  - architecture
  - ux
  - logic-client
dependencies: []
references:
  - Source/ExampleProject/LogicClient/World.inl
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Current pattern: `R → UnitTest`, `L → GISponza`, `Y → convertModel`. Every new scene needs a hunt for a free letter key, and the existing letters are arbitrary (no shared mnemonic, not grouped). An attempted `B → GITestBox` collided with `B → toggleGPUPathTracer` and was reverted.

**Decision (2026-04-18):** debug / dev-time features like scene selection belong in an ImGui panel. Keys are reserved for permanent operations (movement, camera toggle, render-mode toggle). This task expands from "scene picker" to "ImGui debug panel" so adjacent dev-time knobs live in the same place.

Scope:
- "Scenes" pane listing every `*.InnoScene` under `Data/ExampleProject/Scenes/` with a Load button per row; data-driven from the filesystem.
- Migrate `R → UnitTest` and `L → GISponza` off the keymap into the panel so scene-specific bindings stop leaking into keys.
- Host adjacent dev-time knobs (camera speed, light intensity, render-mode cycle) when they come up instead of filing a new task per panel.

Until the panel lands, GITestBox is reachable only by editing a hardcoded `SceneService::Load` call — the auto-test schedule (`-test … -total_frames …`) can be pointed at GITestBox for CI comparisons.
<!-- SECTION:DESCRIPTION:END -->
