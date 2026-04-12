---
id: TASK-12
title: Fix texture sampling — imported PBR textures not applied at runtime
status: In Progress
assignee: []
created_date: '2026-04-12 18:35'
labels:
  - rendering
  - assets
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
After the texture import pipeline fixes (modelBaseDir threading, name construction, binary path), textures are now saved correctly to disk. However at runtime the Sponza and other imported models appear untextured — the PBR textures are not being sampled.

**Likely failure points to investigate:**
1. MaterialComponent JSON TextureNames array — are names correct after latest import?
2. Runtime load path: how AssetService/TextureResourceService loads TextureComponent from JSON and resolves the .innobin binary
3. STBWrapper::Load binary path resolution — does it also double-prepend the working directory (same bug as Save)?
4. Texture slot binding — are loaded textures being bound to the correct material slots in the GBuffer shader?
5. Use RenderDoc CLI capture to inspect texture bindings at draw time.

**Investigation entry points:**
- `Source/Engine/Services/AssetService.cpp` — Load(TextureComponent)
- `Source/Engine/ThirdParty/STBWrapper/STBWrapper.cpp` — Load()
- `Source/Engine/Services/TextureResourceService.cpp` — how TextureComponents become GPU resources
- `Data/Generated/Components/*.MaterialComponent.json` — check TextureComponents array is populated
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Sponza albedo, normal, metallic, roughness textures visibly applied in GISponza scene
- [ ] #2 RenderDoc confirms correct textures bound at GBuffer draw calls
- [ ] #3 No texture-related errors or warnings in engine log during GISponza load
- [ ] #4 RenderTest and Main.exe integration tests pass
<!-- AC:END -->
