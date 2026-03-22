# Test Client — Tier 1 Design Spec
Date: 2026-03-16

## Goal

Enable fully autonomous AI-driven validation: build → run → observe exit code → conclude pass/fail with log evidence. No human in the loop required for Tier 1.

## Scope

Tier 1 only: process exits cleanly with a meaningful exit code; D3D12 validation errors propagate to that code. No render output capture (Tier 2) or reference comparison (Tier 3).

## New Flag

`-test` added to `Main.exe` argument parsing in `Engine::ParseInitConfig`, stored as `InitConfig::isTest`. Activates `TestClient` in place of `DefaultRenderingClient` at engine setup time. Compatible with `-offscreen`.

Full autonomous test command:
```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Main.exe -test -offscreen -loglevel 0" 2>&1
```

Exit codes:
- `0` — clean exit, no GPU errors, termination conditions met
- `1` — D3D12 ERROR or CORRUPTION severity message detected
- `2` — unhandled exception / device removal / hard crash

## Components

### 1. `TestClient`

Location: `Source/Client/TestClient/TestClient.h/.cpp`

Subclasses `IRenderingClient` directly. Methods to implement (from `IRenderingClient` and its base `ISystem`): `Setup`, `Initialize`, `Update`, `PrepareCommands`, `ExecuteCommands(IRenderingConfig* renderingConfig = nullptr)`, `Terminate`, `GetStatus`.

Responsibilities:
- Increment frame counter each `ExecuteCommands()` call
- Track wall time from `Initialize()` using `std::chrono::steady_clock`
- Poll `g_Engine->Get<SceneService>()->IsLoading()` to detect the scene-loaded milestone
- Set a `bool m_ShouldTerminate` member flag when termination conditions are met (do NOT call `g_Engine->Terminate()` from inside the render callback — see §Termination below)

Termination conditions (evaluated each `ExecuteCommands()` call):
1. Scene has finished loading (`!g_Engine->Get<SceneService>()->IsLoading()`)
2. At least `k_MinFramesAfterLoad` frames rendered after load
3. OR wall time exceeds `k_TimeoutSeconds` (safety net — triggers regardless of load state)

Hardcoded constants (top of `.cpp`):
```cpp
static constexpr uint32_t k_MinFramesAfterLoad = 10;
static constexpr float    k_TimeoutSeconds      = 30.0f;
```

#### Termination

To avoid re-entrancy, `TestClient` does not call `g_Engine->Terminate()` from within `ExecuteCommands`. Instead it sets `m_ShouldTerminate = true`. `TestClient::Update()` (called from the engine run loop, outside the render callback) checks this flag and calls `g_Engine->Terminate()`.

### 2. Client Registration

`InitConfig` gains a new `bool isTest = false` field, set when `-test` is parsed in `Engine::ParseInitConfig`.

In `Engine::Setup`, the existing block that constructs `INNO_RENDERING_CLIENT` gains a runtime branch:
```cpp
if (m_pImpl->m_initConfig.isTest)
    m_pImpl->m_RenderingClient = std::make_unique<TestClient>();
else
    m_pImpl->m_RenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();
```

### 3. GPU Error Flag

Location: `DX12RenderingServer_GraphicsDevice_Private.cpp`

Add `std::atomic<bool> g_GPUErrorDetected{false}` with external linkage at file scope (declared `extern` in `DX12RenderingServer.h` so other TUs can read it). Set to `true` inside the existing `D3D12DebugMessageCallback` on `D3D12_MESSAGE_SEVERITY_ERROR` or `D3D12_MESSAGE_SEVERITY_CORRUPTION` — in addition to existing logging, no behavior change otherwise.

Expose via a new virtual method on `IRenderingServer` (avoids `WinMain` depending on any DX12 header):
```cpp
// IRenderingServer.h
virtual bool HasGPUError() const { return false; }

// DX12RenderingServer.h
bool HasGPUError() const override;

// DX12RenderingServer_GraphicsDevice_Private.cpp (same TU as g_GPUErrorDetected)
bool DX12RenderingServer::HasGPUError() const { return g_GPUErrorDetected.load(); }
```

### 4. Exit Code Propagation

Location: `Source/Engine/Platform/WinMain/WinMain.cpp`

Wrap the `wWinMain` body in a `try/catch(...)` block that returns `2` on any unhandled exception. After `m_pEngine->Terminate()` returns in the normal path:
```cpp
if (m_pEngine->getRenderingServer()->HasGPUError()) return 1;
return 0;
```

The existing `UnhandledExceptionFilter` is updated to call `ExitProcess(2)` before returning `EXCEPTION_EXECUTE_HANDLER` to guarantee exit code `2` on hard crashes that bypass the `try/catch`.

## Autonomous Test Loop (AI perspective)

1. Run the test command via Bash tool — stdout/stderr (engine log) captured inline
2. Check process exit code
3. `0` → pass, report success
4. `1` or `2` → scan captured log for `[Error]` lines, report root cause to user

## Out of Scope

- Tier 2: backbuffer capture / non-black pixel check
- Tier 3: reference image comparison
- Vulkan / Metal backends (DX12 only for now)
- TestClient running render passes (it drives termination only, not rendering)
