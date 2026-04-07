# Asset Import Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a professional, non-blocking workflow for importing 3D models into the InnocenceEngine with real-time progress feedback.

**Architecture:** A decoupled system where Electron handles file selection, the Engine performs asynchronous conversion via Assimp, and WebSocket IPC synchronizes progress states.

**Tech Stack:** C++, Assimp, Electron, Vue 3, Naive UI (NProgress).

---

### Task 1: Engine IPC Extensions

**Files:**
- Modify: `Source/Engine/Services/EditorService.cpp`
- Modify: `Source/Engine/Services/AssetService.h`
- Modify: `Source/Engine/Services/AssetService.cpp`

- [x] **Step 1: Add IMPORT_ASSET handler to EditorService**
- [x] **Step 2: Add Progress Callback to AssetService**
- [x] **Step 3: Update AssetService::Import to be reporting-aware**

### Task 2: Electron File Picker & IPC

**Files:**
- Modify: `Source/Editor-Next/main.js`
- Modify: `Source/Editor-Next/src/store.js`

- [x] **Step 1: Implement select-files in main.js**
- [x] **Step 2: Add import state to store.js**
- [x] **Step 3: Implement IMPORT_PROGRESS listener in AppLayout.vue**

### Task 3: Workspace UI Integration

**Files:**
- Modify: `Source/Editor-Next/src/components/AssetPanel.vue`
- Modify: `Source/Editor-Next/src/components/AppLayout.vue`

- [x] **Step 1: Add "Import Model" button to AssetPanel toolbar**
- [x] **Step 2: Add global Progress Overlay to AppLayout.vue**
- [x] **Step 3: Handle Import Completion**

### Task 4: UX Audit & Finalization

- [x] **Step 1: Run UX Audit for Import Workflow**
- [ ] **Step 2: Commit all specs and plans**
- [x] **Step 3: Final E2E Pass**
