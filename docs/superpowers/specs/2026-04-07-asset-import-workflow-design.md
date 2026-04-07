# Spec: Asset Import Workflow with Progress Tracking

**Goal:** Implement a professional, non-blocking workflow for importing 3D models into the InnocenceEngine, featuring real-time progress feedback.

## Architecture
1.  **Frontend (Electron):**
    *   `AssetPanel` contains an "Import" button.
    *   `main.js` handles the native file picker via `dialog.showOpenDialog`.
    *   `store.js` tracks `importQueue` and `importProgress` (0-100%).
    *   A global `n-modal` or `n-notification` displays the current progress.
2.  **IPC Bridge:**
    *   `IMPORT_ASSET`: Electron -> Engine (Path to raw file).
    *   `IMPORT_PROGRESS`: Engine -> Electron (Current percentage, filename).
    *   `IMPORT_FINISHED`: Engine -> Electron (Final status, new .InnoModel path).
3.  **Backend (Engine):**
    *   `EditorService` parses `IMPORT_ASSET` and calls `AssetService::Import`.
    *   `AssetService` runs the conversion in a background task via `TaskScheduler`.
    *   `AssimpWrapper` is updated to report progress back to `AssetService` (which notifies `EditorService`).

## Tech Stack
*   **Engine:** C++, Assimp, WebSocket (IXWebSocket).
*   **Editor:** Electron, Vue 3, Naive UI (Progress component).

## UX-First Considerations
*   **Immediate Feedback:** As soon as a file is selected, the progress bar appears.
*   **Non-Blocking:** The user can continue to browse the scene while the model converts in the background.
*   **Error Handling:** Clear notification if the conversion fails (invalid format, corrupt file).
*   **Multi-File:** Support selecting multiple files in one dialog.
