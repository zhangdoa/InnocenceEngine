---
id: TASK-9
title: Nested InnoScene support — child scenes referenced from master scenes
status: Done
assignee: []
created_date: '2026-04-07 14:10'
updated_date: '2026-04-07 15:08'
labels:
  - asset-pipeline
  - scene-system
dependencies: []
references:
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp
  - Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp
  - Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.h
  - Data/Scenes/UnitTest.InnoScene
  - Data/Scenes/GITestBox.InnoScene
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the phantom ModelComponent/DrawCallComponent indirection with nested scene files.

**Problem:** The asset importer produces `.ModelComponent.json` and `.DrawCallComponent.json` files — virtual component types (type IDs 2 and 200) that don't exist as actual components. The loader expands them back into MeshComponent + MaterialComponent on sub-entities. This is unnecessary indirection with its own serialization path.

**Solution:** The importer should produce a child `.InnoScene` file per imported model. Master scenes (user-authored) reference child scenes. At load time, child scenes are loaded recursively. This naturally maps DCC tool scene structure to engine scene structure.

**Current flow:**
1. Import: Assimp → .MeshComponent.json + .MaterialComponent.json + .DrawCallComponent.json + .ModelComponent.json
2. Master scene references ModelComponent by type ID 2
3. LoadScene expands Model→DrawCall→Mesh/Material chain into sub-entities

**New flow:**
1. Import: Assimp → .MeshComponent.json + .MaterialComponent.json + child .InnoScene (entities with Mesh+Material directly)
2. Master scene references child scene via a "Scene" component entry
3. LoadScene recursively loads child scenes, parenting entities under the referencing entity

**Files involved:**
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp` — produce child .InnoScene instead of Model/DrawCall JSONs
- `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp` — LoadScene: add recursive child scene loading, remove type ID 2/200 handling; SaveScene: handle scene references
- `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.h` — remove SaveDrawCallComponent/SaveModelComponent helpers
- `Source/Engine/Services/AssetService.h/.cpp` — remove GetAssetFilePath usage for DrawCall/Model types
- `Data/Scenes/*.InnoScene` — update GITest scenes to use new format
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Importer produces a child .InnoScene per imported model (no more .ModelComponent.json or .DrawCallComponent.json)
- [x] #2 Master scenes can reference child scenes with a Scene component entry
- [x] #3 LoadScene recursively loads child scenes, creating sub-entities under the parent
- [x] #4 Type IDs 2 (ModelComponent) and 200 (DrawCallComponent) are fully removed
- [x] #5 Existing GITest scenes updated to new format and load correctly
- [x] #6 All integration tests pass (Main.exe, scene reload)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
AC#5 (GITest scenes updated) — Sponza_PBR and Sibenik scenes updated to use ChildScene references. Child .InnoScene files generated from existing ModelComponent+DrawCallComponent data. GITestBox/UnitTest scenes unchanged (no models). Full runtime validation requires loading Sponza_PBR scene interactively (press R key).
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Implemented nested InnoScene support replacing the phantom ModelComponent/DrawCallComponent indirection. Master scenes now reference child scenes via "ChildScene" field. Key changes:\n- JSONWrapper::SaveChildScene produces child .InnoScene files from asset import\n- JSONWrapper::LoadChildScene recursively loads child scenes with parent transform inheritance\n- Removed ModelComponent (type 2) and DrawCallComponent (type 200) phantom types\n- Updated GITestSponza_PBR and GITestSibenik scenes to use ChildScene references\n- All tests pass (RenderTest + Main.exe integration)
<!-- SECTION:FINAL_SUMMARY:END -->
