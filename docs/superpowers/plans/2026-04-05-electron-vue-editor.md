# Electron-Vue Editor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkboxes.

## Overview
This plan implements the Sidecar Editor architecture, enabling a modern web-based UI to control the native InnocenceEngine.

## Tasks

### Phase 1: Engine Integration & Shared Memory

- [x] **Task 1: Engine "Sidecar" Mode**
- [x] **Task 2: Electron + Vue 3 Boilerplate**
- [x] **Task 3: EditorService Interface**
- [x] **Task 4: Shared Handle for DX12**

### Phase 2: Inter-Process Communication (IPC)

- [x] **Task 5: WebSocket Server in Engine**
- [x] **Task 6: Handshake Protocol**

### Phase 3: Resource Sharing & Layout

- [x] **Task 7: DX12 Shared Texture in Electron**
- [x] **Task 8: Dock-Based Panel System**

### Phase 4: Migration of Features (Current)

- [x] **Task 9: Scene Hierarchy Syncing**
    - [x] Step 1: Implement `GET_SCENE` in `EditorService`.
    - [x] Step 2: Implement `HierarchyPanel` with real-time entity list.
- [x] **Task 10: Property Editor Syncing**
    - [x] Step 1: Implement `GET_ENTITY_DETAILS` in `EditorService`.
    - [x] Step 2: Serialize `TransformComponent` and `LightComponent`.
    - [x] Step 3: Implement `PropertyPanel` with two-way binding.
- [x] **Task 11: Real Asset Browser**
    - [x] Step 1: Implement `AssetPanel` using Node.js `fs` module to browse `Data/`.
- [ ] **Task 12: Viewport Refinement & Input**
    - [ ] Step 1: Fix shared texture stability/crashes.
    - [ ] Step 2: Capture mouse/keyboard in `ViewportPanel` and forward to `HIDService`.

### Phase 5: Polish & Distribution

- [ ] **Task 13: Generic Component Reflection**
    - [ ] Step 1: Leverage `Reflector` tool to generate JSON metadata for all components.
    - [ ] Step 2: Auto-generate UI fields in `PropertyPanel` based on metadata.
- [ ] **Task 14: Distribution**
    - [ ] Step 1: Add `electron-builder` for standalone EXE.
