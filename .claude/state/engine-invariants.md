# Engine invariants

Engine-internal facts the dispatcher anchors into briefs when filing or routing related work. Each entry is a load-bearing invariant that has bitten before; capture the file it applies to and the failure mode it prevents so a fresh dispatcher can cite it without re-deriving it.

## HID-driven scene loads must be async

`SceneService::Load()` calls that originate outside the render thread (HIDService button callbacks, any other main-thread context) must pass `AsyncLoad=true`. `LoadSync()` from a non-render-thread caller races the render thread's `IGraphicsService::Update()` and corrupts the DX12 resource state tracker, manifesting as a `tracker=UAV / D3D12-actual=SRV` barrier mismatch on the Final Blend Pass Result texture (e.g. after pressing R to reload a scene).

`LoadAsync()` sets `m_prepareForLoadingScene=true`; `SceneService::Update()` on the render thread picks it up the next frame and calls `LoadSync()` from the safe context. The auto-test path (loading from `World::Update()` inside the render thread callback) may remain synchronous because it already runs on the render thread.

Files: `Source/Engine/Services/HIDService.cpp`, `Source/Engine/Services/SceneService.{cpp,h}`. Anchor this invariant into any dispatch brief that touches HID-triggered scene flow.
