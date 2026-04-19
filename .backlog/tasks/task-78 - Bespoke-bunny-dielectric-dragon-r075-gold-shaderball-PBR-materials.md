---
id: TASK-78
title: Bespoke materials for bunny (dielectric), dragon (r=0.75 gold), and ShaderBall (PBR set)
status: Done
assignee: []
created_date: '2026-04-19 09:30'
closed_date: '2026-04-19 10:00'
labels:
  - assets
  - materials
  - example-project
  - rendering
dependencies:
  - TASK-22
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Replace the share-by-import materials currently bound to the GISponza bunny, dragon, and ShaderBall entities with bespoke `MaterialComponent` JSONs under `Data/ExampleProject/Components/`. Each entity gets its own scalar attribute set (or texture set, for the shader ball) so visual debugging of materials is independent of the import pipeline that built the meshes.

## Per-entity targets

- **Bunny** — pure dielectric. AlbedoR/G/B `0.7 0.7 0.7`, Alpha `1`, Metallic `0`, Roughness `0.1`, AO `1`, Thickness `1`. No textures.
- **Dragon** — gold, slightly rough. AlbedoR/G/B match the gold tinting used in the BRDF tests (~ `1.0 0.766 0.336`), Metallic `1`, Roughness `0.75`, AO `1`. No textures.
- **ShaderBall** — PBR-textured, mirroring the `UnitTest` scene's material wiring style (texture per slot: normal, albedo, metallic, roughness) but pointing at one of the AmbientCG sets that `DownloadAssets.ps1` fetches into `OriginalAssets/Textures/` (Concrete007 / Ground037 / Metal032 / Tiles074 — pick one; Metal032 is the obvious match for a shader ball). Requires TASK-22 to have landed the standalone PNG → BC import path.

## Scope

- New `MaterialComponent` JSONs under `Data/ExampleProject/Components/` (e.g. `GISponza.Bunny.MaterialComponent.json`, `GISponza.Dragon.MaterialComponent.json`, `GISponza.ShaderBall.MaterialComponent.json`).
- Update the corresponding entity entries in the GISponza scene file (or wherever each entity is currently composed) to reference the new material instead of whatever Assimp-imported material they inherit today.
- For ShaderBall: depend on TASK-22 to first turn the AmbientCG PNG sets into TextureComponent JSONs + binaries.

## Non-goals

- Material editor UI in Editor-Next (separate concern; AC #7 of TASK-62 is the umbrella for world / property editing).
- Adding new mesh assets — bunny / dragon / shader ball meshes already exist.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Bunny renders as a dielectric sphere-like surface (no metallic highlights) in both raster and path-traced GISponza captures
- [ ] #2 Dragon renders as gold with roughness ≈ 0.75 in both pipelines
- [ ] #3 ShaderBall renders with one PBR set's normal + albedo + metallic + roughness textures sampled correctly
- [ ] #4 Each entity's material lives in `Data/ExampleProject/Components/` as a checked-in `MaterialComponent` JSON
- [ ] #5 No regression in the GISponza autotest (Tier 2)
<!-- AC:END -->
