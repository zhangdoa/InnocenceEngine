---
id: TASK-182
title: 'Pass bypass leaves stale output — extend m_Bypassed with clear-on-bypass semantic'
status: To Do
assignee: []
created_date: '2026-04-28 16:30'
labels:
  - rendering
  - tooling
  - bug
dependencies: []
priority: medium
references:
  - Source/Engine/Interface/IRenderPass.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient_Bypass.inl
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28 after TASK-171 + RasterizedGI toggle (`a8cbda10`).**

TASK-171's `m_Bypassed` flag skips a pass's `PrepareCommandList` + `Execute` + downstream `WaitOnGPU`. **What it does NOT do**: clear the pass's output render targets. Consumers (e.g. LightPass sampling RadianceCache outputs) still read whatever was in those textures last frame, so bypassing the GI pass group leaves "frozen GI" on screen — visually the toggle looks like it didn't fully disable GI.

User's words: "looks like something is retained on the screen ... bypassing the pass on the CPU side is not enough since we're not only switching ray tracing pipelines."

### Root cause

Bypass is a CPU-side dispatch skip; the GPU resource state isn't touched. For passes whose downstream consumers expect "fresh content this frame OR a defined neutral value", bypass without clear is wrong. For passes whose downstream consumers expect "any valid content, even stale" (e.g. accumulation buffers that re-read their own last-frame value), bypass without clear is correct.

The two intents need to be distinguishable on the pass.

### Required fix — design space

1. **Sibling field `m_ClearOnBypass : bool`** on IRenderPass. When `m_Bypassed && m_ClearOnBypass`, the dispatch site (`DispatchOrBypass` / Execute helpers) issues a clear on the pass's output RTs instead of skipping work entirely. Default false (preserve existing TASK-171 behavior).

2. **Bypass-mode enum** instead of bool: `enum class BypassMode { None, Skip, ClearAndSkip, FreezeLastFrame };` replacing the current `std::atomic<bool>`. More flexible but bigger change; TASK-171 commits assume `bool`.

3. **Caller sets a clear color/value alongside the bypass flag** — e.g. `m_BypassClearColor : Vec4`. Lets RasterizedGI-style toggles produce "neutral GI" (all zeros) while preserving "freeze last frame" for accumulation passes.

Pick during scoping. Recommend (1) + a per-pass-author choice on whether their pass needs `m_ClearOnBypass=true`. The GI passes set the flag; sun-shadow accumulation buffer leaves it false.

### What this delivers

- Field/enum on IRenderPass.
- Dispatch-site honor in `DispatchOrBypass` (PrepareCommands phase) — issue a `ClearRenderTarget` (or equivalent) when m_ClearOnBypass is set.
- Or in `IsBypassed`/`Execute` phase — emit a clearing CL when bypass+clear, instead of fully skipping.
- The 8 GI passes (RadianceCacheRaytracing/Reprojection/FilterH/FilterV/Integration + GIDenoise + GIFilterH/GIFilterV) opt in to clear-on-bypass.

### Why medium priority

The current RasterizedGI toggle is partially functional — it stops the GI cost (FPS recovers further when toggled off), it just doesn't visually clear the contribution. For the user's immediate need (A/B inspection of shadows without GI noise), the workaround is to set `RasterizedGI=false` AND tolerate the brief flash of stale GI; once frames advance enough, the LightPass output stabilizes. Real fix is a quality-of-life upgrade.

### Owner

`graphics-api-expert` (dispatch-site clear logic, resource state hazards) + `rendering-researcher` (per-pass opt-in). Producer to decompose if scope grows.

### Out of scope

- No editor UI changes (TASK-172 owns that).
- No new clear-color authoring (use the existing render-target clear value or a sentinel).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Field/enum added to IRenderPass surfacing the clear-on-bypass intent
- [ ] #2 DispatchOrBypass / Execute honors clear-on-bypass: bypassed pass's RTs are cleared once at the boundary, not left stale
- [ ] #3 The 8 GI passes opt in; toggling RasterizedGI=false produces visually-clean LightPass output (no stale GI contribution)
- [ ] #4 Accumulation-style passes (sun shadow visibility, etc.) leave the flag false; existing TASK-171 behavior preserved
- [ ] #5 No new GBV ERROR / WARNING from the clear-state transition
- [ ] #6 Peer review per discipline
<!-- AC:END -->
