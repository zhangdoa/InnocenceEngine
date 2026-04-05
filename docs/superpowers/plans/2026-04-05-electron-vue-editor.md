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

### Phase 3: Resource Sharing & Layout (Current)

- [x] **Task 7: DX12 Shared Texture in Electron**
- [x] **Task 8: Dock-Based Panel System**
    - [x] Step 1: Install `dockview-vue`.
    - [x] Step 2: Implement `HierarchyPanel` with engine syncing.
    - [x] Step 3: Implement `AssetPanel` with native FS access.
    - [x] Step 4: Implement `PropertyPanel` shell.
    - [x] Step 5: Implement `ViewportPanel` with shared texture rendering.

### Phase 4: Advanced Features (Next)

- [ ] **Task 9: Component Serialization**
    - [ ] Step 1: Implement generic component serialization in `EditorService`.
    - [ ] Step 2: Sync selected entity properties to `PropertyPanel`.
- [ ] **Task 10: Input Redirection**
    - [ ] Step 1: Capture mouse/keyboard in `ViewportPanel`.
    - [ ] Step 2: Forward events via WebSocket to `HIDService`.
- [ ] **Task 11: Scene Manipulation**
    - [ ] Step 1: Implement `UPDATE_ENTITY` message to change transforms from UI.
- [ ] **Task 12: Distribution**
    - [ ] Step 1: Add `electron-builder` for standalone EXE.
