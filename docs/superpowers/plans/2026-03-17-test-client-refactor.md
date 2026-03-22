# TestClient Refactor & Rendering API Test Suite Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the intrusive TestClient/Engine coupling with a clean factory-injected client architecture, restructure the client folders, and introduce a proper rendering API test suite (`TestRenderingClient` + `TestLogicClient`) as a separate `RenderTest.exe` binary.

**Architecture:** `Engine` is decoupled from all concrete client types — it receives `IRenderingClient` and `ILogicClient` as injected `unique_ptr`s via `Engine::Setup`. Each executable (`Main.exe`, `RenderTest.exe`) links a factory implementation (`DefaultClientFactory`, `TestClientFactory`) that provides `CreateRenderingClient()` / `CreateLogicClient()`. `WinMain.cpp` is shared between both executables — it calls the factory and is otherwise generic. Test cases in `TestRenderingClient` are named after the specific IRenderingServer API they exercise (e.g., `draw_instanced`).

**Tech Stack:** C++17, DX12 via IRenderingServer, CMake, MSBuild, HLSL/DXIL shaders.

---

## File Map

### Deleted
- `Source/Client/ClientMetadata.h`
- `Source/Client/ClientMetadata.h.in`
- `Source/Client/TestClient/TestClient.h`
- `Source/Client/TestClient/TestClient.cpp`
- `Source/Client/TestClient/CMakeLists.txt`

### Renamed (entire directory)
- `Source/Client/` → `Source/DefaultClient/`
  - All internal paths (`RenderingClient/`, `LogicClient/`) unchanged relative to new root.

### Modified
- `Source/Engine/Engine.h` — remove INNO_* macros and client includes; add `testCase` to `InitConfig`; change `Setup()` signature to accept injected clients.
- `Source/Engine/Engine.cpp` — store injected clients instead of constructing them; remove `isTest` branch; remove `TestClient.h` include; fix `ParseInitConfig` to extract test case name string.
- `Source/Engine/Platform/WinMain/WinMain.cpp` — include `IClientFactory.h`; call `CreateRenderingClient()` / `CreateLogicClient()`; pass nullptr for both when headless.
- `Source/Engine/Platform/WinMain/CMakeLists.txt` — add `RenderTest` executable target (reuses `WinMain.cpp`); link both executables explicitly to their respective client libraries.
- `Source/CMakeLists.txt` — remove `INNO_RENDERING_CLIENT` / `INNO_LOGIC_CLIENT` variables; rename `Client` → `DefaultClient`; add `TestClient` subdirectory; remove Engine→client link lines.
- `Source/DefaultClient/CMakeLists.txt` (was `Source/Client/CMakeLists.txt`) — remove `configure_file` for `ClientMetadata`; add `DefaultClientFactory.cpp` to build; remove `TestClient` subdir.
- `Source/DefaultClient/RenderingClient/CMakeLists.txt` — rename library target from `${INNO_RENDERING_CLIENT}` to `DefaultRenderingClient`.
- `Source/DefaultClient/LogicClient/CMakeLists.txt` — rename library target from `${INNO_LOGIC_CLIENT}` to `DefaultLogicClient`.
- `CLAUDE.md` — update GPU validation command to use `RenderTest.exe`.

### Created
- `Source/Engine/Interface/IClientFactory.h` — declares `CreateRenderingClient()` and `CreateLogicClient()` free functions in `Inno` namespace.
- `Source/DefaultClient/DefaultClientFactory.cpp` — implements the factory returning `DefaultRenderingClient` / `DefaultLogicClient`.
- `Source/TestClient/CMakeLists.txt` — builds `TestRenderingClient` and `TestLogicClient` static libs.
- `Source/TestClient/TestRenderingClient.h` — `IRenderingClient` implementation; owns frame counter and test case dispatch.
- `Source/TestClient/TestRenderingClient.cpp` — implements `bareboot` (noop) and `draw_instanced` test cases.
- `Source/TestClient/TestLogicClient.h` — `ILogicClient` implementation; returns `"RenderTest"` as application name.
- `Source/TestClient/TestLogicClient.cpp` — stub implementation (no scene setup for initial cases).
- `Source/TestClient/TestClientFactory.cpp` — implements the factory returning `TestRenderingClient` / `TestLogicClient`.
- `Source/TestClient/Shaders/draw_instanced.vs.hlsl` — procedural triangle via `SV_VertexID`, no vertex buffer.
- `Source/TestClient/Shaders/draw_instanced.ps.hlsl` — outputs solid red.

---

## Task 1: Rename Source/Client → Source/DefaultClient

**Files:**
- Rename: `Source/Client/` → `Source/DefaultClient/`
- Modify: `Source/CMakeLists.txt`
- Modify: `Source/DefaultClient/CMakeLists.txt`

- [ ] **Step 1: Git rename the directory**

```bash
cd C:/GitRepo/InnocenceEngine
git mv Source/Client Source/DefaultClient
```

- [ ] **Step 2: Find all include paths that reference the old Client location**

```bash
grep -r "Client/ClientMetadata" Source/Engine/ Source/DefaultClient/
grep -r "#include.*\.\./Client/" Source/
grep -r "#include.*\.\./\.\./Client/" Source/
```

The only Engine.h reference is `../Client/ClientMetadata.h` (to be removed in Task 2). The DefaultClient source files use `../../Engine/...` which is unaffected by the rename. Confirm no other references exist.

- [ ] **Step 3: Update Source/CMakeLists.txt — rename subdirectory**

Change:
```cmake
add_subdirectory("Client")
```
To:
```cmake
add_subdirectory("DefaultClient")
```

- [ ] **Step 4: Update Source/DefaultClient/CMakeLists.txt**

Remove the `configure_file` line (ClientMetadata is being deleted in Task 2).
Remove `add_subdirectory("TestClient")` (TestClient moves to its own top-level dir in Task 4).
Keep `add_subdirectory("RenderingClient")` and `add_subdirectory("LogicClient")`.

- [ ] **Step 5: Update RenderingClient CMakeLists — hardcode library name**

In `Source/DefaultClient/RenderingClient/CMakeLists.txt`, replace `${INNO_RENDERING_CLIENT}` with `DefaultRenderingClient`:

```cmake
file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultRenderingClient ${HEADERS} ${SOURCES})
set_target_properties(DefaultRenderingClient PROPERTIES FOLDER DefaultClient)
target_link_libraries(DefaultRenderingClient Engine)
```

- [ ] **Step 6: Update LogicClient CMakeLists — hardcode library name**

In `Source/DefaultClient/LogicClient/CMakeLists.txt`, replace `${INNO_LOGIC_CLIENT}` with `DefaultLogicClient`:

```cmake
file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(DefaultLogicClient ${HEADERS} ${SOURCES})
set_target_properties(DefaultLogicClient PROPERTIES FOLDER DefaultClient)
target_link_libraries(DefaultLogicClient Engine)
```

- [ ] **Step 7: Regenerate CMake and verify it configures without error**

```bash
cd C:/GitRepo/InnocenceEngine/Build
cmake .. -G "Visual Studio 17 2022" -A x64
```

Expect: configuration succeeds, no "Client" references in output.

- [ ] **Step 8: Commit**

```
git add -A
git commit -m "refactor: rename Source/Client to Source/DefaultClient"
```

---

## Task 2: Decouple Engine from concrete client types

**Files:**
- Modify: `Source/Engine/Engine.h`
- Modify: `Source/Engine/Engine.cpp`
- Delete: `Source/DefaultClient/ClientMetadata.h`, `Source/DefaultClient/ClientMetadata.h.in`
- Modify: `Source/CMakeLists.txt`

- [ ] **Step 1: Delete ClientMetadata files**

```bash
git rm Source/DefaultClient/ClientMetadata.h
git rm Source/DefaultClient/ClientMetadata.h.in
```

- [ ] **Step 2: Strip Engine.h of all client-specific content**

Remove these lines from `Source/Engine/Engine.h`:
```cpp
#include "../Client/ClientMetadata.h"  // (now ../DefaultClient/...)

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define INNO_RENDERING_CLIENT_HEADER_PATH ../Client/RenderingClient/INNO_RENDERING_CLIENT.h
#define INNO_LOGIC_CLIENT_HEADER_PATH ../Client/LogicClient/INNO_LOGIC_CLIENT.h

#include STRINGIZE(INNO_RENDERING_CLIENT_HEADER_PATH)
#include STRINGIZE(INNO_LOGIC_CLIENT_HEADER_PATH)
```

Add includes for the interfaces (if not already transitively included):
```cpp
#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"
```

- [ ] **Step 3: Update InitConfig in Engine.h**

Remove `bool isTest = false;` and add `testCase`. Use a plain `char` array — NOT `FixedSizeString<N>`. `FixedSizeString::operator=(const char*)` has a pre-existing off-by-one bug at `FixedSizeString.h:44` that writes `m_content[l_sizeOfContent - 1] = '\0'`, stripping the last character of any assigned string. Assigning `"bareboot"` would store `"bareboo"`, breaking `strcmp` in `ParseTestCase`.

```cpp
struct InitConfig
{
    EngineMode engineMode = EngineMode::Host;
    RenderingServer renderingServer = RenderingServer::DX12;
    LogLevel logLevel = LogLevel::Success;
    bool isHeadless = false;
    bool isOffscreen = false;
    char testCase[64] = {};  // empty = not a test run
};
```

- [ ] **Step 4: Update Engine::Setup signature in Engine.h**

```cpp
bool Setup(
    void* appHook,
    void* extraHook,
    char* pScmdline,
    std::unique_ptr<IRenderingClient> renderingClient,
    std::unique_ptr<ILogicClient> logicClient);
```

- [ ] **Step 5: Update Engine.cpp — ParseInitConfig**

Replace the `-test` parsing block:
```cpp
// Old:
auto l_testArgPos = arg.find("-test");
if (l_testArgPos != std::string::npos)
{
    l_result.isTest = true;
    Log(Success, "Launch in test mode: TestClient will drive termination.");
}

// New:
auto l_testArgPos = arg.find("-test");
if (l_testArgPos != std::string::npos)
{
    std::string l_remainder = arg.substr(l_testArgPos + 5); // skip "-test"
    auto l_start = l_remainder.find_first_not_of(' ');
    if (l_start != std::string::npos)
    {
        auto l_end = l_remainder.find(' ', l_start);
        std::string l_caseName = l_remainder.substr(l_start,
            l_end == std::string::npos ? std::string::npos : l_end - l_start);
        strncpy(l_result.testCase, l_caseName.c_str(), sizeof(l_result.testCase) - 1);
        Log(Success, "Test case: ", l_result.testCase);
    }
}
```

`testCase` is `char[64]` — use `strncpy` directly. Do NOT use `FixedSizeString::operator=(const char*)` — see Step 3 note about the off-by-one bug.

- [ ] **Step 6: Update Engine.cpp — Setup, Initialize, and Terminate**

Replace the client construction block in `Engine::Setup`:
```cpp
// Old:
if (m_pImpl->m_initConfig.isTest)
    m_pImpl->m_RenderingClient = std::make_unique<TestClient>();
else
    m_pImpl->m_RenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();

m_pImpl->m_LogicClient = std::make_unique<INNO_LOGIC_CLIENT>();

// New:
m_pImpl->m_RenderingClient = std::move(renderingClient);
m_pImpl->m_LogicClient = std::move(logicClient);
```

**Critical — convert all `isHeadless` guards to null checks.** Every place in `Engine::Setup`, `Engine::Initialize`, and `Engine::Terminate` that gates on `!m_pImpl->m_initConfig.isHeadless` to guard client operations must become a null pointer check instead. The injected-client design means headless = nullptr clients, not a config flag. For example:

```cpp
// Old pattern (throughout Engine.cpp):
if (!m_pImpl->m_initConfig.isHeadless) {
    // ... setup/init/terminate RenderingClient and LogicClient
}

// New pattern:
if (m_pImpl->m_RenderingClient) {
    // ... rendering client operations
}
if (m_pImpl->m_LogicClient) {
    // ... logic client operations
}
```

Do this conversion for every such block. The existing per-service guards (e.g., the `TemplateAssetService`, `RenderingContextService`, `GUISystem` blocks already guarded by `!isHeadless`) must also remain because those services are independent of client injection — keep those guards as `!m_pImpl->m_initConfig.isHeadless`.

- [ ] **Step 7: Remove TestClient.h include from Engine.cpp**

Delete:
```cpp
#include "../Client/TestClient/TestClient.h"
```

- [ ] **Step 8: Update Source/CMakeLists.txt — remove INNO_* variables and Engine→client links**

Remove these lines entirely:
```cmake
set(INNO_LOGIC_CLIENT DefaultLogicClient)
set(INNO_RENDERING_CLIENT DefaultRenderingClient)

target_link_libraries(Engine ${INNO_RENDERING_CLIENT})
target_link_libraries(Engine ${INNO_LOGIC_CLIENT})
target_link_libraries(Engine TestClient)
```

After removal, Engine is no longer linked to any client. The clients link Engine (handled in their own CMakeLists). Executables will link both Engine and the client libs explicitly (done in Tasks 3 and 4).

- [ ] **Step 9: Attempt build — expect linker errors in WinMain (wrong Setup call) and missing INNO_* symbols**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && cmake .. -G "Visual Studio 17 2022" -A x64 && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

These errors are expected and will be fixed in Task 3. Confirm the errors are only call-site issues, not deeper Engine compilation failures.

- [ ] **Step 10: Commit Engine decoupling (WIP — build intentionally broken until Task 3)**

```
git add Source/Engine/Engine.h Source/Engine/Engine.cpp Source/CMakeLists.txt
git rm Source/DefaultClient/ClientMetadata.h Source/DefaultClient/ClientMetadata.h.in
git commit -m "refactor: decouple Engine from concrete client types via injected unique_ptr [WIP - BUILD BROKEN]"
```

The `[WIP - BUILD BROKEN]` tag signals that this commit is an intermediate state. Task 3 immediately restores a working build. Do not run CI on this commit in isolation.

---

## Task 3: IClientFactory + DefaultClientFactory + wire WinMain

**Files:**
- Create: `Source/Engine/Interface/IClientFactory.h`
- Create: `Source/DefaultClient/DefaultClientFactory.cpp`
- Modify: `Source/Engine/Platform/WinMain/WinMain.cpp`
- Modify: `Source/Engine/Platform/WinMain/CMakeLists.txt`

- [ ] **Step 1: Create IClientFactory.h**

`Source/Engine/Interface/IClientFactory.h`:
```cpp
#pragma once
#include "IRenderingClient.h"
#include "ILogicClient.h"
#include "../Common/STL17.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient();
    std::unique_ptr<ILogicClient>     CreateLogicClient();
}
```

Verify `STL17.h` provides `std::unique_ptr`. If the engine uses a different header for `memory`, match whatever `IRenderingClient.h` or `Engine.h` uses.

- [ ] **Step 2: Create DefaultClientFactory.cpp**

`Source/DefaultClient/DefaultClientFactory.cpp`:
```cpp
#include "../Engine/Interface/IClientFactory.h"
#include "RenderingClient/DefaultRenderingClient.h"
#include "LogicClient/DefaultLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return std::make_unique<DefaultRenderingClient>();
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<DefaultLogicClient>();
    }
}
```

- [ ] **Step 3: Add DefaultClientFactory as an explicit library in DefaultClient/CMakeLists.txt**

Add a dedicated `DefaultClientFactory` static library. Do not add `DefaultClientFactory.cpp` to the `DefaultRenderingClient` glob — that would create a dependency from `DefaultRenderingClient` on `DefaultLogicClient` and `IClientFactory.h`, violating separation of concerns.

```cmake
add_library(DefaultClientFactory DefaultClientFactory.cpp)
set_target_properties(DefaultClientFactory PROPERTIES FOLDER DefaultClient)
target_link_libraries(DefaultClientFactory DefaultRenderingClient DefaultLogicClient)
```

- [ ] **Step 4: Update WinMain.cpp**

Add the factory include and update `WinMain`:
```cpp
#include "../../Interface/IClientFactory.h"
```

Update the engine setup call:
```cpp
bool l_isHeadless = (pScmdline && strstr(pScmdline, "headless") != nullptr);

if (!m_pEngine->Setup(
    hInstance, nullptr, pScmdline,
    l_isHeadless ? nullptr : Inno::CreateRenderingClient(),
    l_isHeadless ? nullptr : Inno::CreateLogicClient()))
    return 2;
```

Note: this string scan for `"headless"` duplicates the detection logic already in `Engine::ParseInitConfig`. It is intentionally minimal — the alternatives (exposing a pre-parse helper, or letting Engine create null stubs internally) add more complexity than the duplication warrants. If `-headless` is ever renamed, both sites must be updated.

- [ ] **Step 5: Update WinMain/CMakeLists.txt — Main.exe links DefaultClientFactory**

```cmake
add_executable(Main WIN32 WinMain.cpp)
set_target_properties(Main PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(Main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(Main Engine DefaultClientFactory)
```

- [ ] **Step 6: Regenerate CMake and build**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && cmake .. -G "Visual Studio 17 2022" -A x64 && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Main" 2>&1
```

Expect: Main.exe builds successfully.

- [ ] **Step 7: Run Main.exe to verify normal operation is unchanged**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen" 2>&1
```

Terminate after a few seconds with Ctrl+C (it runs indefinitely in offscreen mode). Confirm no crash at startup.

- [ ] **Step 8: Commit**

```
git add Source/Engine/Interface/IClientFactory.h Source/DefaultClient/DefaultClientFactory.cpp Source/DefaultClient/CMakeLists.txt Source/Engine/Platform/WinMain/WinMain.cpp Source/Engine/Platform/WinMain/CMakeLists.txt
git commit -m "refactor: introduce IClientFactory, wire DefaultClientFactory into Main.exe"
```

---

## Task 4: TestClient skeleton + RenderTest.exe build target

**Files:**
- Create: `Source/TestClient/TestLogicClient.h`
- Create: `Source/TestClient/TestLogicClient.cpp`
- Create: `Source/TestClient/TestRenderingClient.h`
- Create: `Source/TestClient/TestRenderingClient.cpp`
- Create: `Source/TestClient/TestClientFactory.cpp`
- Create: `Source/TestClient/CMakeLists.txt`
- Modify: `Source/CMakeLists.txt`
- Modify: `Source/Engine/Platform/WinMain/CMakeLists.txt`

This task implements only the `bareboot` test case — no GPU work. The test just validates startup/shutdown of the engine with the test client pair.

- [ ] **Step 1: Create TestLogicClient.h**

`Source/TestClient/TestLogicClient.h`:
```cpp
#pragma once
#include "../Engine/Interface/ILogicClient.h"

namespace Inno
{
    class TestLogicClient : public ILogicClient
    {
    public:
        bool Setup(ISystemConfig* systemConfig = nullptr) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;
        const char* GetApplicationName() override;

    private:
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;
    };
}
```

- [ ] **Step 2: Create TestLogicClient.cpp**

`Source/TestClient/TestLogicClient.cpp`:
```cpp
#include "TestLogicClient.h"
#include "../Engine/Common/LogService.h"
#include "../Engine/Engine.h"

using namespace Inno;

bool TestLogicClient::Setup(ISystemConfig*)
{
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool TestLogicClient::Initialize() { return true; }
bool TestLogicClient::Update()     { return true; }

bool TestLogicClient::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestLogicClient::GetStatus() { return m_ObjectStatus; }

const char* TestLogicClient::GetApplicationName() { return "RenderTest"; }
```

- [ ] **Step 3: Create TestRenderingClient.h**

`Source/TestClient/TestRenderingClient.h`:
```cpp
#pragma once
#include "../Engine/Interface/IRenderingClient.h"
#include "../Engine/Common/STL14.h"

namespace Inno
{
    class TestRenderingClient : public IRenderingClient
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
        enum class TestCase { BareBoot, DrawInstanced, Unknown };
        static TestCase ParseTestCase(const char* name);

        // Lifecycle
        bool Setup_BareBoot();
        bool Setup_DrawInstanced();
        bool Initialize_DrawInstanced();
        bool PrepareCommands_DrawInstanced();
        bool ExecuteCommands_DrawInstanced();
        bool Terminate_DrawInstanced();

        // Frame control
        void CountFrameAndTerminateIfDone();
        static constexpr uint32_t k_TargetFrames = 10;
        uint32_t m_FramesAfterLoad = 0;

        TestCase m_TestCase = TestCase::Unknown;
        ObjectStatus m_ObjectStatus = ObjectStatus::Created;

        // draw_instanced GPU resources (null until initialized)
        struct DrawInstancedResources;
        DrawInstancedResources* m_DrawInstanced = nullptr;
    };
}
```

- [ ] **Step 4: Create TestRenderingClient.cpp — skeleton with bareboot only**

`Source/TestClient/TestRenderingClient.cpp`:
```cpp
#include "TestRenderingClient.h"
#include "../Engine/Engine.h"
#include "../Engine/Interface/IWindowSystem.h"
#include "../Engine/Services/SceneService.h"

using namespace Inno;

struct TestRenderingClient::DrawInstancedResources
{
    // filled in Task 5
};

TestRenderingClient::TestCase TestRenderingClient::ParseTestCase(const char* name)
{
    if (strcmp(name, "bareboot")       == 0) return TestCase::BareBoot;
    if (strcmp(name, "draw_instanced") == 0) return TestCase::DrawInstanced;
    return TestCase::Unknown;
}

bool TestRenderingClient::Setup(ISystemConfig*)
{
    m_TestCase = ParseTestCase(g_Engine->getInitConfig().testCase);

    switch (m_TestCase)
    {
    case TestCase::BareBoot:      return Setup_BareBoot();
    case TestCase::DrawInstanced: return Setup_DrawInstanced();
    default:
        Log(Error, "TestRenderingClient: unknown test case '",
            g_Engine->getInitConfig().testCase.c_str(), "'");
        return false;
    }
}

bool TestRenderingClient::Initialize()
{
    m_ObjectStatus = ObjectStatus::Activated;
    switch (m_TestCase)
    {
    case TestCase::DrawInstanced: return Initialize_DrawInstanced();
    default: return true;
    }
}

bool TestRenderingClient::Update()
{
    return true;
}

bool TestRenderingClient::PrepareCommands()
{
    if (m_TestCase == TestCase::DrawInstanced)
        return PrepareCommands_DrawInstanced();
    return true;
}

bool TestRenderingClient::ExecuteCommands(IRenderingConfig*)
{
    switch (m_TestCase)
    {
    case TestCase::BareBoot:
        CountFrameAndTerminateIfDone();
        return true;
    case TestCase::DrawInstanced:
        return ExecuteCommands_DrawInstanced();
    default:
        return true;
    }
}

bool TestRenderingClient::Terminate()
{
    if (m_TestCase == TestCase::DrawInstanced)
        Terminate_DrawInstanced();
    delete m_DrawInstanced;
    m_DrawInstanced = nullptr;
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestRenderingClient::GetStatus() { return m_ObjectStatus; }

void TestRenderingClient::CountFrameAndTerminateIfDone()
{
    if (g_Engine->Get<SceneService>()->IsLoading())
        return;

    ++m_FramesAfterLoad;
    if (m_FramesAfterLoad >= k_TargetFrames)
    {
        Log(Success, "TestRenderingClient: completed ", m_FramesAfterLoad, " frames. Terminating.");
        g_Engine->getWindowSystem()->Terminate();
    }
}

// --- BareBoot ---
bool TestRenderingClient::Setup_BareBoot() { return true; }

// --- DrawInstanced stubs (implemented in Task 5) ---
bool TestRenderingClient::Setup_DrawInstanced()         { return true; }
bool TestRenderingClient::Initialize_DrawInstanced()    { return true; }
bool TestRenderingClient::PrepareCommands_DrawInstanced(){ return true; }
bool TestRenderingClient::ExecuteCommands_DrawInstanced()
{
    CountFrameAndTerminateIfDone();
    return true;
}
bool TestRenderingClient::Terminate_DrawInstanced()     { return true; }
```

- [ ] **Step 5: Create TestClientFactory.cpp**

`Source/TestClient/TestClientFactory.cpp`:
```cpp
#include "../Engine/Interface/IClientFactory.h"
#include "TestRenderingClient.h"
#include "TestLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return std::make_unique<TestRenderingClient>();
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<TestLogicClient>();
    }
}
```

- [ ] **Step 6: Create Source/TestClient/CMakeLists.txt**

```cmake
file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(TestRenderingClient TestRenderingClient.h TestRenderingClient.cpp)
set_target_properties(TestRenderingClient PROPERTIES FOLDER TestClient)
target_link_libraries(TestRenderingClient Engine)

add_library(TestLogicClient TestLogicClient.h TestLogicClient.cpp)
set_target_properties(TestLogicClient PROPERTIES FOLDER TestClient)
target_link_libraries(TestLogicClient Engine)

add_library(TestClientFactory TestClientFactory.cpp)
set_target_properties(TestClientFactory PROPERTIES FOLDER TestClient)
target_link_libraries(TestClientFactory TestRenderingClient TestLogicClient)
```

- [ ] **Step 7: Add TestClient to Source/CMakeLists.txt**

Add after the `DefaultClient` line:
```cmake
add_subdirectory("DefaultClient")
add_subdirectory("TestClient")
```

- [ ] **Step 8: Add RenderTest.exe target to WinMain/CMakeLists.txt**

Append to `Source/Engine/Platform/WinMain/CMakeLists.txt`:
```cmake
add_executable(RenderTest WIN32 WinMain.cpp)
set_target_properties(RenderTest PROPERTIES FOLDER Engine/Main)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(RenderTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(RenderTest Engine TestClientFactory)
```

- [ ] **Step 9: Regenerate CMake and build all targets**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && cmake .. -G "Visual Studio 17 2022" -A x64 && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expect: `Main.exe`, `RenderTest.exe`, `Test.exe` all build without error.

- [ ] **Step 10: Run bareboot test — verify clean exit**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test bareboot"
echo Exit code: %ERRORLEVEL%
```

Expected: exit code 0.

- [ ] **Step 11: Commit**

```
git add Source/TestClient/ Source/CMakeLists.txt Source/Engine/Platform/WinMain/CMakeLists.txt
git commit -m "feat: add TestRenderingClient/TestLogicClient skeleton and RenderTest.exe build target"
```

---

## Task 5: Implement draw_instanced test case

**Files:**
- Create: `Source/TestClient/Shaders/draw_instanced.vs.hlsl`
- Create: `Source/TestClient/Shaders/draw_instanced.ps.hlsl`
- Modify: `Source/TestClient/TestRenderingClient.h`
- Modify: `Source/TestClient/TestRenderingClient.cpp`

Before writing any GPU setup code, read these files to understand the descriptor/config fields:
- `Source/Engine/Component/RenderPassComponent.h`
- `Source/Engine/Component/ShaderProgramComponent.h`
- `Source/Engine/Component/CommandListComponent.h`

Match field names and patterns exactly from those structs. Do not guess.

- [ ] **Step 1: Read component headers to understand descriptor fields**

```
Read: Source/Engine/Component/RenderPassComponent.h
Read: Source/Engine/Component/ShaderProgramComponent.h
Read: Source/Engine/Component/CommandListComponent.h
```

Note down: how render targets are configured, how shaders are pointed to compiled DXIL files, how a command list is associated with a render pass and engine type.

- [ ] **Step 2: Write draw_instanced vertex shader**

`Source/TestClient/Shaders/draw_instanced.vs.hlsl`:
```hlsl
float4 main(uint vertexID : SV_VertexID) : SV_Position
{
    float2 positions[3] =
    {
        float2(-0.5,  -0.5),
        float2( 0.0,   0.5),
        float2( 0.5,  -0.5)
    };
    return float4(positions[vertexID], 0.0, 1.0);
}
```

- [ ] **Step 3: Write draw_instanced pixel shader**

`Source/TestClient/Shaders/draw_instanced.ps.hlsl`:
```hlsl
float4 main() : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
```

- [ ] **Step 4: Compile shaders to DXIL**

Check the HLSL compilation script and output directory convention used by other passes (e.g., look at `Scripts/HLSL2DXIL.ps1` and where existing DXIL files land). Add entries for the new shaders and compile:

```cmd
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1" 2>&1
```

Confirm DXIL output files exist in the expected location.

- [ ] **Step 5: Populate DrawInstancedResources and implement Setup_DrawInstanced**

Based on what you read in Step 1, fill in the `DrawInstancedResources` struct and implement the setup. The general pattern (verify field names against component headers):

```cpp
struct TestRenderingClient::DrawInstancedResources
{
    RenderPassComponent*    RenderPass    = nullptr;
    ShaderProgramComponent* ShaderProgram = nullptr;
    CommandListComponent*   CommandList   = nullptr;
};

bool TestRenderingClient::Setup_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();

    m_DrawInstanced = new DrawInstancedResources();

    // Shader program — point to compiled DXIL, match paths from existing passes
    m_DrawInstanced->ShaderProgram = l_rs->AddShaderProgramComponent("TestDrawInstanced");
    // Configure shader paths using the field names from ShaderProgramComponent.h

    // Render pass — single color RT, no depth, offscreen
    m_DrawInstanced->RenderPass = l_rs->AddRenderPassComponent("TestDrawInstanced");
    // Configure RenderPassDesc using field names from RenderPassComponent.h
    // Key settings: pipeline type = Graphics, one color output, no swap chain

    m_DrawInstanced->CommandList = l_rs->AddCommandListComponent("TestDrawInstanced");

    return true;
}
```

Use an existing simple pass (e.g., `SkyPass.cpp`) as a reference for the exact descriptor field names and initialization sequence.

- [ ] **Step 6: Implement Initialize_DrawInstanced**

```cpp
bool TestRenderingClient::Initialize_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();
    l_rs->Initialize(m_DrawInstanced->ShaderProgram);
    l_rs->Initialize(m_DrawInstanced->RenderPass);
    l_rs->Initialize(m_DrawInstanced->CommandList);
    return true;
}
```

- [ ] **Step 7: Implement PrepareCommands_DrawInstanced**

The third parameter to `DrawInstanced` is `instanceCount`, not vertex count. The vertex count for this procedural triangle (3 vertices via `SV_VertexID`) must be configured in the `RenderPassComponent` descriptor — confirm the exact field name by reading `RenderPassComponent.h` in Step 1. Look for a field like `m_drawCallCount`, `m_vertexCount`, or similar on `RenderPassDesc`. Set it to `3` during `Setup_DrawInstanced`. If no such field exists on the render pass and the count is instead a draw call argument, check whether `DrawInstanced` has an overload or variant that accepts vertex count. Match whatever existing simple passes do.

```cpp
bool TestRenderingClient::PrepareCommands_DrawInstanced()
{
    auto l_rs  = g_Engine->getRenderingServer();
    auto l_rp  = m_DrawInstanced->RenderPass;
    auto l_cl  = m_DrawInstanced->CommandList;

    l_rs->CommandListBegin(l_rp, l_cl, l_rs->GetCurrentFrame());
    l_rs->BindRenderPassComponent(l_rp, l_cl);
    l_rs->ClearRenderTargets(l_rp, l_cl);
    l_rs->DrawInstanced(l_rp, l_cl, 1);  // instanceCount = 1; vertex count configured in RenderPassDesc
    l_rs->CommandListEnd(l_rp, l_cl);

    return true;
}
```

- [ ] **Step 8: Implement ExecuteCommands_DrawInstanced**

```cpp
bool TestRenderingClient::ExecuteCommands_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();
    auto l_rp = m_DrawInstanced->RenderPass;
    auto l_cl = m_DrawInstanced->CommandList;

    l_rs->Execute(l_cl, GPUEngineType::Graphics);
    l_rs->SignalOnGPU(l_rp, GPUEngineType::Graphics);

    CountFrameAndTerminateIfDone();
    return true;
}
```

- [ ] **Step 9: Implement Terminate_DrawInstanced**

```cpp
bool TestRenderingClient::Terminate_DrawInstanced()
{
    auto l_rs = g_Engine->getRenderingServer();
    auto l_graphicsVal = l_rs->GetSemaphoreValue(GPUEngineType::Graphics);
    l_rs->WaitOnCPU(l_graphicsVal, GPUEngineType::Graphics);

    l_rs->Delete(m_DrawInstanced->CommandList);
    l_rs->Delete(m_DrawInstanced->RenderPass);
    l_rs->Delete(m_DrawInstanced->ShaderProgram);
    return true;
}
```

- [ ] **Step 10: Build**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:RenderTest" 2>&1
```

Fix any compile errors before proceeding.

- [ ] **Step 11: Run draw_instanced test — verify pass**

```cmd
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced"
echo Exit code: %ERRORLEVEL%
```

Expected: exit code 0, no D3D12 validation errors in log.

- [ ] **Step 12: Commit**

```
git add Source/TestClient/ Source/TestClient/Shaders/
git commit -m "feat: implement draw_instanced rendering API test case"
```

---

## Task 6: Update CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update GPU validation command**

Replace:
```
# GPU validation — autonomous test, exits 0=pass, 1=GPU error, 2=crash (preferred over Test.exe when rendering is touched)
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test" 2>&1
```

With:
```
# GPU validation — rendering API unit tests, exits 0=pass, 1=GPU error, 2=crash
# Test cases: bareboot, draw_instanced
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced" 2>&1
```

- [ ] **Step 2: Commit**

```
git add CLAUDE.md
git commit -m "docs: update GPU validation command to use RenderTest.exe"
```
