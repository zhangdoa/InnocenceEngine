---
id: TASK-22
title: >-
  Implement standalone texture import pipeline for PBR texture sets and color
  checker
status: To Do
assignee: []
created_date: '2026-04-13 09:35'
labels:
  - textures
  - asset-pipeline
  - scene
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
DownloadAssets.ps1 now downloads AmbientCG PBR texture sets (Concrete007, Ground037, Metal032, Tiles074) and a Macbeth Color Checker PNG into OriginalAssets/Textures/. These need to flow through the engine's import pipeline (STBWrapper load → BC compression → TextureComponent JSON + binary) to become usable in scene MaterialComponents.

Current gap: AssetService::Import() only handles model files (OBJ/FBX/glTF) through AssimpWrapper. Standalone PNG textures have no import path.

Work needed:
1. Add `AssetService::ImportTexture(path, slot)` that:
   - Loads PNG via STBWrapper
   - Compresses to the appropriate BC format for the given slot (BC5/BC1/BC4 per slot convention)
   - Saves TextureComponent JSON + binary under Generated/Components/
2. Add a World.inl trigger (e.g. key 'I') that calls ImportTexture for each PNG in OriginalAssets/Textures/
3. Create MaterialComponent files for each imported texture set referencing the baked TextureComponent names
4. Add scene entities in UnitTest.InnoScene for:
   - 4 textured spheres (one per PBR set) in a new row at Z=24
   - 1 color-checker cube (UnitCubeMesh, scale 1×1×0.01 flat) placed as ground reference at Z=0

References:
- OriginalAssets/Textures/ (populated by DownloadAssets.ps1)
- Source/Engine/Services/AssetService.cpp — existing Import() for models
- Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp — BC compression logic to reuse
<!-- SECTION:DESCRIPTION:END -->
