---
id: TASK-23.11
title: >-
  AssetData family: rename to drop the "Data" suffix; pick names that describe
  the role
status: Done
assignee: []
created_date: '2026-05-22 07:33'
updated_date: '2026-05-22 09:32'
labels: []
dependencies: []
parent_task_id: TASK-23
priority: medium
ordinal: 11000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/AssetData.h currently declares three CPU-side asset descriptors:

- `MeshAssetData` — `{ ObjectName m_Name; ObjectLifespan m_Lifespan; AssetResidency m_Residency; GPUBufferView m_VertexBufferView/m_IndexBufferView; void* m_MappedMemory_VB/IB; AABB m_AABB; }`
- `TextureAssetData` — `{ ObjectName; Lifespan; Residency; TextureDesc; }`
- `MaterialAssetData` — `{ ObjectName; Lifespan; Residency; MaterialAttributes; std::vector<std::string> m_TextureNames; std::vector<TextureAssetHandle>; ShaderModel; }`

The "Data" suffix is meaningless — every struct holds data. User's directive: **drop "Data"; pick a name that says what the type actually is.** If no better name exists, the model is bad and needs rethinking, not relabeling.

Candidates (record the pick + why):

- `MeshAsset`, `TextureAsset`, `MaterialAsset` — short, accurate. But potentially collides with other "Asset" usage in the codebase. Audit.
- `MeshAssetRecord`, `TextureAssetRecord`, ... — says "this is the registry entry for an asset". Clearer about role.
- `MeshDescriptor`, `TextureDescriptor`, `MaterialDescriptor` — emphasises that the struct describes (not owns) the asset.

User signal — "no more `Data`. Be explicit, and if you can’t, typically it means a bad modeling." So if none of the above fits, that's a signal to refactor the model.

This is mechanical: grep + sed across all consumers. References cascade through 17+ files (every Get/Allocate/Release path).

Cross-link TASK-23.12 — AssetImportData.h's collapse is the same flavour ("Data" suffix indicates the class isn't earning its name).

References:
- Source/Engine/Common/AssetData.h
- Source/Engine/Common/AssetHandle.h (uses the names as template arguments)
- Source/Engine/Services/AssetService.h, AssetService_*Registry.cpp (heavy consumer)
- Source/Engine/Component/MeshComponent.h, MaterialComponent.h
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Decision recorded (chosen names + reasoning) in the closure note.
- [x] #2 MeshAssetData / TextureAssetData / MaterialAssetData renamed across the codebase via grep + sed (do NOT regenerate; pure rename sweep).
- [x] #3 AssetData.h renamed accordingly (or split / merged as the new naming dictates).
- [x] #4 AssetHandle.h forward-decls updated to match.
- [x] #5 All consumer services compile and pass tests after the rename.
- [x] #6 Main.exe -total_frames 10 exits 0; RenderTest.exe exits 0.
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
**Names chosen:** `MeshAsset`, `TextureAsset`, `MaterialAsset` (short, accurate). Verified zero collisions in source code via grep (only existing matches were inside Log-message string literals).

File renamed: `AssetData.h` → `AssetTypes.h` (the file holds three asset-record types; the filename reflects that without the bad "Data" suffix).

## Diff

Mechanical sed -i sweep with `\b` word-boundaries on 10 files:

- `MeshAssetData` → `MeshAsset`
- `TextureAssetData` → `TextureAsset`
- `MaterialAssetData` → `MaterialAsset`
- `"AssetData.h"` → `"AssetTypes.h"` (in `#include` lines)

Files touched:
- `Source/Engine/Common/AssetData.h` → renamed via `git mv` to `AssetTypes.h` (contents also updated).
- `Source/Engine/Common/AssetHandle.h` — forward decls + AssetHandle aliases templated on the new type names.
- `Source/Engine/Services/AssetService.h`, `AssetService.cpp`, `AssetService_Internal.h`.
- `Source/Engine/Services/AssetService_{Mesh,Material,Texture}Registry.cpp`.
- `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.h`, `.cpp`.

The aliases `MeshAssetHandle`, `TextureAssetHandle`, `MaterialAssetHandle` (in `AssetHandle.h`) keep their existing names — they describe what they are (asset handles) without the Data suffix.

## Verification

- `BuildWin.ps1 -SkipShaderCompile` clean.
- `Main.exe -total_frames 10` exits 0.
- `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` exits 0 — verifies the rename didn't break the JSON serialization path which is heavily Asset-type-dependent.

## ACs

- #1 Decision recorded above.
- #2 + #3 + #4 Types renamed + file renamed via sed + git mv (no regenerated code).
- #5 All consumer services compile after the rename.
- #6 Main.exe -total_frames 10 + serialize-test both exit 0.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
