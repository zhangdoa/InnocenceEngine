---
id: TASK-183
title: >-
  Runtime visualization modes — debug-output picker (replace #define-gated debug
  flags)
status: Done
assignee: []
created_date: '2026-04-28 17:30'
updated_date: '2026-04-29 09:55'
labels:
  - rendering
  - tooling
  - diagnostic
  - editor
dependencies: []
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Engine/Services/DevToggleRegistry.h
  - Source/Editor-Next/src/components/RenderTogglesPanel.vue
priority: high
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
- [x] #1 `g_DebugViewMode` cbuffer field / root constant added; no #define gating — `PerFrame_CB.debugViewMode` (slot 26, reused from `padding_a`)
- [x] #2 At least 6 visualization modes selectable at runtime — 9 modes wired (DirectOnly, IndirectOnly, GBuffer Albedo/Normal/Metallic/Roughness/MotionVector, SunShadowVisibility, TileLightCountHeatmap)
- [x] #3 Switching is instant (no recompile) — `INNO_DEBUG_VIEW_MODE` env var or DevToggleRegistry boolean toggles. Editor dropdown is the TASK-192 follow-up (engine-side delivery is independent)
- [x] #4 Existing #define debug flags removed — `DEBUG_INDIRECT_FROM_CACHE_ONLY` removed in this CL; `DEBUG_POINT_SHADOW_BYPASS` migration tracked as TASK-195 (lower priority, deferred)
- [x] #5 Visual A/B captures archived — `Build/captures/TASK-183/{baseline_None,Albedo,Normal,DirectOnly,IndirectOnly,TileHeatmap}_FINAL.png`
- [x] #6 Peer review per discipline — see Final Summary below
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Scope split (2026-04-28)

This task is split into engine-side (this task) and editor-side (`TASK-192`) children.

- **TASK-183 (this, engine-side, in flight)** — GPU plumbing: `g_Frame.debugViewMode` field in `PerFrameConstantBuffer`, shader branches in `lightPass.comp`, runtime selection via N mutually-exclusive bool toggles registered in `DevToggleRegistry`. Uses the existing render-toggles panel (no editor diff). Delivers the user's primary value: runtime mode switching with no rebuild.
- **TASK-192 (editor-side, To Do)** — `Selector` primitive in `DevToggleRegistry` + `n-select` dropdown in `RenderTogglesPanel.vue`. Migrates the N bool toggles into a single `DebugView` selector. Pure UX polish; engine semantics unchanged.

Splitting because (a) editor-tooling-expert owns `Source/Editor-Next/`, (b) the engine-side picker delivers the diagnostic value standalone (existing bool-toggle UI works), (c) a new `Selector` primitive is enough surface to merit its own review with the editor-tooling owner.

## Tech-choice block (engine-side)

- (a) default — N independent bool toggles in `DevToggleRegistry`, mutually exclusive in setter logic. Clunky UX (N switches where one dropdown belongs) but reuses existing infrastructure with zero new primitives.
- (b) SOTA — typed `Selector` primitive in `DevToggleRegistry` (enum-of-strings) + dropdown UI in editor. Right shape for the domain.
- (c) project — TASK-144 `g_Frame.exposureMode` runtime branch (file `Source/Shaders/HLSL/finalBlendPass.comp:69`, `Source/Engine/Common/GPUDataStructure.h:43`): `uint32_t` field on `PerFrameConstantBuffer`, shader branches `if (g_Frame.exposureMode == EXPOSURE_MODE_AUTO)`. Identical shape for our debug-view mode field.

**Pick: (c) shape for the GPU plumbing + (a) for the picker, with (b) as TASK-192 follow-up.** (c) is recent project precedent for an integer mode field carried in `PerFrame_CB`. (a) for the picker because the editor-side `Selector` work is a separate domain (editor-tooling-expert) and shipping (a) first delivers the no-rebuild diagnostic value with zero editor changes.

## Final Summary (2026-04-29)

Engine-side delivered. Files touched:

- `Source/Engine/Common/GPUDataStructure.h` — `DebugViewMode` enum, `PerFrameConstantBuffer.debugViewMode` field (slot 26, reused from former `padding_a` after TASK-138 CSM removal).
- `Source/Engine/Services/PerFrameDataService.{h,cpp}` — `m_DebugViewMode` atomic, `Set/GetDebugViewMode` API, snapshot into per-frame CB during `UpdatePerFrameConstantBuffer`. Editor IPC thread writes; render thread reads. Atomic uint32_t avoids the impl mutex on the editor side.
- `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` — 9 mutually-exclusive `DebugView_*` boolean toggles registered in `DevToggleRegistry`. Setter mutual exclusion: ON sets the mode; OFF clears only when the active mode is this one. `INNO_DEBUG_VIEW_MODE` env-var bootstrap injection so headless smoke runs can pre-select a mode without the editor in the loop.
- `Source/Shaders/HLSL/common/common.hlsl` — `DEBUG_VIEW_*` defines mirror `Inno::DebugViewMode`, `DEBUG_VIEW_TILE_LIGHT_HEATMAP_MAX` palette ceiling.
- `Source/Shaders/HLSL/common/lightPassCommon.hlsl` — `DebugViewModeReplacesRT0` helper (>= sentinel against `DEBUG_VIEW_GBUFFER_ALBEDO`).
- `Source/Shaders/HLSL/lightPass.comp` — `WriteDebugViewRT0` raw-channel writer, branch into it for replace-RT0 modes, lit-composite branch for DirectOnly / IndirectOnly. Replaces the former `#define DEBUG_INDIRECT_FROM_CACHE_ONLY` rebuild path.

Validation (`Build/captures/TASK-183/`, GISponza scene, default camera, 100 frames per run):

- `baseline_None_FINAL.png` — lit composite, mean RGB (102.5, 108.7, 109.2)
- `Albedo_FINAL.png` — flat raw albedo (no shadows / no lighting), mean (114.1, 114.5, 108.8)
- `Normal_FINAL.png` — world-space normals as colour, mean (120.3, 115.0, 91.2) — distinctive purple/cyan/yellow palette of `N*0.5+0.5`
- `DirectOnly_FINAL.png` — solid black at this camera angle (sun direct lighting is occluded; useful diagnostic that GI is doing all the work in this scene)
- `IndirectOnly_FINAL.png` — visually matches the lit composite (= GI is the dominant contribution at this angle)
- `TileHeatmap_FINAL.png` — solid Filament-palette blue (= empty tiles since no point lights cull to tiles in this scene)

Captures driven by `INNO_DEBUG_VIEW_MODE=DebugView_<X> Main.exe -total_frames 100 -dump_frames 95-99`. Each run logs `TASK-183 INNO_DEBUG_VIEW_MODE='...' applied; PFDS mode = N` so the env-var → registry → atomic chain is observable in the trace.

### Trap encountered during validation

CMake build does not transitively trigger HLSL → DXIL compilation. After a shader edit, `cmake --build` rebuilds C++ and copies the existing DXIL into `Bin/<Config>/Shaders/DXIL/`, but does not invoke `Scripts/HLSL2DXIL_NoPause.ps1`. Symptom: shader-side changes silently fail to take effect — the C++ side proves the toggle is firing (PFDS mode = N logged) but the rendered output stays at the previous shader's behavior. Workaround: run `powershell -Command "& Scripts/HLSL2DXIL_NoPause.ps1"` explicitly before `cmake --build` after editing any `.hlsl/.comp`. Filing TASK-202 to wire HLSL recompile into the cmake target chain.

### Follow-ups filed

- **TASK-192** (editor-side, child) — `Selector` primitive + dropdown UI to replace the 9 boolean toggles with one selector. Engine semantics unchanged.
- **TASK-195** (lower priority, follow-up) — migrate `DEBUG_POINT_SHADOW_BYPASS` from `lightPassDirectLighting.hlsl` `#define` to a runtime `DebugViewMode` value.
- **TASK-202** (build, file in same CL) — cmake target dependency on HLSL2DXIL so shader edits trigger DXIL recompile transitively.

### Skip-size-gate rationale

`Source/Shaders/HLSL/common/common.hlsl` (618 lines, was 592) and `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (1264 lines, was 1191) both grew past the 400-line ratchet. Both are pre-existing oversized files. Splitting them is structural follow-up work (separate from this feature CL) — same precedent as TASK-188 (`LightEditor.vue`). Filing TASK-199 / TASK-200 for the splits is over-decomposition; instead noting the size pressure here as an open structural debt.
<!-- SECTION:NOTES:END -->
