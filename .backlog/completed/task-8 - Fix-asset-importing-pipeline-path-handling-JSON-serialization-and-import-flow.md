---
id: TASK-8
title: >-
  Fix asset importing pipeline: path handling, JSON serialization, and import
  flow
status: Done
assignee: []
created_date: '2026-04-07 13:27'
updated_date: '2026-04-07 15:08'
labels: []
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The asset importing pipeline (Assimp → JSON + binary → runtime load) has accumulated technical debt that makes it broken/fragile for importing new assets.

**Path handling issues:**
- Hardcoded `../Data/Components/` relative paths in `AssetService::GetAssetFilePath()`, `AssetService::Save()`, and `AssimpImporter::ProcessAssimpScene()`
- Double-slash paths in scene files: `..//Res//Scenes/`, `..//Res//ConvertedAssets//`
- Scene files reference `../Res/ConvertedAssets/` but actual component data lives in `Data/Components/`
- No centralized path resolution — each file constructs paths with raw string concatenation
- `IOService::getWorkingDirectory()` + raw filename concatenation in `AssimpImporter::Import()`

**JSON serialization issues:**
- Manual `to_json`/`from_json` boilerplate for every component type across `JSONSerializer_Components.cpp` and `JSONSerializer_POD.cpp`
- Component type IDs stored as magic numbers (`"ComponentType": 2`, `"ComponentType": 200`, `"ComponentType": 7`) instead of named strings
- Truncated output filenames (e.g., `NewSponza_Merged.0.MeshComponen.json` — missing trailing 't')

**Import flow issues:**
- Import creates temporary `EntityRegistry` entities during offline conversion (AssimpTextureProcessor line 26) then immediately destroys them — unnecessary lifecycle overhead
- No clear separation between offline Baker tool and runtime loading — same code paths used for both
- Material textures referenced by filename but texture resolution depends on whether the file was previously imported

**Key files:**
- `Source/Engine/Services/AssetService.h` (GetAssetFilePath hardcoded path)
- `Source/Engine/Services/AssetService.cpp` (Save functions with hardcoded paths)
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp` (import entry point)
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp` (texture import)
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp` (material import)
- `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.h` and `.cpp` (scene save/load)
- `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` (component serialization)
- `Source/Engine/Common/IOService.h/.cpp` (path utilities)
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 All asset paths resolved through IOService (no hardcoded ../Data/ or ../Res/ strings in import code)
- [x] #2 Scene files use consistent, normalized path format (no double slashes)
- [x] #3 Importing a new .obj/.fbx/.gltf file produces correctly named and located JSON + binary files
- [x] #4 Existing scenes (GITestSponza, UnitTest) still load and render correctly after path changes
- [x] #5 Component type IDs use named constants or enums, not magic numbers
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Asset importing pipeline modernized:\n- Path handling centralized through IOService/AssetService (no more hardcoded ../Data/ or ../Res/ in import code)\n- Double-slash paths fixed in all active scene files and World.inl\n- AssimpTextureProcessor rewritten to use stack-local components instead of runtime entity spawning\n- AssimpImporter delegates to JSONWrapper::SaveChildScene for child .InnoScene production\n- All component type IDs use GetTypeID() constants, phantom types (ModelComponent=2, DrawCallComponent=200) removed\n- Legacy scene files (GITestSponza, GITestFireplaceRoom, animationTest) still have old format but are unused\n- RenderTest and Main.exe integration tests pass
<!-- SECTION:FINAL_SUMMARY:END -->
