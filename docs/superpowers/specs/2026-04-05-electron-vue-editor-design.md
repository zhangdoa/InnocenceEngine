# Spec: Electron-Vue Editor Sidecar Architecture

## Overview
A modern, responsive, and decoupled editor for the **InnocenceEngine**. This architecture separates the high-level "Authoring" logic (UI, Asset browsing, Undo/Redo) into a web-based process, while the heavy-duty rendering and simulation remain in the native C++ engine.

## Core Features (Phase 1-3)
- **Zero-Copy Viewport:** Engine renders to a D3D12 texture; the shared handle is passed to Electron and rendered via `VideoFrame` on a `<canvas>`.
- **Dock-Based UI:** Using `dockview-vue` for a professional, IDE-style layout with resizable and rearrangeable panels.
- **WebSocket IPC:** Low-latency JSON-based communication bridge between UI and Engine.
- **Scene Hierarchy:** Real-time synchronization of the engine's entity registry.
- **Asset Browser:** Native file system integration for browsing engine data.

## Panels
1.  **Hierarchy:** Lists all entities in the current scene. Supports selection and filtering.
2.  **Property Editor:** Displays components and editable fields for the selected entity.
3.  **Asset Browser:** Grid-based view of the `Data/` directory.
4.  **Viewport:** High-performance render output from the engine sidecar.

## Communication Protocol (JSON)
| Message Type | Direction | Payload | Description |
| :--- | :--- | :--- | :--- |
| `HELO` | UI -> Engine | - | Initial handshake |
| `HELLO_REPLY` | Engine -> UI | `sharedHandle`, `width`, `height` | Handshake response with texture info |
| `GET_SCENE` | UI -> Engine | - | Request scene hierarchy |
| `SCENE_DATA` | Engine -> UI | `entities: [...]` | Full list of entities |
| `GET_ENTITY_DETAILS`| UI -> Engine | `id` | Request component data for entity |
| `ENTITY_DETAILS` | Engine -> UI | `details: { components: [...] }` | Component data for selected entity |

## Technology Stack
- **Engine Side:** C++, IXWebSocket (Server), rapidjson/nlohmann-json.
- **UI Side:** Electron 41+, Vue 3, Vite, Dockview, TailwindCSS (optional).
