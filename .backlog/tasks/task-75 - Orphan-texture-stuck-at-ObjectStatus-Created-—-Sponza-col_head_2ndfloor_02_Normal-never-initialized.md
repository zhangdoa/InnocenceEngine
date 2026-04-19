---
id: TASK-75
title: >-
  Orphan texture stuck at ObjectStatus::Created — Sponza
  col_head_2ndfloor_02_Normal never initialized
status: Done
assignee: []
created_date: '2026-04-18 19:24'
closed_date: '2026-04-19 04:05'
labels:
  - bug
  - asset
  - textures
  - not-reproducible
dependencies: []
priority: low
---

## Resolution (2026-04-19): no longer reproducible

Verified with verbose logging on a fresh GISponza load (`-loglevel 0 -total_frames 20`) and on a scene-reload run (`-total_frames 30 -reload_at_frame 15`):

- `col_head_2ndfloor_02_Normal` log trail: queued → InitializeComponents picks it up → CreateSRV succeeds → "is initialized."
- 238 texture initialisations on a single load, 253 across the reload cycle, **zero failures, zero stuck**.

Likely fixed as a side effect of the TASK-74 work that reorganised the deferred init queue (`EnqueueBinaryLoad` → background thread → `Initialize` → `InitializeComponents`) and added per-frame `RefreshMaterialTextureIndices` so that any future late-arriving texture is tolerated rather than gating the rebuild. Closing as not-reproducible — re-open with a fresh log if it surfaces again.

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
While debugging TASK-74 I found that `NewSponza_Main_glTF_003.col_head_2ndfloor_02_Normal` is registered in `TextureResourceService`'s pool (so `Find()` returns a valid TextureComponent) but its `m_ObjectStatus` stays at `Created` forever — never promoted to `Activated`. The `.innobin` and `.json` for it exist on disk, so it was imported. Appears `TextureResourceService::Initialize` is never called for it, or the deferred-init queue never drains it.

This was the single texture that blocked a first-attempt "wait for all textures Activated" gate in GPUPathTracerPass from ever passing. Harmless in TASK-74's final fix (per-frame refresh tolerates it) but indicates a real asset-pipeline hole: an orphaned texture component that imports successfully but never becomes GPU-usable.

Investigate:
- Which mesh/material references this texture? Is it referenced but its mesh/material fails to trigger its init?
- Does `MaterialResourceService::Initialize` skip queuing a texture when it shouldn't?
- Log the deferred queue size at scene-load boundaries and cross-reference with texture count on disk.

Likely a small fix once the missing-queue-insert site is found.
<!-- SECTION:DESCRIPTION:END -->
