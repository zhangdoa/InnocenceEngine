# Engine invariants

## HID-driven scene loads must be async

`SceneService::Load()` must pass `AsyncLoad=true` whenever the caller is (a) outside the render thread, or (b) on the render thread mid-frame after pass commands have been recorded. Sync `LoadSync()` from either context races the render thread's `IGraphicsService::Update()` and corrupts the DX12 resource state tracker (`tracker=UAV / D3D12-actual=SRV` mismatch on Final Blend Pass Result).

`LoadAsync()` sets `m_prepareForLoadingScene=true`; `SceneService::Update()` picks it up next frame.

Files: `Source/Engine/Services/HIDService.cpp`, `Source/Engine/Services/SceneService.{cpp,h}`.

### Known `SceneService::Load()` call sites

| Site | Thread | Required `AsyncLoad` |
|---|---|---|
| `World.inl:253` (`WorldSystem::Initialize`) | Main, pre-render-thread | `true` |
| `World.inl:283` (auto-GISponza switch) | Render thread, FMS callback | `true` |
| `World.inl:291` (auto-reload-at-frame) | Render thread, FMS callback | `true` |
| `EditorService.cpp:498` (LOAD_SCENE WS handler) | WebSocket worker | `true` |
| `ImGuiWrapper.cpp:296` ("Load scene" button) | Render thread, GUIService::Update | `true` |

No sync `Load()` callers remain.

## HIDService::m_ButtonEvents access is shared_mutex-synchronised

`m_ButtonEvents` is read on the engine tick (`HIDService::Update`, `HIDService::ButtonStateCallback`) and written on the logic-client thread (`HIDService::AddButtonStateCallback`). Without synchronisation the reader's `unordered_map::find` torn-reads mid-rehash and AVs at `0xC0000005` (TASK-217 / TASK-218).

Synchronisation: `mutable std::shared_mutex m_ButtonEventsMutex`. Readers take `std::shared_lock`; writer takes `std::unique_lock`. Reader path snapshots matching events under shared_lock and dispatches `ExecuteEvent` outside the critical section (MSVC SRW-backed `std::shared_mutex` does not allow shared→unique upgrade on the same thread; same constraint as `Engine::singletons_mutex_`).

`m_MouseMovementEvents` has the same write/read split but is not currently observed to AV (callbacks only mutate during single-shot Setup). If a future caller registers mouse callbacks mid-session, extend the same pattern.

Files: `Source/Engine/Services/HIDService.{cpp,h}`.
