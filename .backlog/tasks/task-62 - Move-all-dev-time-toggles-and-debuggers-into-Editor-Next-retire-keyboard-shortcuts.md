---
id: TASK-62
title: Move all dev-time toggles and debuggers into Editor-Next; retire keyboard shortcuts
status: To Do
updated_date: '2026-04-19 02:25'
assignee: []
created_date: '2026-04-18 10:25'
labels:
  - editor
  - architecture
  - ux
  - dev-tools
  - testability
dependencies: []
references:
  - Source/Editor-Next
  - Source/ExampleProject/LogicClient/World.inl
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Stop adding one-shot keyboard handlers (R/L/Y/B/T/J/C/I/...) for every dev-time feature. Move all of them — and the ImGui-based debuggers we have grown alongside — into **Editor-Next** (the existing Electron + Vue 3 + dockview-vue + Naive UI scaffold under `Source/Editor-Next`). Keys stay reserved for permanent runtime operations (movement, camera toggle, render-mode toggle). The editor is the place developers live.

**Why drop ImGui for these and use the editor instead.** ImGui-in-engine forces every dev-tool change to go through C++ rebuilds and lives in the same process as the renderer (so a renderer hang freezes the tools). The editor is a separate process that already speaks to the engine over WebSocket IPC, hosts a docking system (`dockview-vue`), and has Playwright wired in — meaning every dev tool we add immediately gets headless e2e / unit / smoke / regression coverage. ImGui has none of that.

## Editor scaffolding that already works

`Source/Editor-Next` is not a sketch — it is a working host:

- **Electron** main process (`main.js`) spawns `Main.exe -mode 2 -parent_pid <pid>` (sidecar mode) and brokers a WebSocket between renderer and engine.
- **Vue 3 + dockview-vue** dockable panel system (`AppLayout.vue` already declares Viewport, Outliner, Inspector, Workspace panels with full drag/dock/float behaviour).
- **Pinia-style stores** (`assetStore`, `connectionStore`, `sceneStore`, `uiStore`).
- **Naive UI** for widgets, **Catppuccin** theme switching.
- **IPC composable** (`useIpc.js`) routes engine messages: `SCENE_DATA`, `ENTITY_DETAILS`, `IMPORT_PROGRESS`, `IMPORT_FINISHED`, `HELLO_REPLY`. Add new message types here.
- **Playwright** smoke + UX-audit specs already exist (`tests/editor.spec.js`, `tests/ux-audit.spec.js`) — every new panel registers itself for snapshot + interaction coverage.

So the work is *not* "build an editor"; it is "fill in panes + extend the IPC vocabulary."

## Panels to add

Each item below replaces a pile of one-shot keys + ad-hoc ImGui windows. Each lands as one or more `.vue` files under `Source/Editor-Next/src/components/` plus its IPC message types, plus its Playwright spec.

### 1. Master Panel Manager (toggles every other panel)

A "Window" menu (or persistent left-rail button list) listing every available panel and whether it is currently docked / floating / hidden. Hooks into dockview's `addPanel` / `removePanel` API. Persists layout to `localStorage` so panel state survives reload.

### 2. Scene Picker

Lists every `*.InnoScene` under `Data/ExampleProject/Scenes/` (filesystem-driven via Electron `fs`, no IPC needed). Click a row → IPC `LOAD_SCENE { path }` → engine `SceneService::Load(path)`. **Retires `R → UnitTest`, `L → GISponza`, hardcoded GITestBox load.**

### 3. Concurrency / Task Debugger

Replaces the current ImGui task-debugger window (clunky UX). Renders the `Inno::Task` graph as a directed graph (e.g. `vis-network` or a flat-table Naive grid) with per-thread swim-lanes, task durations, and "alive last seen" timestamps. Engine streams a `TASK_GRAPH_FRAME` IPC message per N frames (configurable rate to keep the wire cheap).

### 4. Render Target Debugger

Replaces every `INNO_KEY_T / INNO_KEY_J / etc → toggle showing this RT as swap chain input` keymap. A pane with a dropdown of every named RenderPass + RT (sourced from a new `LIST_RENDER_TARGETS` IPC), plus channel mask (R/G/B/A), exposure scrub, and a sample-pixel readout. Selecting a target sends `SET_SWAPCHAIN_SOURCE { renderPassName, rtIndex }` to the engine; engine swaps the FinalBlend input pointer (or routes around FinalBlend entirely for raw RT view).

### 5. Per-Pass On/Off Toggles

Lists every pass registered with `RenderPassResourceService` with a checkbox. Toggling sends `SET_PASS_ENABLED { name, enabled }`. Engine respects the flag in `ExecuteCommands`. **Retires the `m_showTransparent`, `m_showVolumetric`, `m_GPUPathTracerActive` etc. boolean key flips.**

### 6. Asset Conversion / Import

Extends the existing `ImportModal.vue`. Surfaces both model imports (Assimp path) and standalone PBR / color-checker textures (TASK-22). Drag-and-drop a folder; preview before commit; per-file slot-type selection. **Retires `INNO_KEY_Y → convertModel` and the future `INNO_KEY_I` for textures.**

### 7. World Editing

Entity create / delete / rename, transform-gizmo manipulation in the viewport (translate / rotate / scale handles), per-component property editors that round-trip through IPC and rewrite the corresponding `*.json` on Save. Builds on existing `HierarchyPanel.vue` + `PropertyPanel.vue` + the inspector subdir.

## Engine-side changes

A small, well-scoped expansion of the IPC vocabulary in `useIpc.js` and a matching dispatcher on the engine side (`Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` and `Source/Engine/Services/SceneService.cpp` are the two surfaces today; this work makes the dispatcher first-class instead of ad-hoc):

| New IPC type | Direction | Purpose |
|---|---|---|
| `LOAD_SCENE { path }` | → engine | Scene picker |
| `LIST_SCENES` | ↔ | Optional engine-side enumeration if Electron `fs` access is restricted |
| `LIST_RENDER_TARGETS` | ↔ | RT debugger populates dropdown |
| `SET_SWAPCHAIN_SOURCE { pass, rtIndex }` | → engine | RT debugger view selection |
| `LIST_PASSES` | ↔ | Per-pass toggle population |
| `SET_PASS_ENABLED { name, enabled }` | → engine | Per-pass enable flip |
| `TASK_GRAPH_FRAME { tasks, threads }` | ← engine | Concurrency debugger feed |
| `IMPORT_TEXTURE { paths, slotMap }` | → engine | Asset import (TASK-22) |
| `ENTITY_CREATE / ENTITY_TRANSFORM / ENTITY_DELETE / ENTITY_RENAME` | → engine | World editing |
| `COMPONENT_UPDATE { entityId, type, payload }` | → engine | Property editor commits |

## Tests live in the editor (Playwright), not in your fingers

The point of moving off keyboard is that *every dev workflow becomes testable*:

- Scene-load smoke: open editor → click GISponza row → assert Viewport receives a `SCENE_DATA` with the expected entity count.
- Path-tracer toggle smoke: click "Enable GPU Path Tracer" in pass panel → screenshot Viewport at frame 30 → diff against baseline.
- Render-mode regression: cycle every RT in the RT-debugger dropdown → snapshot each → fail on diff.
- Asset import e2e: drag PBR set into ImportModal → assert `IMPORT_FINISHED` message → assert generated `.json` + `.innobin` files exist.
- Window-layout persistence: drag a panel out, reload, assert position restored.

These slot into `tests/editor.spec.js` (smoke) / `tests/ux-audit.spec.js` (visual diff) and replace the manual "press button, eyeball the result" loop entirely.

## Non-goals

- Replacing the in-engine ImGui *runtime overlay* used for in-process diagnostics during a hung-renderer scenario — that stays as a last-resort tool. Everything else moves out.
- Rebuilding the dockview / Naive UI / IPC layer — they exist and work; this task is panel content + message types.
- Vulkan-specific editor support — DX12 / Win32 first; the WebSocket layer is renderer-agnostic anyway.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Master "Window" panel toggles every other panel; layout persists across editor reload
- [x] #2 Scene Picker pane lists `Data/ExampleProject/Scenes/*.InnoScene` and loads on click; `R` / `L` / hardcoded `GITestBox.Load` removed from `World.inl` (existing AssetPanel filesystem browser surfaces every scene; `R` / `L` / `Y` keys deleted)
- [x] #3 Concurrency / Task Debugger pane visualises live task graph; the in-engine ImGui task-debugger window is removed
- [x] #4 Render Target Debugger pane lists every pass + RT and routes a chosen one to the swap chain; per-RT-as-swapchain-input keys removed (`H`, `G` deleted)
- [x] #5 Per-Pass Toggle pane enables/disables every registered pass; `m_showTransparent`, `m_showVolumetric`, `m_GPUPathTracerActive` (and equivalents) no longer have key bindings (`B`, `C` deleted; `T`, `J`, `V` deleted as dead code)
- [ ] #6 Asset Conversion / Import pane handles model + standalone-texture imports — single-file model import works via existing ImportModal; multi-file PBR-set drag-drop is the remaining gap
- [ ] #7 World Editing: entity create / delete / rename + transform gizmo round-trips to engine IPC and rewrites the entity's `*.json` on Save
- [x] #8 Engine IPC dispatcher refactored from ad-hoc handlers to a first-class message router (one place to register a `(type → handler)` mapping)
- [x] #9 Playwright specs cover smoke + visual-diff for every new panel; all green in CI (smoke specs for every shipped pane)
- [x] #10 `Source/ExampleProject/LogicClient/World.inl` no longer registers any `OneShot` `INNO_KEY_*` callback for dev-time features (only permanent gameplay / camera bindings remain) (`R`, `L`, `Y` removed; the `B`/`C`/`H`/`G`/`T`/`J`/`V` keys lived in `ExampleRenderingClient.cpp` and are also gone)
<!-- AC:END -->
