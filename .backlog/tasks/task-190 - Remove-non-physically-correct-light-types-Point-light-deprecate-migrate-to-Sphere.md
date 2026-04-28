---
id: TASK-190
title: 'Remove non-physically-correct light types (Point light → deprecate, migrate to Sphere)'
status: To Do
assignee: []
created_date: '2026-04-28 18:50'
labels:
  - rendering
  - lighting
  - architecture
  - cleanup
dependencies: []
priority: medium
references:
  - Source/Engine/Component/LightComponent.h
  - Source/Engine/Services/LightDataService.cpp
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Data/ExampleProject/Components/
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28 after visual A/B with TASK-184 working light editor:** *"the point light seems to be not great at generating samples, but i understand it's because it's not physically correct, let's add a task to remove all non-physically correct light types."*

### Why

Point light is a Dirac-delta light source — zero area, infinite precision in direction. This is fundamentally incompatible with:

- **Area-sampling shadow rays** (TASK-176 inline RT, TASK-138 sun shadows) — there is no surface to sample on a zero-area light, so the only "sample" is the deterministic ray toward the light's center. Penumbra is impossible.
- **Cone-jitter soft shadows** — a point has no angular extent. Cone half-angle = 0 → cone jitter collapses to a single direction.
- **GI evaluation with finite light extent** — radiance-cache and PT NEE both want to sample over the light's surface for variance reduction. Point light forces a Dirac-delta evaluation that doesn't compose with stochastic sampling.

The visible artifact is "point lights produce poor samples" — sharp, noisy direct lighting that doesn't match the rest of the engine's stochastic sampling pipeline.

### What stays (physically correct)

- **Sphere light** — has finite radius; can sample on its surface; cone-jitter half-angle = `asin(saturate(radius / distance))`.
- **Sun (directional) light** — conventional approximation as "infinitely-far light with small angular extent." Authored as a half-angle (e.g. 0.27° for the Sun's actual angular size). Treated like a sphere light at infinite distance for sampling purposes.
- **Future: rect / disk / area lights** — physically correct; would be added under TASK-179 territory.

### What to remove

- **Point light** (`LightType::Point` or equivalent) — deprecate the type; remove from authoring options; delete per-type evaluation code.
- **Spot light**, IF currently implemented as cone-restricted Point → either remove or restate as cone-restricted Sphere with a finite radius. Determine during scoping.

### Migration

Existing GISponza scenes have point lights authored. Migration:

1. Convert each authored point light → small-radius sphere light (e.g. 0.05m or whatever matches the artistic intent — round-trip through the editor or a one-shot script).
2. Preserve position, color, intensity (luminous flux), cast-shadow flag.
3. Re-bake materials / scene file via the existing serialize round-trip (TASK-152 fixture-sweep machinery).

### What this delivers

1. **Component schema** — remove `LightType::Point` (and `LightType::Spot` if applicable) from the enum.
2. **Per-type buffers / cbuffer schemas** — collapse the point-light cbuffer into the sphere-light cbuffer. May simplify `PointLightConstantBuffer` ↔ `SphereLightConstantBuffer` into a single `PunctualLightConstantBuffer` if the sphere shape supersedes both.
3. **LightDataService** — delete the point-light upload path; route all "point-like" auth through the sphere path.
4. **`lightPass.comp` + `lightPassDirectLighting.hlsl`** — collapse the per-type evaluation; one path for sphere lights (with sun as the infinite-distance limiting case).
5. **Editor LightEditor.vue** — remove the Point option from the LightType picker.
6. **Scene data migration** — sweep `Data/ExampleProject/Components/*.LightComponent.json` for `LightType: Point` (or whatever the int constant is) entries; convert in place. Verify via serialize-test.
7. **Test coverage** — entity-property-symmetry round-trip for the migrated lights; light-editor-roundtrip spec extended.

### Project invariants (anchor)

- **TASK-149** (`m_CastShadow` flag) — preserved across light types.
- **TASK-176** (inline RT shadow path) — preserved; sphere lights consume the same inline path with cone-jitter for soft shadows. Point→sphere migration MAY require the cone-half-angle code (TASK-179 territory) to land first; check ordering during scoping.
- **TASK-188** (K-mode editor toggle) — orthogonal; LightType enum change shouldn't disturb the K-mode path.

### Owner

Multi-agent. Producer to decompose. Likely:

- **`software-architect`** — component schema + scene migration (LightType enum, JSON migration, serialize-test).
- **`rendering-researcher`** — shader + render-pass collapse (lightPass.comp + lightPassDirectLighting.hlsl + LightDataService cbuffer schema).
- **`editor-tooling-expert`** — LightEditor.vue picker simplification + IPC writer drop.

### Why medium priority

Real architectural cleanup; eliminates a class of "samples poorly" rendering surprise. Not user-blocking today (point lights work, just not great). Pairs naturally with TASK-179 (sphere shadow + extended light shapes) — TASK-179 may want to land first or in parallel since it owns the sphere-light cone-jitter logic that point→sphere migration depends on.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 LightType::Point (and Spot if applicable) removed from `LightComponent.h` enum
- [ ] #2 PointLightConstantBuffer collapsed into SphereLightConstantBuffer (or unified PunctualLightConstantBuffer)
- [ ] #3 LightDataService point-light upload + slot-allocator paths deleted; sphere path absorbs the use case
- [ ] #4 lightPass.comp + lightPassDirectLighting.hlsl per-type-Point branches deleted; sphere/sun unified evaluation
- [ ] #5 LightEditor.vue Point option removed from LightType picker
- [ ] #6 Scene data migrated: all GISponza / GITestBox / UnitTest LightComponent.json `LightType: Point` → `LightType: Sphere` with appropriate radius
- [ ] #7 entity-property-symmetry + light-editor-roundtrip spec coverage for migrated lights
- [ ] #8 60-FPS bar maintained (rendering-researcher manifest)
- [ ] #9 Visual cross-check: GISponza windowed before/after — sphere lights produce equivalent or better lighting than the deleted point lights
- [ ] #10 Peer review per discipline (multi-agent decomposition; multiple CLs likely)
<!-- AC:END -->
