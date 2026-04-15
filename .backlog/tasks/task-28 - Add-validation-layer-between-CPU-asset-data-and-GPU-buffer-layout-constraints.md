---
id: TASK-28
title: Add validation layer between CPU asset data and GPU buffer layout constraints
status: To Do
assignee: []
created_date: '2026-04-13 18:12'
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
