---
id: TASK-63
title: 'Assimp importer: suspicious concatenated texture filename in Sponza materials'
status: To Do
assignee: []
created_date: '2026-04-18 11:33'
labels:
  - bug
  - asset-pipeline
  - assimp
  - sponza
dependencies: []
references:
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp
  - Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`NewSponza_Curtains_glTF.curtain_02.MaterialComponent.json` (and similar) has slot 2 (metallic) and slot 3 (roughness) both set to the same name `"NewSponza_Curtains_glTF.curtain_fabric_Roughnesscurtain_fabric_Metalness"` — two source filenames joined with no separator.

glTF stores metallic-roughness as a single packed texture (G=roughness, B=metallic), and Assimp can expose the same file under both `aiTextureType_METALNESS` and `aiTextureType_DIFFUSE_ROUGHNESS`. Somewhere between Assimp's per-type `GetTexture` returning the filename and our `CreateTextureComponent` writing the instance name, two distinct filenames are being welded together without a separator. Either:

- Assimp is returning the concatenation itself (unlikely but possible with a broken model), or
- Our loop over `aiTextureType` is misusing a static buffer / re-reading the same `aiString`, or
- The source asset's texture filename genuinely contains "Roughnesscurtain_fabric_Metalness" as its file basename (tail-end hypothesis; verify by listing the original gltf's embedded texture names).

Investigate by logging raw `l_AssString.C_Str()` for every `aiTextureType` iteration during a re-import, and cross-checking against the original `NewSponza_Curtains_glTF` gltf/bin files on disk.

Not blocking TASK-60 (already closed via shadow fix) but a real data-pipeline correctness bug.
<!-- SECTION:DESCRIPTION:END -->
