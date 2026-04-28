---
id: TASK-183
title: 'Runtime visualization modes — debug-output picker (replace #define-gated debug flags)'
status: To Do
assignee: []
created_date: '2026-04-28 17:30'
labels:
  - rendering
  - tooling
  - diagnostic
  - editor
dependencies: []
priority: high
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Engine/Services/DevToggleRegistry.h
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-28**: *"highest priority tasks, which should give us more tools/abilities to diagnose without reading code or cl again and again."*

Today's debug-visualization knobs are `#define`-gated in shader source (e.g. `DEBUG_INDIRECT_FROM_CACHE_ONLY`, `DEBUG_POINT_SHADOW_BYPASS`). To A/B them you edit a shader, recompile, redeploy. Every diagnostic question pays a ~30s rebuild cycle, multiplied by every bisect step. This is the gap that produced the recent material-bug thrash where the only path to "is GI the cause?" was a temp source patch.

### What this delivers

Runtime-toggleable **visualization modes** that replace `#define`-gated debug flags. Each is a `DevToggleRegistry` toggle (mirrors `GPUPathTracer`/`RasterizedGI`), exposed in the editor's render-toggles panel. The shader reads a small constant buffer or root constant, branches on the mode at runtime, writes the requested debug output to the swapchain.

Initial set (extend later as needs surface):

- **Direct lighting only** — bypass GI compose; show only direct-light contribution.
- **Indirect lighting only** — bypass direct-light add; show only GI compose.
- **GBuffer channels** — albedo, normal (RGB), roughness, metallic, motion-vectors as individual debug views.
- **Sun shadow visibility texture** — show `in_SunShadowRTVisibility` directly.
- **Per-light shadow visibility** — for the inline RT shadow rays, surface a per-pixel "is shadowed" debug output.
- **Tile light count heatmap** — show how many lights each tile culled to (validates LightCullingPass).

### Architecture

A single `int g_DebugViewMode` root constant or cbuffer field, set by an engine state mirror of a DevToggle ("DebugView") with enum values. `lightPass.comp` (or a dedicated debug-pass) branches on it before final write. No #define gating — pure runtime branch.

Editor-side: a dropdown in `RenderTogglesPanel.vue` (or a new `DebugViewPanel.vue`) listing the modes. Each mode change just sets the value via existing IPC.

### Why high priority

User's stated principle: diagnostic work should not require reading code/CL repeatedly. The `#define` rebuild cycle violates that.

This task pairs naturally with TASK-172 (full pass-bypass UI) and the panel-breakage investigation — all three are "diagnostics-without-rebuild" infrastructure.

### Owner

`rendering-researcher` (shader branching + cbuffer field) + `editor-tooling-expert` (UI). Producer to decompose.

### Out of scope

- No new visualization modes beyond the initial set listed above (avoid scope creep). Extension is a follow-up.
- No changes to existing `#define DEBUG_*` until their runtime equivalents are proven; deletion is a sequel.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `g_DebugViewMode` cbuffer field / root constant added; no #define gating
- [ ] #2 At least 6 visualization modes (direct-only, indirect-only, GBuffer R/G/B, shadow-visibility) selectable at runtime
- [ ] #3 Editor dropdown / panel exposes the modes; switching is instant (no recompile)
- [ ] #4 Existing #define debug flags removed once their runtime equivalents are verified (or filed for follow-up if dependencies prevent)
- [ ] #5 Visual A/B captures archived showing each mode's output
- [ ] #6 Peer review per discipline
<!-- AC:END -->
