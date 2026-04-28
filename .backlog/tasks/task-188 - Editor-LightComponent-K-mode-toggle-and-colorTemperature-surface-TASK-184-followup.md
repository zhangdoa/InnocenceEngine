---
id: TASK-188
title: 'Editor: surface LightComponent K-mode toggle and colorTemperature (TASK-184 follow-up)'
status: To Do
assignee: []
created_date: '2026-04-28 18:25'
labels:
  - editor
  - lighting
  - feature
dependencies:
  - TASK-184
priority: medium
references:
  - Source/Engine/Services/EditorService.cpp
  - Source/Editor-Next/src/components/inspector/LightEditor.vue
  - Source/Engine/Component/LightComponent.h
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-184 review ADVISORY (editor-tooling-expert peer, 2026-04-28).**

TASK-184's IPC color-handler fix (`EditorService.cpp:609-619`) silently sets `m_UseColorTemperature = false` whenever the user edits the RGB color, so the every-frame `LightSimulationService` K→RGB overwrite stops winning. **But**: there's no editor surface to flip K-mode back on. Once the user touches color, the light is RGB-mode permanently from the editor's perspective; only a source-asset reload (`.InnoScene` re-load) restores K-mode.

GISponza ships every light with `UseColorTemperature: true` and a meaningful K (warm 2500-6500K range). The artistic workflow presumes K-temperature lighting is reachable; today it is one-way-out from the editor.

### What this delivers

1. **GET payload extension** — `EditorService.cpp:308-318` (LightComponent serializer) emits `useColorTemperature : bool` and `colorTemperature : float`.
2. **UPDATE_ENTITY_PROPERTY writers** — handlers for both new fields. `useColorTemperature` flips the flag; `colorTemperature` writes the K value (which `LightSimulationService::Update()` then re-derives RGB from on the next frame).
3. **`LightEditor.vue` widgets** — `n-checkbox` for K-mode + `n-input-number` (gated on the checkbox) for K value. Layout consistent with the existing intensity / cast-shadow rows.
4. **`entity-property-symmetry.spec.js`** writable-light-fields pin extended for the two new fields.
5. **`light-editor-roundtrip.spec.js`** — additional case for the K-mode toggle round-trip.

### Why medium priority

Real artistic-workflow gap (K-mode is the authored default for half the lights), but workaround exists (edit the source `.InnoScene` JSON). Pairs naturally with TASK-184 — same surface, same discipline.

### Owner

`editor-tooling-expert`. Coordinates with `software-architect` if `LightComponent` schema needs adjustment (probably not — fields already exist, only the editor surface is missing).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 GET_ENTITY_DETAILS LightComponent payload includes `useColorTemperature` + `colorTemperature`
- [ ] #2 UPDATE_ENTITY_PROPERTY handlers for both new fields land at `EditorService.cpp` (mirror existing `intensity` / `color` shape)
- [ ] #3 LightEditor.vue exposes K-mode checkbox + K-temp input (gated on checkbox)
- [ ] #4 entity-property-symmetry.spec.js writable-light-fields pin extended
- [ ] #5 light-editor-roundtrip.spec.js K-mode round-trip case added; live-engine spec green
- [ ] #6 Peer review per discipline
<!-- AC:END -->
