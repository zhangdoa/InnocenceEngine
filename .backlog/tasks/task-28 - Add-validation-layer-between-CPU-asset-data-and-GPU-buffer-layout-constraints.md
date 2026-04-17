---
id: TASK-28
title: Add validation layer between CPU asset data and GPU buffer layout constraints
status: Done
assignee: []
created_date: '2026-04-13 18:12'
updated_date: '2026-04-17 04:12'
labels:
  - architecture
  - explicit-contracts
  - gpu
  - asset-system
dependencies: []
references:
  - Source/Engine/Common/GPUDataStructure.h
  - Source/Engine/Services/DrawCallService.cpp
  - Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
MaxTextureSlotCount (currently 7) is a compile-time GPU layout constraint defined in GPUDataStructure.h and mirrored in HLSL. MaterialAsset::m_TextureNames is an unbounded std::vector. Nothing in the engine validates that a material's texture count fits within the GPU buffer layout before serialization into DrawCallService GPU buffers.

**Structural weakness:** The gap between "what an asset can contain" and "what the GPU buffer can hold" is completely unchecked. Silent truncation (with the defensive guard added in aef0f866) is better than overrun, but truncation without a warning means data is lost without feedback.

**Target improvement:**
- At asset load time (JSONWrapper or MaterialResourceService::Initialize), assert/log-error if texture count exceeds MaxTextureSlotCount
- In DrawCallService::UpdateDrawCalls, promote the silent truncation guard to a logged warning: "Material X has N textures but GPU layout supports only MaxTextureSlotCount; extra entries are ignored"
- Consider whether MaxTextureSlotCount should be enforced as a cap at the asset data layer rather than only at the GPU serialization layer

The goal is that any asset that exceeds GPU layout limits is immediately visible during development, not silently corrupted in production.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):** Closed the silent-truncation gap at both layers.

- `JSONSerializer_Components::Load(MaterialComponent)`: logs a warning at load time when the JSON declares more than `MaxTextureSlotCount` `TextureComponents`, naming the material so the offending asset is immediately visible.
- `DrawCallServiceImpl::UpdateDrawCalls`: logs a warning per-frame when `m_TextureNames.size() > MaxTextureSlotCount` on any material, naming the material and the actual count. The existing `for (... && j < MaxTextureSlotCount)` cap still applies (truncation behaviour preserved); the change is pure observability.

Deliberately kept `MaxTextureSlotCount` as a GPU-layer cap rather than an asset-layer cap: a material JSON with 9 textures is still well-formed data, and the engine may one day raise `MaxTextureSlotCount`; we don't want CPU-side truncation here to lose the original intent.

**Validation:** Build clean. RenderTest exit 0. Integration run on the 10-frame UnitTest→GISponza auto-test produces zero `GPU layout` warnings, confirming no current material currently exceeds the cap. The warnings will fire immediately if a future material does.
<!-- SECTION:NOTES:END -->
