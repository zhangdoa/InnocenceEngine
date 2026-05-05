# Engine invariants

Engine-internal facts the dispatcher anchors into briefs when filing or routing related work. Each entry is a load-bearing invariant that has bitten before; capture the file it applies to and the failure mode it prevents so a fresh dispatcher can cite it without re-deriving it.

## HID-driven scene loads must be async

`SceneService::Load()` calls must pass `AsyncLoad=true` whenever the caller is either (a) outside the render thread (HIDService button callbacks, WebSocket worker, any other non-render-thread context), OR (b) on the render thread but fired mid-frame after pass commands have already been recorded for the current frame (e.g. ImGui button callbacks dispatched from `GUIService::Update`, FMS upload-heap callback callers in `WorldSystem::Update`). `LoadSync()` from either context races the render thread's `IGraphicsService::Update()` and corrupts the DX12 resource state tracker, manifesting as a `tracker=UAV / D3D12-actual=SRV` barrier mismatch on the Final Blend Pass Result texture (e.g. after pressing R to reload a scene).

`LoadAsync()` sets `m_prepareForLoadingScene=true`; `SceneService::Update()` on the render thread picks it up the next frame and calls `LoadSync()` from the safe context. Sync `Load()` is only safe from a render-thread caller fired *before* any pass commands have been recorded for the current frame (the historical auto-test path was an example, but no remaining callers meet this condition — see the call-site table below).

Files: `Source/Engine/Services/HIDService.cpp`, `Source/Engine/Services/SceneService.{cpp,h}`. Anchor this invariant into any dispatch brief that touches HID-triggered scene flow.

### Known `SceneService::Load()` call sites

Audit reference — keep current when adding or moving call sites. Grep: `SceneService.*->Load\(` under `Source/`.

| Site | Thread context | Required `AsyncLoad` | Rationale |
|---|---|---|---|
| `Source/ExampleProject/LogicClient/World.inl:253` (`WorldSystem::Initialize`) | Main thread (LogicClient::Initialize, before render thread is serialised into FMS) | `true` | Pre-FMS startup; LoadSync would race the render thread coming online. (TASK precedent: `f4e0252a`.) |
| `Source/ExampleProject/LogicClient/World.inl:283` (`WorldSystem::Update` auto-GISponza switch) | Render thread (FMS upload-heap callback) | `true` | Mid-frame trigger after pass commands recorded; defer to next-frame `SceneService::Update` boundary. |
| `Source/ExampleProject/LogicClient/World.inl:291` (`WorldSystem::Update` auto-reload-at-frame) | Render thread (FMS upload-heap callback) | `true` | Same as above. |
| `Source/Engine/Services/EditorService.cpp:498` (`LOAD_SCENE` WebSocket handler) | WebSocket worker thread (ix::WebSocket dispatch) | `true` | Non-render-thread caller; documented in-place. |
| `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiWrapper.cpp:296` ("Load scene" ImGui button) | Render thread (`GUIService::Update` inside FMS) | `true` | ImGui callback fires mid-frame after pass commands recorded; sync LoadSync races the DX12 resource-state tracker. (TASK-215.) |

No sync `Load()` callers remain. If a new call site must be sync (render-thread context, before any pass commands have been recorded that frame), document the rationale in-place at the call site.
