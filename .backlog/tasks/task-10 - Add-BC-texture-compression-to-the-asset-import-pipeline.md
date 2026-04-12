---
id: TASK-10
title: Add BC texture compression to the asset import pipeline
status: To Do
assignee: []
created_date: '2026-04-12 11:50'
labels:
  - rendering
  - assets
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Currently the Assimp import pipeline resolves texture paths at import time and loads raw images via STBWrapper. The engine has no BC (block-compressed) texture support, so every texture is stored and uploaded as uncompressed RGBA8/RGBA16F. This is prohibitive for large PBR scenes like GISponza.

**Context**
- Textures are sourced from `../OriginalAssets/` (outside Bin/), resolved via `modelBaseDir + fileName` after the texture-path fix.
- Generated MaterialComponent JSON files store texture names; TextureComponents are loaded at runtime via `AssetService`.
- The GPU-facing path goes: STBWrapper::Load → TextureDesc → D3D12/Vulkan texture upload.

**Work**
1. Integrate a BC compressor (DirectXTex on Windows, or `stb_dxt.h` for a lighter option) into the import pipeline — compress during `AssimpTextureProcessor::CreateTextureComponent`, output `.dds` files alongside the model's generated assets.
2. Store the `.dds` path (not the raw source path) in the MaterialComponent JSON so the runtime loads the pre-compressed file.
3. Update `STBWrapper` / add a `DDSWrapper` to load `.dds` files at runtime; populate `TextureDesc` with the correct compressed format (`DXGI_FORMAT_BC*`).
4. Choose format by usage slot: BC5 for normal maps (slot 0), BC1/BC7 for albedo (slot 1), BC1 for metallic/roughness (slots 2–3), BC4 for AO (slot 4).
5. Mipmaps should be generated and stored in the DDS during compression.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Importing a glTF/FBX model produces .dds files for each texture alongside the .json components
- [ ] #2 MaterialComponent JSON references the .dds path, not the raw source path
- [ ] #3 Runtime texture load reads the .dds and uploads with the correct BC format to the GPU
- [ ] #4 Normal maps use BC5; albedo uses BC7 (or BC1 if size budget is tight); metallic/roughness use BC1; AO uses BC4
- [ ] #5 Mipmaps are present in the DDS and uploaded to the GPU
- [ ] #6 RenderTest and Main.exe integration tests pass after the change
<!-- AC:END -->
