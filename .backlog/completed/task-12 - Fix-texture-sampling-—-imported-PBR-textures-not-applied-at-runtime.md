---
id: TASK-12
title: Fix texture sampling — imported PBR textures not applied at runtime
status: Done
assignee: []
created_date: '2026-04-12 18:35'
updated_date: '2026-04-12 21:21'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fixed the full PBR texture pipeline for runtime scene loading:

1. **NamedObjectPool key normalization** — stripped trailing `/` in `Allocate`/`Find` so `DrawCallService::Find(name)` matches pool entries created with `name + "/"`.
2. **Texture loading at runtime** — added `JSONWrapper::Load(MaterialComponent)` logic to load each referenced texture into `TextureResourceService` pool when a material is deserialized.
3. **Dedup fix** — changed dedup guard from `ObjectStatus::Activated` to a simple null check so already-enqueued (not yet GPU-activated) textures aren't loaded twice.
4. **Background binary loader** — added a dedicated `std::thread` in `TextureResourceService` draining a `ThreadSafeQueue<BinaryLoadRequest>`. Decouples heavy STB PNG decode (~680ms per 4K texture × 168) from the render-critical `TaskScheduler` thread pool; render loop starts immediately, textures stream in over ~115s.
5. **Concurrent import data race** — serialized all 5 parallel model imports behind a static mutex in `AssetService::Import` to prevent `std::string` corruption in shared `AssetServiceNS` globals.
6. **Texture slot index fix** — pre-sized `m_TextureNames` to 5 slots and assigned by `l_textureSlotIndex` (normal=0, albedo=1, metallic=2, roughness=3, AO=4) instead of appending. Existing generated `.json` files need a re-import (press Y in-engine) to take effect.
<!-- SECTION:FINAL_SUMMARY:END -->
