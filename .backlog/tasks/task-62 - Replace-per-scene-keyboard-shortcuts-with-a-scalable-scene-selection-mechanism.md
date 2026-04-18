---
id: TASK-62
title: Replace per-scene keyboard shortcuts with a scalable scene-selection mechanism
status: To Do
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

**Candidate replacements (for the human to pick from, not an exhaustive list):**

1. **Cycle key.** Bind one key (e.g. Tab) to iterate over every `*.InnoScene` file in `Data/ExampleProject/Scenes/`. Scales to any number of scenes, no keymap collisions, data-driven. Log the new scene name on each cycle so the user knows what they switched to.
2. **ImGui scene picker.** A "Scenes" pane that lists available scene files and loads the clicked one. Zero keyboard commitment, discoverable, mouse-friendly for comparison work.
3. **Console command.** `load SceneName` typed into an existing or new in-engine console. Zero keyboard commitment; composes with other dev commands.

Until one is picked, the engine has no way to load GITestBox interactively — the auto-test hardcodes GISponza at frame 5 and the `L` key loads GISponza. GITestBox is still on disk at `Data/ExampleProject/Scenes/GITestBox.InnoScene`.
<!-- SECTION:DESCRIPTION:END -->
