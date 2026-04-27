---
id: TASK-157
title: 'Delete CSM orphan data: CSMConstantBuffer + activeCascade + maxCSMSplits (post TASK-138)'
status: Done
assignee:
  - low-level-expert
created_date: '2026-04-27 14:30'
updated_date: '2026-04-27 13:20'
labels:
  - cleanup
  - engine
  - dead-code
dependencies: []
references:
  - Source/Engine/Common/GPUDataStructure.h
  - Source/Engine/Services/RenderingConfigurationService.h
  - Source/Engine/Services/RenderingConfigurationService.cpp
  - Source/Engine/Services/PerFrameDataService.cpp
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
- [x] #1 `CSMConstantBuffer` struct removed from GPUDataStructure.h
- [x] #2 `PerFrameConstantBuffer::activeCascade` field removed; shader-side `padding_a` reviewed
- [x] #3 `maxCSMSplits` removed from RenderingConfigurationService.h (and its default initializer)
- [x] #4 Grep zero hits for `CSMConstantBuffer`, `activeCascade`, `maxCSMSplits`, `NR_CSM_SPLITS` in tracked source
- [x] #5 Build green; smoke exit 0
<!-- AC:END -->

## Implementation Notes
<!-- SECTION:IMPL_NOTES:BEGIN -->
**Landed 2026-04-27 by low-level-expert.**

### Changes
- `Source/Engine/Common/GPUDataStructure.h` — deleted `CSMConstantBuffer` struct entirely; renamed `PerFrameConstantBuffer::activeCascade` → `padding_a` to mirror the shader-side rename (TASK-138 phase 2). Cleaned up the `PointShadowConstantBuffer` docstring's stale reference to `CSMConstantBuffer`'s alignment idiom.
- `Source/Engine/Services/RenderingConfigurationService.h` — deleted `RenderingCapability::maxCSMSplits` field.
- `Source/Engine/Services/RenderingConfigurationService.cpp` — deleted the `m_renderingCapability.maxCSMSplits = 4;` initializer.
- `Source/Engine/Services/PerFrameDataService.cpp` — renamed the `activeCascade = 0` zero-init to `padding_a = 0`; collapsed the multi-line ABI-justification comment into one line.

### Shader-side `padding_a` decision (AC #2)

**Kept.** The shader-side `padding_a` in `Source/Shaders/HLSL/common/common.hlsl` is retained. Reasoning:

1. **HLSL CB layout** (16-byte rows). Row 26 is currently `aperture, shutterTime, ISO, padding_a` (4 × 4 bytes = full row), then row 27 is `radianceCacheJitter_x, radianceCacheJitter_y, frameIndex, modelCount`. Row 28 is the `exposureMode/auto…` block.
2. **C++ `Vec2 radianceCacheHaltonJitter`** is 8 bytes with 4-byte natural alignment (no `alignas` on `TVec2<float>`). With `padding_a` retained on the HLSL side, the C++ `padding_a` field at offset 12 of row 26 keeps both layouts byte-identical: `Vec2.x` lands at offset 16 (start of row 27) on both sides.
3. **Without `padding_a`**, the HLSL scalar packing would land `radianceCacheJitter_x` at offset 12 of row 26 and `_y` at offset 16 of row 27 — which would still match a C++ `Vec2` at offset 12 because C++ `Vec2` doesn't enforce 16-byte alignment. So technically dropping both works mathematically. But:
   - Cross-subtree edit: `Source/Shaders/` is owned by `rendering-researcher`, not `low-level-expert`. The task explicitly authorizes coordination but doesn't require it.
   - HLSL CB packing rules differ subtly between scalar and vector members, and DXC versions; an explicit `padding_a` slot is the defensive choice.
   - Net-zero size impact (4-byte slot either way; struct is 512B total either way).
4. **C++ side rename** (`activeCascade` → `padding_a`) kills the dead semantic name (AC #2 satisfied — no field by that name exists) while preserving the ABI exactly. The byte at offset 28 of row 26 is now explicitly named "padding" on both sides.

### Verification
- Grep across `Source/`: zero hits for `CSMConstantBuffer`, `activeCascade`, `maxCSMSplits`, `NR_CSM_SPLITS` outside of `Source/Shaders/HLSL/WIP/` (off the build path, allowed by task spec).
- `Scripts/BuildWin.ps1` exit 0; engine + RenderTest both built; mirror-semantic DXIL deploy completed (no manual nuke needed per TASK-146).
- `Bin/RelWithDebInfo/Main.exe -total_frames 60` exit 0; capture written; clean shutdown.
<!-- SECTION:IMPL_NOTES:END -->
