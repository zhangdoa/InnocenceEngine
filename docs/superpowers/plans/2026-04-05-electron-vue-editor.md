# Electron-Vue Editor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkboxes.

## Overview
This plan implements the Sidecar Editor architecture, enabling a modern web-based UI to control the native InnocenceEngine.

## Tasks

### Phase 1: Engine Integration & Shared Memory

- [x] **Task 1: Engine "Sidecar" Mode**
    - [x] Step 1: Add `Sidecar` to `EngineMode` enum in `Source/Engine/Engine.h`.
    - [x] Step 2: Update command line parsing for `-sidecar` flag in `Source/Engine/Engine.cpp`.
    - [x] Step 3: Run regression tests to ensure no regressions.
    - [x] Step 4: Commit.

- [x] **Task 2: Electron + Vue 3 Boilerplate**
    - [x] Step 1: Initialize NPM project in `Source/Editor-Next`.
    - [x] Step 2: Install `electron`, `vue`, `vite`.
    - [x] Step 3: Create basic `main.js` (spawn engine) and `App.vue`.
    - [x] Step 4: Commit.

- [x] **Task 3: EditorService Interface**
    - [x] Step 1: Create `EditorService` class in `Source/Engine/Services/EditorService.h`.
    - [x] Step 2: Implement stubs in `Source/Engine/Services/EditorService.cpp`.
    - [x] Step 3: Register service in `Source/Engine/Engine.cpp` (only if `engineMode == Sidecar`).
    - [x] Step 4: Update `Source/Engine/CMakeLists.txt` to include new files.
    - [x] Step 5: Commit.

- [x] **Task 4: Shared Handle for DX12**
    - [x] Step 1: Update `DX12FrameManagementService.cpp` to generate shared handles.
    - [x] Step 2: Commit.

### Phase 2: Inter-Process Communication (IPC)

- [ ] **Task 5: WebSocket Server in Engine**
    - [ ] Step 1: Add `ixwebsocket` (already in `Source/External/GitSubmodules/ixwebsocket`) to project.
    - [ ] Step 2: Implement `EditorService::Initialize()` to start a WS server on port 8081.
    - [ ] Step 3: Implement JSON message dispatcher.
    - [ ] Step 4: Commit.

- [ ] **Task 6: Handshake Protocol**
    - [ ] Step 1: Implement `HELO` message in Editor (Vue) to connect to Engine.
    - [ ] Step 2: Engine replies with `HELLO_REPLY` containing the `sharedHandle` address.
    - [ ] Step 3: Commit.

### Phase 3: Resource Sharing (GPU)

- [ ] **Task 7: DX12 Shared Texture in Electron**
    - [ ] Step 1: Use `electron-directx-sharing` or implement a small native node module to open the shared handle.
    - [ ] Step 2: Render the shared texture into a Vue component.
    - [ ] Step 3: Commit.

## Task Details (Selected)

### Task 4 Step 1: Shared Handle Generation
```cpp
// Source/Engine/Services/DX12/DX12FrameManagementService.cpp
#include <d3d12.h>
// ... in AssignSwapChainImages or similar ...
HANDLE sharedHandle = nullptr;
m_ctx->m_device->CreateSharedHandle(m_ViewportTexture.Get(), nullptr, GENERIC_ALL, nullptr, &sharedHandle);
```

- [x] **Step 2: Commit**
```bash
git add Source/Engine/Services/DX12/DX12FrameManagementService.cpp
git commit -m "feat: add shared handle generation for dx12"
```
