---
id: TASK-192
title: >-
  Editor: Selector primitive in DevToggleRegistry + dropdown for visualization
  modes (TASK-183 child)
status: To Do
assignee: []
created_date: '2026-04-28 19:33'
labels:
  - editor
  - tooling
  - diagnostic
dependencies:
  - TASK-183
references:
  - Source/Engine/Services/DevToggleRegistry.h
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
  - Source/Editor-Next/src/store/devToggleStore.js
  - Source/Engine/Services/EditorService.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

Follow-up to **TASK-183** (engine-side runtime visualization modes shipped via N mutually-exclusive bool toggles). This task replaces the N-bool UX with a dedicated **Selector** primitive (string-valued enum) for cleaner picker semantics and editor dropdown UI.

### Background

TASK-183 (engine-side child) shipped runtime visualization modes by registering one bool toggle per mode (`DebugView_DirectOnly`, `DebugView_IndirectOnly`, `DebugView_GBufferAlbedo`, etc.). Setting any one to `true` clears the others. This works but presents N switches in the editor where one dropdown would be clearer — and the underlying domain *is* an enum, not N independent flags.

### What this delivers

1. New primitive in `Source/Engine/Services/DevToggleRegistry.{h,cpp}`:
   ```cpp
   struct Selector {
       std::string m_Name;
       std::vector<std::string> m_Options;     // ordered list of valid values
       std::function<std::string()> m_Get;
       std::function<void(const std::string&)> m_Set;
   };
   void RegisterSelector(...); std::vector<Selector> AllSelectors(); ...
   ```
2. New IPC verbs in `Source/Engine/Services/EditorService.cpp`: `SET_DEV_SELECTOR` and selectors block in `LIST_DEV_TOGGLES` payload.
3. Editor: `Source/Editor-Next/src/store/devToggleStore.js` extends with `selectors` array; `RenderTogglesPanel.vue` renders an `n-select` per selector.
4. Migrate the TASK-183 N-bool DebugView toggles to a single `DebugView` selector. Bool toggles deregistered.
5. Playwright spec covering dropdown → engine state.

### Owner

`editor-tooling-expert` (Vue + IPC + DevToggleRegistry primitive).

## Acceptance Criteria
- [ ] #1 `Selector` primitive + IPC verbs landed; bool-toggle path unchanged
- [ ] #2 `DebugView` selector replaces the N `DebugView_*` bools
- [ ] #3 Editor dropdown switches modes live (no engine restart)
- [ ] #4 Playwright spec exercises the dropdown end-to-end
- [ ] #5 Peer review per discipline
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
