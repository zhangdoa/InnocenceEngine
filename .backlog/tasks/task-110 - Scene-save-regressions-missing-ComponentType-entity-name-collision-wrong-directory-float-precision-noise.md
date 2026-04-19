---
id: TASK-110
title: >-
  Scene save regressions: missing ComponentType, entity-name collision, wrong
  directory, float precision noise
status: To Do
assignee: []
created_date: '2026-04-19 19:47'
labels:
  - editor
  - scene-save
  - bug
  - regression
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

`SceneService::Save` (IPC: `SAVE_SCENE`, UI: EditorHeader's save button → `sceneStore.saveScene()`) writes scene state back to disk, but the output has four issues visible in `git diff` after a no-op save:

### 1. MaterialComponent JSON drops `ComponentType` field

`JSONWrapper::to_json(json& j, const MaterialComponent& component)` (in `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`) builds the output without the `"ComponentType"` field. Transform/Light/Camera/Texture all include it. On the next scene load, the load path expects `ComponentType` at the component's top level. Tracked material JSONs (authored manually) include it; regenerated ones don't. This is a silent round-trip break.

Fix: add `{"ComponentType", component.GetTypeID()}` to the `j = json{...}` block for materials.

### 2. Saved component filenames use entity name instead of the originally-loaded component name

`JSONWrapper::SaveScene` at line 102 builds the filename as `l_Name + ".TransformComponent"`, where `l_Name` is the entity name. But the scene JSON references components by their originally-loaded filename (e.g. `GITestBox.Camera.TransformComponent`), not by entity name. Symptom: saving UnitTest creates `Main Camera.TransformComponent.json` (with a space!) while the old `GITestBox.Camera.TransformComponent.json` sits unchanged.

The scene's `Entities[].Components[].Name` field in the saved `.InnoScene` ends up pointing at the new filename, so next load reads the new file — but the old tracked file is orphaned. This also produces filenames with spaces, which is a mess cross-platform.

Fix: preserve the original component filename. Either store it on the component (MaterialComponent already has `m_InstanceName` but others don't), or look it up from a load-time map keyed by EntityID.

### 3. Saved component files land in wrong project directory

Save emits `Main Camera.TransformComponent.json` into `Data/Components/` and `Data/Generated/Components/`, not `Data/ExampleProject/Components/` (where the scene was loaded from). `AssetService::GetAssetFilePath` hardcodes or defaults a path that doesn't match the source project.

Fix: thread the project root (or the scene's `DefaultComponentPath`) through the save path.

### 4. Float precision expansion makes every diff noisy

nlohmann::json's default dump serializes floats at full precision (`0.782` → `0.7820000052452087`). Also: authored JSON was compact one-line (`{"R": 1, "G": 0.782, "B": 0.344, "A": 1.0}`), saved is pretty-printed multi-line. Both change every save even for untouched components, making actual diffs impossible to review.

Fix options:
- Write a custom dumper that trims trailing zeros beyond a tolerance (e.g. 5 decimals).
- Always pretty-print and accept authored format changes as one-time migration.
- Dump with `dump(4, ' ', true, nlohmann::json::error_handler_t::strict)` to control formatting, and post-process floats.

## Why

Save is the backbone of editor workflow. The current state silently corrupts references (#2, #3), breaks future loads (#1), and buries every diff in noise (#4). User can't usefully commit scene edits.

## Acceptance

- Saving an unmodified scene produces zero git diff (or diff limited to files the user genuinely edited).
- Material JSONs retain `ComponentType`.
- Component filenames match the ones in the loaded `.InnoScene`.
- Files land in the correct project directory (same as the scene's `DefaultComponentPath`).

## Not in scope

- Adding scene-format versioning / migration.
- Preserving BOM / line-ending style across saves (separate cleanup).
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
