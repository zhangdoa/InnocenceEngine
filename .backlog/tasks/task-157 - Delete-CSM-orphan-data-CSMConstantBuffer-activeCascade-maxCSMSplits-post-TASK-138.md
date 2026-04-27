---
id: TASK-157
title: 'Delete CSM orphan data: CSMConstantBuffer + activeCascade + maxCSMSplits (post TASK-138)'
status: To Do
assignee: []
created_date: '2026-04-27 14:30'
labels:
  - cleanup
  - engine
  - dead-code
dependencies: []
references:
  - Source/Engine/Common/GPUDataStructure.h
  - Source/Engine/Services/RenderingConfigurationService.h
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by rendering-researcher during TASK-138 phase 2 closure, 2026-04-27.**

TASK-138 swapped the sun CSM+PCSS path for hardware-RT shadows and removed every consumer of the CSM data. The data definitions themselves were left in engine-common headers because deletion crosses agent ownership boundaries (low-level-expert owns the engine common layer).

### Orphan data to delete

- `Source/Engine/Common/GPUDataStructure.h::CSMConstantBuffer` — entire struct.
- `Source/Engine/Common/GPUDataStructure.h::PerFrameConstantBuffer::activeCascade` — single field; was used by per-cascade dispatch indexing in the old CSM rasterize path. The shader-side replacement was a `padding_a` field per phase 2 — coordinate so both sides drop the slot together.
- `Source/Engine/Services/RenderingConfigurationService.h::maxCSMSplits` — capacity constant; unused after CSM removal.

### Verification

- Grep each symbol across the repo before deletion; confirm zero remaining consumers in tracked source.
- Build engine + shaders post-deletion to confirm no symbol leak.
- The shader-side `padding_a` field that replaced `activeCascade` should also be revisited — if no other use emerges, drop the padding entirely (subject to alignment requirements).

### Why low priority

Harmless dead data. No runtime cost. Cleanup hygiene only.

### Owner

`low-level-expert` — owns `Source/Engine/Common/` (GPUDataStructure.h) and engine services (RenderingConfigurationService.h). May coordinate with `rendering-researcher` if shader-side `padding_a` adjustment requires it.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `CSMConstantBuffer` struct removed from GPUDataStructure.h
- [ ] #2 `PerFrameConstantBuffer::activeCascade` field removed; shader-side `padding_a` reviewed
- [ ] #3 `maxCSMSplits` removed from RenderingConfigurationService.h (and its default initializer)
- [ ] #4 Grep zero hits for `CSMConstantBuffer`, `activeCascade`, `maxCSMSplits`, `NR_CSM_SPLITS` in tracked source
- [ ] #5 Build green; smoke exit 0
<!-- AC:END -->
