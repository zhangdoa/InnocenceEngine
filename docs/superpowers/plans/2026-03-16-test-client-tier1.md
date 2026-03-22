# TestClient Tier 1 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `Main.exe -test -offscreen` that exits with a meaningful exit code (0=pass, 1=GPU error, 2=crash) so AI can run the full GPU pipeline and conclude pass/fail autonomously.

**Architecture:** Three independent changes wired together — (1) GPU error flag set by the D3D12 debug callback, surfaced via a new virtual on `IRenderingServer`; (2) WinMain reads that flag on exit and returns the appropriate code; (3) `TestClient` subclasses `IRenderingClient` and drives termination after a fixed frame count, activated by `-test` in `InitConfig`.

**Tech Stack:** C++17, DX12, MSBuild RelWithDebInfo. Build: `"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" "C:/GitRepo/InnocenceEngine/Build/InnocenceEngine.sln" //p:Configuration=RelWithDebInfo //m //v:minimal 2>&1 | tail -5`. Run: `cd /c/GitRepo/InnocenceEngine/Bin && timeout 20 ./RelWithDebInfo/Main.exe -test -offscreen -loglevel 0 2>&1 | grep -iE "\[Error\]|\[Warning\]|Engine has been terminated"; echo "Exit: $?"`

---

## Chunk 1: GPU Error Flag

**Files:**
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_GraphicsDevice_Private.cpp` (lines 73–80)
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer.h`
- Modify: `Source/Engine/RenderingServer/IRenderingServer.h`

- [ ] **Step 1: Add the atomic flag and set it in the debug callback**

In `DX12RenderingServer_GraphicsDevice_Private.cpp`, at the top of the file (after existing includes, before any functions), add:

```cpp
static std::atomic<bool> g_GPUErrorDetected{false};
```

Inside `D3D12DebugMessageCallback`, in the `case D3D12_MESSAGE_SEVERITY_CORRUPTION:` / `case D3D12_MESSAGE_SEVERITY_ERROR:` block (lines 73–80), add `g_GPUErrorDetected.store(true);` before the existing `Log(Error, ...)` call:

```cpp
case D3D12_MESSAGE_SEVERITY_CORRUPTION:
case D3D12_MESSAGE_SEVERITY_ERROR:
    g_GPUErrorDetected.store(true);
    // ... existing log code unchanged ...
```

- [ ] **Step 2: Add HasGPUError() virtual to IRenderingServer**

In `Source/Engine/RenderingServer/IRenderingServer.h`, in the `IRenderingServer` class public section, add alongside other virtual methods:

```cpp
virtual bool HasGPUError() const { return false; }
```

- [ ] **Step 3: Add HasGPUError() override to DX12RenderingServer**

In `Source/Engine/RenderingServer/DX12/DX12RenderingServer.h`, in the `DX12RenderingServer` class public section, add:

```cpp
bool HasGPUError() const override;
```

In `DX12RenderingServer_GraphicsDevice_Private.cpp` (same TU as `g_GPUErrorDetected`), add the implementation after the existing functions:

```cpp
bool DX12RenderingServer::HasGPUError() const
{
    return g_GPUErrorDetected.load();
}
```

- [ ] **Step 4: Build to verify**

Run the build command. Expected: `Main.vcxproj -> ...\Main.exe` in last 5 lines, no errors.

- [ ] **Step 5: Commit**

```bash
cd /c/GitRepo/InnocenceEngine
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer_GraphicsDevice_Private.cpp \
        Source/Engine/RenderingServer/DX12/DX12RenderingServer.h \
        Source/Engine/RenderingServer/IRenderingServer.h
git commit -m "feat: add GPU error flag propagation via IRenderingServer::HasGPUError()"
```

---

## Chunk 2: WinMain Exit Codes

**Files:**
- Modify: `Source/Engine/Platform/WinMain/WinMain.cpp` (lines 102, 117, 122, 127–129)

- [ ] **Step 1: Make UnhandledExceptionHandler exit with code 2**

In `WinMain.cpp`, replace the return statement in `UnhandledExceptionHandler` (line 102):

```cpp
// Before:
return EXCEPTION_EXECUTE_HANDLER;

// After:
ExitProcess(2);
return EXCEPTION_EXECUTE_HANDLER; // unreachable, satisfies return type
```

- [ ] **Step 2: Wrap WinMain body and propagate exit code**

Replace the `WinMain` function body (lines 113–129) so that setup/init failures return `2` (unrecoverable errors) and the normal exit checks the GPU error flag:

```cpp
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    try
    {
        std::unique_ptr<Engine> m_pEngine = std::make_unique<Engine>();

        if (!m_pEngine->Setup(hInstance, nullptr, pScmdline))
            return 2;

        if (!m_pEngine->Initialize())
            return 2;

        m_pEngine->Run();

        m_pEngine->Terminate();

        if (m_pEngine->getRenderingServer()->HasGPUError())
            return 1;

        return 0;
    }
    catch (...)
    {
        return 2;
    }
}
```

Note: `getRenderingServer()` returns `IRenderingServer*` — the call is safe even in headless mode because `HeadlessRenderingServer` inherits the default `HasGPUError()` returning `false`.

- [ ] **Step 3: Build to verify**

Run the build command. Expected: no errors, `Main.vcxproj -> ...\Main.exe` in last 5 lines.

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Platform/WinMain/WinMain.cpp
git commit -m "feat: propagate GPU error and crash to WinMain exit codes (0/1/2)"
```

---

## Chunk 3: TestClient + InitConfig + Engine::Setup

**Files:**
- Create: `Source/Client/TestClient/TestClient.h`
- Create: `Source/Client/TestClient/TestClient.cpp`
- Modify: `Source/Engine/Engine.h` (line 32 — add `bool isTest = false;` to `InitConfig`)
- Modify: `Source/Engine/Engine.cpp` (line 304 area — parse `-test`; line 413–415 — runtime branch)
- Modify: `Build/Source/Client/Main/Main.vcxproj` — add new .cpp to the build

- [ ] **Step 1: Create TestClient.h**

```cpp
#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
    class TestClient : public IRenderingClient
    {
    public:
        bool Setup(ISystemConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool PrepareCommands() override;
        bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

    private:
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;
        uint32_t m_FrameCount = 0;
        uint32_t m_FramesAfterLoad = 0;
        std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
        bool m_ShouldTerminate = false;
    };
}
```

- [ ] **Step 2: Create TestClient.cpp**

```cpp
#include "TestClient.h"
#include "../../Engine/Common/CommonMacro.inl"
#include "../../Engine/ComponentManager/SceneService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

static constexpr uint32_t k_MinFramesAfterLoad = 10;
static constexpr float    k_TimeoutSeconds      = 30.0f;

bool TestClient::Setup(ISystemConfig*)
{
    m_ObjectStatus = ObjectStatus::Activated;
    Log(Success, "TestClient: Setup complete.");
    return true;
}

bool TestClient::Initialize()
{
    m_StartTime = std::chrono::steady_clock::now();
    Log(Success, "TestClient: Initialized. Will terminate after ", k_MinFramesAfterLoad, " frames post-load or ", k_TimeoutSeconds, "s timeout.");
    return true;
}

bool TestClient::Update()
{
    if (m_ShouldTerminate)
        g_Engine->Terminate();
    return true;
}

bool TestClient::PrepareCommands()
{
    return true;
}

bool TestClient::ExecuteCommands(IRenderingConfig*)
{
    ++m_FrameCount;

    auto elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - m_StartTime).count();

    if (elapsed >= k_TimeoutSeconds)
    {
        Log(Warning, "TestClient: Timeout reached (", elapsed, "s). Terminating.");
        m_ShouldTerminate = true;
        return true;
    }

    bool isLoading = g_Engine->Get<SceneService>()->IsLoading();
    if (!isLoading)
        ++m_FramesAfterLoad;

    if (m_FramesAfterLoad >= k_MinFramesAfterLoad)
    {
        Log(Success, "TestClient: Completed ", m_FrameCount, " total frames, ",
            m_FramesAfterLoad, " post-load. Terminating cleanly.");
        m_ShouldTerminate = true;
    }

    return true;
}

bool TestClient::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestClient::GetStatus()
{
    return m_ObjectStatus;
}
```

- [ ] **Step 3: Add isTest to InitConfig**

In `Source/Engine/Engine.h`, in `struct InitConfig` (around line 32), add after `bool isOffscreen = false;`:

```cpp
bool isTest = false;
```

- [ ] **Step 4: Parse -test in Engine::ParseInitConfig**

In `Source/Engine/Engine.cpp`, in `Engine::ParseInitConfig` near the `-offscreen` parsing block (around line 304), add:

```cpp
if (l_arg == "-test")
{
    Log(Success, "Launch in test mode: TestClient will drive termination.");
    l_result.isTest = true;
}
```

- [ ] **Step 5: Add runtime branch in Engine::Setup**

In `Source/Engine/Engine.cpp`, at line 413, replace:

```cpp
// Before:
if (!m_pImpl->m_initConfig.isHeadless)
{
    m_pImpl->m_RenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();
```

With:

```cpp
if (!m_pImpl->m_initConfig.isHeadless)
{
    if (m_pImpl->m_initConfig.isTest)
        m_pImpl->m_RenderingClient = std::make_unique<TestClient>();
    else
        m_pImpl->m_RenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();
```

Also add the include at the top of `Engine.cpp` alongside existing client includes:

```cpp
#include "../Client/TestClient/TestClient.h"
```

- [ ] **Step 6: Add TestClient.cpp to the Main project**

Open `Build/Source/Client/Main/Main.vcxproj` and add `TestClient.cpp` to the `<ClCompile>` item group, following the same pattern as the existing client entries:

```xml
<ClCompile Include="..\..\..\Source\Client\TestClient\TestClient.cpp" />
```

Also add the header to `<ClInclude>`:

```xml
<ClInclude Include="..\..\..\Source\Client\TestClient\TestClient.h" />
```

- [ ] **Step 7: Build**

Run the build command. Expected: no errors, `Main.vcxproj -> ...\Main.exe` in last 5 lines.

- [ ] **Step 8: Run and verify exit code 0**

```bash
cd /c/GitRepo/InnocenceEngine/Bin
timeout 35 ./RelWithDebInfo/Main.exe -test -offscreen -loglevel 0 2>&1 | grep -iE "\[Error\]|\[Warning\]|TestClient|terminated"
echo "Exit: $?"
```

Expected:
- Log line: `TestClient: Completed N total frames, 10 post-load. Terminating cleanly.`
- Log line: `Engine has been terminated.`
- `Exit: 0`

- [ ] **Step 9: Update CLAUDE.md test command**

In `CLAUDE.md`, replace the graphics validation command:

```
# Before:
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen" 2>&1

# After:
cd /c/GitRepo/InnocenceEngine/Bin && timeout 35 ./RelWithDebInfo/Main.exe -test -offscreen -loglevel 0 2>&1 | grep -iE "\[Error\]|\[Warning\]|TestClient|terminated"; echo "Exit: $?"
```

- [ ] **Step 10: Commit**

```bash
cd /c/GitRepo/InnocenceEngine
git add Source/Client/TestClient/TestClient.h \
        Source/Client/TestClient/TestClient.cpp \
        Source/Engine/Engine.h \
        Source/Engine/Engine.cpp \
        Build/Source/Client/Main/Main.vcxproj \
        CLAUDE.md \
        docs/superpowers/specs/2026-03-16-test-client-tier1-design.md \
        docs/superpowers/plans/2026-03-16-test-client-tier1.md
git commit -m "feat: add TestClient and exit code support for autonomous GPU testing"
```
