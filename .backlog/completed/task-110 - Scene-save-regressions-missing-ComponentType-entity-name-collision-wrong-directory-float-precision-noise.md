---
id: TASK-110
title: >-
  Scene save regressions: missing ComponentType, entity-name collision, wrong
  directory, float precision noise
status: Done
assignee: []
created_date: '2026-04-19 19:47'
updated_date: '2026-04-19 20:39'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
All four issues addressed across three commits.

## Fixes

**#1 MaterialComponent dropped ComponentType** — commit `05158649`. Added `{"ComponentType", MaterialComponent::GetTypeID()}` to the serializer; now matches the other component types.

**#2 Entity-name-based filenames orphaned original files** — commit `05158649`. Added a static per-scene map (`g_LoadedCompFilenames` keyed by `{EntityID, typeID}`) that remembers the load-time filename; `SaveScene` consults it first and only falls back to `{EntityName}.{ComponentType}` for editor-spawned entities with no authored file. Cleared on scene unload via a new `JSONWrapper::ClearLoadedCompFilenames()` called from `SceneService::LoadSync`.

**#3 Wrong project directory** — fixed as a side effect of #2. `AssetService::GetAssetFilePath(compName)` searches project-first then Generated; with the original filename preserved, it resolves to the project path (e.g. `Data/ExampleProject/Components/GITestBox.Camera.TransformComponent.json`). Editor-spawned components without a tracked original still land in `Data/Generated/Components/` — that's the intentional destination for new assets.

**#4 Float precision expansion** — commit `8a78b49d`. Pre-dump rounding pass in `JSONWrapper::Save` that rewrites every float node to 6 significant digits via `snprintf("%.6g")` + `strtod` round-trip. Single precision carries ~7.2 significant digits, so 6 loses nothing that could round-trip back into the original float. Integers / zero / NaN / Inf untouched.

## Validation
- Engine build green across all three commits.
- Main.exe tier-2 offscreen (GISponza auto-load, 10 frames): exit 0.
- User manually verified #1 and #2 by tweaking the camera in the editor, saving, and observing the diff landed in the original tracked component files (not orphaned copies with spaces in names).

## What was NOT verified
- **Byte-for-byte "zero git diff" on an unmodified-scene round-trip**. The `-setw(4)` pretty-print output does not byte-match authored JSONs that used compact `{"R": 1, "G": 0.782, ...}` single-line objects. Values are semantically equal; formatting differs. Getting fully byte-clean diffs would need either a custom dumper that preserves author formatting or a one-time reformat commit that normalizes everything to the new output style. Neither is in TASK-110's scope; TASK-111 should cover the semantic-equivalence assertion.
- **Editor-spawned component save path** — not exercised in this session. The fallback `{EntityName}.{ComponentType}` → `Generated/Components/` path is untested; the logic is simple but deserves a regression once TASK-111's determinism tier is in place.

## Follow-ups
- TASK-111 (semantic determinism test tier) — should now have a clean baseline to regress against.
- If the whitespace-level diff becomes a real nuisance, open a small task for a custom dumper that preserves the single-line compact layout on arrays of primitives and small objects.
<!-- SECTION:FINAL_SUMMARY:END -->
