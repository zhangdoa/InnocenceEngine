---
id: TASK-151
title: 'Stale clangd index after WIP revert deleting files — mirrors TASK-146 DXIL class'
status: To Do
assignee: []
created_date: '2026-04-26 23:50'
labels:
  - infrastructure
  - tooling
  - clangd
  - bug
dependencies: []
references:
  - .claude/disciplines/regression-fix-flow.md
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Discovered 2026-04-26 during TASK-66 multi-agent dispatch session, while triaging session-start clangd diagnostics.**

Same class of bug as TASK-146 (mirror-semantic shader deploy), one stack lower:

- **TASK-146**: stale `.dxil` artifacts in `Bin/Shaders/DXIL/` after WIP revert → engine loaded mismatched shaders → device-removed.
- **TASK-151 (this task)**: stale clangd in-memory index referencing deleted source files (e.g. `SunShadowRTPass.h/cpp` from reverted TASK-138 WIP) → ghost diagnostics across `LightPass.cpp`, `ExampleRenderingClient.cpp`, etc., even though the live source no longer references the deleted files.

### Symptoms

Session-start clangd reports lines like:

- `'SunShadowRTPass.h' file not found` on tracked files that don't actually `#include` it.
- `Use of undeclared identifier 'SunShadowRTPass'` at line numbers whose actual content is unrelated.
- `Cannot initialize object parameter of type 'Inno::IRenderPass'` for tracked passes (`GPUPathTracerPass`, `SSAOPass`) that DO cleanly inherit from `IRenderPass` in the live source.

Verified false by:
- `git ls-files Source/ExampleProject/RenderingClient/` — no `SunShadowRTPass*` tracked.
- `Grep "SunShadowRTPass" Source/` — zero matches.
- Reading `LightPass.cpp:1-30` directly — line 13 is `#include "LightCullingPass.h"`, not `SunShadowRTPass.h`.

### Why this hurts

Per `feedback_no_dismissing_tool_noise.md`, the team must fix false IDE/linter diagnostics rather than learn to ignore them. Each session start currently floods main-session Claude with bogus errors that need investigation to dismiss; that investigation cost compounds across sessions and across agents (each in-flight agent sees the same noise).

### Required fix — options

1. **Auto-trigger compile_commands.json regeneration on revert/branch-switch** via a git post-checkout / post-merge hook that runs `cmake -B Build -S . --regenerate-compile-commands` (or equivalent).
2. **CMake post-build / post-configure step that emits a `compile_commands.json` invalidation marker** so clangd reindexes.
3. **Document a manual flow** in `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination": after any revert that deletes files, regenerate compile_commands.json and restart clangd. Lower-impact than #1 but loses automation.

### Owner

`ci-build-expert` — owns CMake + build automation. Coordinate with `ai-expert` if discipline-doc changes are needed.

### Why not high priority

Unlike TASK-146, this does NOT cause runtime failures or phantom regressions in the engine — only false IDE noise. Real `cmake --build` is unaffected (verified by ci-build-expert's TASK-146 build run after the same revert). Backlog at medium until frequency justifies infrastructure investment.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Mechanism (auto or manual) chosen + implemented to invalidate clangd index after revert/branch-switch
- [ ] #2 If automation: validated by reverting a WIP that deletes a header, observing clangd no longer reports stale references on next index pass
- [ ] #3 If manual: discipline updated in `.claude/disciplines/regression-fix-flow.md` § "Build-cache contamination" with the C++ analog
- [ ] #4 Recurrence test documented — same bisect/revert pattern that triggered TASK-146 should not produce ghost C++ diagnostics
<!-- AC:END -->
