---
id: TASK-27
title: Enforce explicit "new vs. existing" semantics in AllocateMaterialAsset
status: Done
assignee: []
created_date: '2026-04-13 18:12'
updated_date: '2026-04-17 03:40'
labels:
  - architecture
  - explicit-contracts
  - asset-system
dependencies: []
references:
  - Source/Engine/Services/AssetService.cpp
  - Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
AllocateMaterialAsset silently returns an existing handle when a material with the same name already exists. Callers that perform destructive initialization (clear + repopulate) cannot distinguish a recycled handle from a fresh one, leading to the texture name accumulation bug (fixed in aef0f866) and any future bug of the same class.

**Structural weakness:** The engine conflates allocation with initialization. "Get or create" semantics are invisible at the call site. Callers must manually know whether the returned handle is new or recycled, and must defensively clear existing data — a contract that exists only in comments, if at all.

**Target improvement:** Make the return type communicate the allocation outcome. Options:
- Return `{handle, bool wasCreated}` so callers that need a clean slate can assert or react
- Add an explicit `ResetMaterialAsset(handle)` that callers invoke when they intend to reinitialize
- Make `AllocateMaterialAsset` always return a clean asset (clear existing data before returning) when called from a load path, removing the responsibility from the caller entirely

The chosen approach must eliminate the possibility of silent data accumulation without the caller having to know the asset's history.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):** Changed the return type of `AssetService::AllocateMaterialAsset` from `MaterialAssetHandle` to `MaterialAssetAllocation { m_Handle, m_WasNewlyCreated }`. Callers can no longer silently ignore the recycled-vs-fresh distinction — accessing the handle is now one struct field away, and the `m_WasNewlyCreated` flag sits right next to it.

All three call sites updated:
- `JSONSerializer_Components::Load(MaterialComponent)` — unconditionally resets `m_TextureNames` (the load path fully repopulates them), and resets `m_Attributes` when the slot was recycled. Removes the duplicate clear inside the TextureComponents branch.
- `AssimpMaterialProcessor::CreateMaterial` — when recycled, clears `m_TextureNames` and resets `m_Attributes` before `ProcessMaterialProperties` / `ProcessMaterialTextures` re-populate.
- `TemplateAssetService` default-material branch — clears `m_TextureNames` before `resize(5)` so a recycled template slot doesn't leak old entries.

The shared-by-name semantics of the allocator are preserved (two components referring to the same material name still share a slot); only the caller's responsibility to reset stale state is now explicit instead of invisible.

**Validation:** Build clean. RenderTest exit 0. Integration run (which exercises `JSONSerializer_Components::Load` twice: once for UnitTest then once for GISponza) completes without new errors; TASK-52 TDR at frame 8 unchanged as expected.
<!-- SECTION:NOTES:END -->
