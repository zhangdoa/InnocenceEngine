---
id: TASK-27
title: Enforce explicit "new vs. existing" semantics in AllocateMaterialAsset
status: To Do
assignee: []
created_date: '2026-04-13 18:12'
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
