# ECS Phase 3 — TransformService and HierarchyGraph Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `TransformService` (an `ISystem`) that computes `WorldTransformComponent` from each entity's local `TransformComponent`, with a `HierarchyGraph` for parent-child propagation.

**Architecture:** Split `TransformComponent` into local-only data and a new `WorldTransformComponent` (world matrix + rotation matrix). `TransformService` owns a `HierarchyGraph` (sparse array of `HierarchyNode` linked-lists) and in `Update()` iterates all entities with `TransformComponent` in topological order (BFS from roots), propagating parent world matrices to children. `DrawCallService` and `DX12RenderingServer` are updated to read from `WorldTransformComponent` instead of `TransformComponent`.

**Tech Stack:** C++17, engine math helpers (`Math::toTranslationMatrix`, `Math::toRotationMatrix`, `Math::toScaleMatrix` from `MathHelper.h`), `EntityRegistry` (sparse-set component storage), `SceneService` (scene loading callbacks), MSVC/MSBuild.

---

## Context for the implementer

### Critical codebase facts

- **Build directory**: `C:\GitRepo\InnocenceEngine\Build\` — not committed to git. After creating `TransformService.cpp`, you must add it to `Build\Source\Engine\Services\Services.vcxproj` manually (search for `EntityRegistry.cpp` in that file to find the right `<ClCompile>` block, then add a sibling entry for `TransformService.cpp`).
- **No Quat type** — quaternions are stored as `Vec4` with XYZW layout. `Math::toRotationMatrix(Vec4)` converts to `Mat4`.
- **TRS matrix order**: `translationMat * rotationMat * scaleMat` — verified from `MathHelper.h::calcTransformationMatrix`.
- **`EntityRegistry::Emplace<T>` asserts** if the entity already has that component type (via `TComponentStorage::Add` — see `ComponentStorage.h:24`). Always guard with `Has<T>()` before calling `Emplace<T>()`.
- **`EntityRegistry::Emplace<T>`** takes `T Data = {}` as second arg. Does NOT check for duplicates — that is the caller's responsibility.
- **Engine update loop order** (from `Engine.cpp` `SetUploadHeapPreparationCallback`):
  1. `SceneService::Update()` — loads/unloads scenes
  2. `LogicClient::Update()` — gameplay writes local transforms
  3. `CameraSystem::Update()`, `LightSystem::Update()`
  4. `EntityRegistry::Update()`
  5. **← Insert `TransformService::Update()` here** (after EntityRegistry, before PerFrameData)
  6. `PerFrameDataService`, `LightDataService`, `DrawCallService` — must see computed world matrices
- **SceneService callback API**: `AddSceneLoadingStartedCallback(std::function<void()>* functor, int32_t priority)` — functor must be a pointer to a stable function object (store as a member).
- **`m_WorldMatrix` is currently zero-initialized** and never written — `DrawCallService` reads it and produces zero transforms. Phase 3 fixes this.
- **Test binary**: `cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe`
- **GPU validation**: `powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"`
- **Build**: `cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1`

### Files to read before starting

- `Source/Engine/Component/TransformComponent.h` — current struct (has `m_WorldMatrix` and `m_Dirty` to remove)
- `Source/Engine/Services/EntityRegistry.h` — component API
- `Source/Engine/Common/ComponentStorage.h` — `TComponentStorage::Add` behaviour
- `Source/Engine/Services/DrawCallService.cpp` lines 171–177 — `m_WorldMatrix` read to migrate
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp` lines 702–724 — `m_WorldMatrix` read to migrate
- `Source/Engine/Common/MathHelper.h` lines 1109–1115 — matrix composition pattern
- `Source/Engine/Interface/ISystem.h` — ISystem interface
- `Source/Engine/Services/SceneService.h` — callback registration API
- `Source/Engine/Engine.cpp` lines 476–530 — where to insert TransformService
- `Source/Test/UnitTests/EntityRegistryTests.cpp` lines 146–192 — test to fix (`m_Dirty` removed)
- `Documents/code-standards.md` — mandatory before every code change

### Code standards highlights (read the doc — this is a summary only)

- No STL includes directly — use `STL14.h` / `STL17.h` engine wrappers
- No `new[]`/`malloc`/`free` — use engine memory patterns
- No `std::cout` — use `Log()`
- No inline engine-API calls (`g_Engine`, `Log`) in headers
- Member variables: `m_PascalCase`, local variables: `l_camelCase`
- No explanatory comments — only comment when code intent is not obvious

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `Source/Engine/Component/WorldTransformComponent.h` | **Create** | POD struct: world matrix + world rotation matrix |
| `Source/Engine/Component/TransformComponent.h` | **Modify** | Remove `m_WorldMatrix` and `m_Dirty` |
| `Source/Engine/Services/TransformService.h` | **Create** | TransformService ISystem declaration + HierarchyNode struct |
| `Source/Engine/Services/TransformService.cpp` | **Create** | Full implementation: HierarchyGraph, BFS traversal, TRS propagation |
| `Source/Engine/Engine.cpp` | **Modify** | Include + register TransformService |
| `Source/Engine/Services/DrawCallService.cpp` | **Modify** | Read `WorldTransformComponent` instead of `TransformComponent::m_WorldMatrix` |
| `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp` | **Modify** | Read `WorldTransformComponent` instead of `TransformComponent::m_WorldMatrix` |
| `Source/Test/UnitTests/EntityRegistryTests.cpp` | **Modify** | Remove `m_Dirty` reference; add `WorldTransformComponent` test |
| `Build/Source/Engine/Services/Services.vcxproj` | **Modify** | Add `TransformService.cpp` compile entry (not committed) |

---

## Task 1: WorldTransformComponent struct + strip TransformComponent

**Files:**
- Create: `Source/Engine/Component/WorldTransformComponent.h`
- Modify: `Source/Engine/Component/TransformComponent.h`

This is a header-only change. No build yet — the test in Task 2 will gate it.

- [ ] **Step 1: Create `WorldTransformComponent.h`**

```cpp
// Source/Engine/Component/WorldTransformComponent.h
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct WorldTransformComponent
    {
        Mat4 m_WorldMatrix         = {};
        Mat4 m_WorldRotationMatrix = {};
    };
}
```

- [ ] **Step 2: Strip `TransformComponent.h`**

Remove `m_WorldMatrix` and `m_Dirty`. The final file:

```cpp
// Source/Engine/Component/TransformComponent.h
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct TransformComponent
    {
        Vec3 m_LocalPos   = {};
        Vec4 m_LocalRot   = Vec4(0.f, 0.f, 0.f, 1.f);  // quaternion XYZW
        Vec3 m_LocalScale = Vec3(1.f, 1.f, 1.f);
    };
}
```

- [ ] **Step 3: Fix `EntityRegistryTests.cpp` — remove `m_Dirty` reference**

In `Source/Test/UnitTests/EntityRegistryTests.cpp`, line 163 currently reads:
```cpp
l_TestPassed = l_TPtr != nullptr && l_TPtr->m_LocalPos.x == 1.f && l_TPtr->m_Dirty == true;
```

Change it to (removing `m_Dirty` check which no longer exists):
```cpp
l_TestPassed = l_TPtr != nullptr && l_TPtr->m_LocalPos.x == 1.f;
```

- [ ] **Step 4: Fix `PhysXWrapper.cpp` — remove m_Dirty write**

In `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp`, find the physics write-back loop (search for `m_Dirty`). Remove the line:
```cpp
l_Transform->m_Dirty = true;
```
(It was a flag with no consumer — TransformService will recompute world matrices every frame regardless.)

- [ ] **Step 5: Build to verify header changes compile**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: build errors about `m_WorldMatrix` and `m_Dirty` in callers (DrawCallService.cpp, DX12RenderingServer_EngineComponent_Protected.cpp). This is expected — those callers will be fixed in Task 4.

If unexpected errors appear (anything other than m_WorldMatrix/m_Dirty references), fix them before continuing.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Component/WorldTransformComponent.h \
        Source/Engine/Component/TransformComponent.h \
        Source/Test/UnitTests/EntityRegistryTests.cpp \
        Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp
git commit -m "feat: add WorldTransformComponent — strip m_WorldMatrix and m_Dirty from TransformComponent"
```

---

## Task 2: Create TransformService

**Files:**
- Create: `Source/Engine/Services/TransformService.h`
- Create: `Source/Engine/Services/TransformService.cpp`
- Modify: `Build/Source/Engine/Services/Services.vcxproj`

- [ ] **Step 1: Write `TransformService.h`**

```cpp
// Source/Engine/Services/TransformService.h
#pragma once
#include "../Common/EntityID.h"
#include "../Common/STL14.h"
#include "../Interface/ISystem.h"

namespace Inno
{
    struct HierarchyNode
    {
        EntityID m_Parent      = INVALID_ENTITY;
        EntityID m_FirstChild  = INVALID_ENTITY;
        EntityID m_NextSibling = INVALID_ENTITY;
        uint32_t m_Depth       = 0;
    };

    class TransformService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(TransformService);

        bool Setup(ISystemConfig*) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

        void     SetParent(EntityID Child, EntityID Parent);
        void     ClearParent(EntityID Child);
        EntityID GetParent(EntityID Child) const;
        EntityID GetFirstChild(EntityID Parent) const;
        EntityID GetNextSibling(EntityID Entity) const;

    private:
        // Sparse array indexed by EntityID — O(1) parent/child lookup.
        // Allocated once at Setup; 65536 * 16 bytes = 1 MB.
        std::vector<HierarchyNode> m_Nodes;
        std::vector<EntityID>      m_TraversalOrder;
        bool                       m_HierarchyDirty = true;
        ObjectStatus               m_ObjectStatus   = ObjectStatus::Invalid;

        std::function<void()> m_SceneLoadingCallback;

        void RebuildTraversalOrder(const std::vector<EntityID>& AllTransformOwners);
    };
}
```

- [ ] **Step 2: Write `TransformService.cpp`**

```cpp
// Source/Engine/Services/TransformService.cpp
#include "TransformService.h"
#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Component/TransformComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "../Engine.h"

using namespace Inno;

bool TransformService::Setup(ISystemConfig*)
{
    m_Nodes.resize(MAX_ENTITIES);
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool TransformService::Initialize()
{
    m_SceneLoadingCallback = [this]()
    {
        std::fill(m_Nodes.begin(), m_Nodes.end(), HierarchyNode{});
        m_TraversalOrder.clear();
        m_HierarchyDirty = true;
    };
    g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&m_SceneLoadingCallback, 0);

    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool TransformService::Update()
{
    if (m_ObjectStatus != ObjectStatus::Activated)
        return true;

    auto* l_Registry = g_Engine->Get<EntityRegistry>();
    auto& l_TransformStorage = l_Registry->Storage<TransformComponent>();
    const auto& l_Owners = l_TransformStorage.AllOwners();

    // Ensure every entity with TransformComponent has a WorldTransformComponent.
    for (EntityID l_Entity : l_Owners)
    {
        if (!l_Registry->Has<WorldTransformComponent>(l_Entity))
            l_Registry->Emplace<WorldTransformComponent>(l_Entity);
    }

    // Rebuild topological order when entity count changes or hierarchy is modified.
    if (m_HierarchyDirty || l_Owners.size() != m_TraversalOrder.size())
        RebuildTraversalOrder(l_Owners);

    // Propagate transforms in topological order (parents before children).
    for (EntityID l_Entity : m_TraversalOrder)
    {
        auto* l_Local = l_Registry->Get<TransformComponent>(l_Entity);
        auto* l_World = l_Registry->Get<WorldTransformComponent>(l_Entity);
        if (!l_Local || !l_World)
            continue;

        Mat4 l_T = Math::toTranslationMatrix(Vec4(l_Local->m_LocalPos, 1.0f));
        Mat4 l_R = Math::toRotationMatrix(l_Local->m_LocalRot);
        Mat4 l_S = Math::toScaleMatrix(Vec4(l_Local->m_LocalScale, 1.0f));
        Mat4 l_LocalTRS = l_T * l_R * l_S;

        EntityID l_Parent = m_Nodes[l_Entity].m_Parent;
        if (l_Parent == INVALID_ENTITY)
        {
            l_World->m_WorldMatrix         = l_LocalTRS;
            l_World->m_WorldRotationMatrix = l_R;
        }
        else
        {
            auto* l_ParentWorld = l_Registry->Get<WorldTransformComponent>(l_Parent);
            if (l_ParentWorld)
            {
                l_World->m_WorldMatrix         = l_ParentWorld->m_WorldMatrix * l_LocalTRS;
                l_World->m_WorldRotationMatrix = l_ParentWorld->m_WorldRotationMatrix * l_R;
            }
            else
            {
                l_World->m_WorldMatrix         = l_LocalTRS;
                l_World->m_WorldRotationMatrix = l_R;
            }
        }
    }

    return true;
}

bool TransformService::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TransformService::GetStatus()
{
    return m_ObjectStatus;
}

void TransformService::SetParent(EntityID l_Child, EntityID l_Parent)
{
    if (l_Child == INVALID_ENTITY || l_Child >= MAX_ENTITIES)
        return;

    // Detach from current parent.
    EntityID l_OldParent = m_Nodes[l_Child].m_Parent;
    if (l_OldParent != INVALID_ENTITY)
    {
        EntityID& l_Head = m_Nodes[l_OldParent].m_FirstChild;
        if (l_Head == l_Child)
        {
            l_Head = m_Nodes[l_Child].m_NextSibling;
        }
        else
        {
            EntityID l_Prev = l_Head;
            while (l_Prev != INVALID_ENTITY && m_Nodes[l_Prev].m_NextSibling != l_Child)
                l_Prev = m_Nodes[l_Prev].m_NextSibling;
            if (l_Prev != INVALID_ENTITY)
                m_Nodes[l_Prev].m_NextSibling = m_Nodes[l_Child].m_NextSibling;
        }
    }

    // Attach to new parent (prepend to sibling list).
    m_Nodes[l_Child].m_Parent      = l_Parent;
    m_Nodes[l_Child].m_NextSibling = INVALID_ENTITY;

    if (l_Parent != INVALID_ENTITY && l_Parent < MAX_ENTITIES)
    {
        m_Nodes[l_Child].m_NextSibling   = m_Nodes[l_Parent].m_FirstChild;
        m_Nodes[l_Parent].m_FirstChild   = l_Child;
        m_Nodes[l_Child].m_Depth         = m_Nodes[l_Parent].m_Depth + 1;
    }

    m_HierarchyDirty = true;
}

void TransformService::ClearParent(EntityID l_Child)
{
    SetParent(l_Child, INVALID_ENTITY);
}

EntityID TransformService::GetParent(EntityID l_Child) const
{
    if (l_Child == INVALID_ENTITY || l_Child >= MAX_ENTITIES)
        return INVALID_ENTITY;
    return m_Nodes[l_Child].m_Parent;
}

EntityID TransformService::GetFirstChild(EntityID l_Parent) const
{
    if (l_Parent == INVALID_ENTITY || l_Parent >= MAX_ENTITIES)
        return INVALID_ENTITY;
    return m_Nodes[l_Parent].m_FirstChild;
}

EntityID TransformService::GetNextSibling(EntityID l_Entity) const
{
    if (l_Entity == INVALID_ENTITY || l_Entity >= MAX_ENTITIES)
        return INVALID_ENTITY;
    return m_Nodes[l_Entity].m_NextSibling;
}

void TransformService::RebuildTraversalOrder(const std::vector<EntityID>& l_AllOwners)
{
    m_TraversalOrder.clear();
    m_TraversalOrder.reserve(l_AllOwners.size());

    // BFS from roots — roots are entities with no parent in the HierarchyGraph.
    std::vector<EntityID> l_Queue;
    l_Queue.reserve(l_AllOwners.size());

    for (EntityID l_Entity : l_AllOwners)
    {
        if (m_Nodes[l_Entity].m_Parent == INVALID_ENTITY)
            l_Queue.push_back(l_Entity);
    }

    for (size_t l_Idx = 0; l_Idx < l_Queue.size(); ++l_Idx)
    {
        EntityID l_Current = l_Queue[l_Idx];
        m_TraversalOrder.push_back(l_Current);

        EntityID l_Child = m_Nodes[l_Current].m_FirstChild;
        while (l_Child != INVALID_ENTITY)
        {
            l_Queue.push_back(l_Child);
            l_Child = m_Nodes[l_Child].m_NextSibling;
        }
    }

    m_HierarchyDirty = false;
}
```

- [ ] **Step 3: Add TransformService.cpp to Services.vcxproj**

Open `Build\Source\Engine\Services\Services.vcxproj`. Find the `<ClCompile>` entry for `EntityRegistry.cpp`:
```xml
<ClCompile Include="..\..\..\Source\Engine\Services\EntityRegistry.cpp" />
```

Add immediately after it:
```xml
<ClCompile Include="..\..\..\Source\Engine\Services\TransformService.cpp" />
```

- [ ] **Step 4: Build to verify TransformService compiles (without registration yet)**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: same `m_WorldMatrix`/`m_Dirty` errors in DrawCallService and DX12RenderingServer as before (those are fixed in Task 4). TransformService itself should compile cleanly.

If there are new errors in TransformService.cpp or TransformService.h, fix them before continuing.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Services/TransformService.h \
        Source/Engine/Services/TransformService.cpp
git commit -m "feat: implement TransformService — HierarchyGraph BFS traversal and TRS propagation"
```

---

## Task 3: Register TransformService with Engine

**Files:**
- Modify: `Source/Engine/Engine.cpp`

- [ ] **Step 1: Add include**

In `Source/Engine/Engine.cpp`, after the `#include "Services/EntityRegistry.h"` line, add:
```cpp
#include "Services/TransformService.h"
```

- [ ] **Step 2: Add SystemSetup call**

In `Engine::Setup()`, after `SystemSetup(EntityRegistry);` (around line 476), add:
```cpp
SystemSetup(TransformService);
```

- [ ] **Step 3: Add SystemInit call**

In `Engine::Initialize()`, after `SystemInit(EntityRegistry);` (search for the Init block), add:
```cpp
SystemInit(TransformService);
```

- [ ] **Step 4: Add Update call**

In the `SetUploadHeapPreparationCallback` lambda, after `SystemUpdate(EntityRegistry);` and before the rendering-services block, add:
```cpp
Get<TransformService>()->Update();
```

The ordering context (from Engine.cpp ~line 506–520):
```cpp
SystemUpdate(EntityRegistry);

Get<TransformService>()->Update();  // ← ADD HERE

// Only update rendering-related services if not headless
if (!m_pImpl->m_initConfig.isHeadless) {
    Get<PerFrameDataService>()->Update();
    ...
```

- [ ] **Step 5: Add SystemTerm call**

In `Engine::Terminate()`, find where other services are terminated and add:
```cpp
SystemTerm(TransformService);
```

Place it near `SystemTerm(EntityRegistry)` — TransformService should terminate before EntityRegistry.

- [ ] **Step 6: Build to verify registration compiles**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: same `m_WorldMatrix` errors in DrawCallService and DX12RenderingServer (still unfixed). No new errors from the registration changes.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Engine.cpp
git commit -m "feat: register TransformService with engine — Setup/Init/Update/Terminate"
```

---

## Task 4: Update DrawCallService and DX12RenderingServer to read WorldTransformComponent

**Files:**
- Modify: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp`

After this task, the engine should build with zero errors.

- [ ] **Step 1: Update DrawCallService.cpp**

In `Source/Engine/Services/DrawCallService.cpp`, find the `#include` block at the top and add:
```cpp
#include "../Component/WorldTransformComponent.h"
```

Find lines 171–177:
```cpp
auto* l_transform = l_registry->Get<TransformComponent>(l_Entity);
TransformConstantBuffer l_transformCB = {};
if (l_transform)
{
    l_transformCB.m = l_transform->m_WorldMatrix;
    l_transformCB.normalMat = l_transform->m_WorldMatrix.inverse().transpose();
}
```

Replace with:
```cpp
auto* l_world = l_registry->Get<WorldTransformComponent>(l_Entity);
TransformConstantBuffer l_transformCB = {};
if (l_world)
{
    l_transformCB.m = l_world->m_WorldMatrix;
    l_transformCB.normalMat = l_world->m_WorldRotationMatrix;
}
```

Note: `m_WorldRotationMatrix` is the pure rotation matrix (inverse-transpose of the rotation part equals itself for orthonormal matrices), so it replaces `m_WorldMatrix.inverse().transpose()` correctly.

Also: the old `l_transform` variable was used only for `m_WorldMatrix` reads. Verify it is not used for anything else in the surrounding code block. If the `TransformComponent` include is no longer needed in DrawCallService.cpp, remove it to keep includes clean.

- [ ] **Step 2: Update DX12RenderingServer_EngineComponent_Protected.cpp**

In `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp`, find the `#include` block and add:
```cpp
#include "../../Component/WorldTransformComponent.h"
```

Find lines 702–705 (function `DX12RenderingServer::InitializeImpl`):
```cpp
auto* l_transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(Entity);
Mat4 transformMatrix = l_transform ? l_transform->m_WorldMatrix : Mat4{};
```

Replace with:
```cpp
auto* l_world = g_Engine->Get<EntityRegistry>()->Get<WorldTransformComponent>(Entity);
Mat4 transformMatrix = l_world ? l_world->m_WorldMatrix : Mat4{};
```

Also check if the `TransformComponent` include in this file is still needed elsewhere. If `TransformComponent.h` is no longer referenced in this file, remove the include.

- [ ] **Step 3: Build — expect clean compile**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: **zero errors**. All `m_WorldMatrix` and `m_Dirty` references have been removed or migrated.

Fix any remaining errors before continuing.

- [ ] **Step 4: Run unit tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: all tests pass. Verify the `EntityRegistry New Component Types` test passes (it now checks `m_LocalPos.x == 1.f` without `m_Dirty`).

A shutdown crash (exit code -1073741819 / 0xC0000005) is a known pre-existing issue — do not investigate it.

- [ ] **Step 5: Run GPU validation test**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code **0**.

If exit code is 1 (GPU error) or 2 (crash): this is a regression introduced by Phase 3. Do not proceed — debug first. The most likely cause is `WorldTransformComponent` not being populated before `DrawCallService` reads it (check engine update order in Engine.cpp).

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/DrawCallService.cpp \
        Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp
git commit -m "refactor: migrate DrawCallService and DX12RenderingServer to WorldTransformComponent"
```

---

## Task 5: Unit test for TransformService

**Files:**
- Modify: `Source/Test/UnitTests/EntityRegistryTests.cpp`

Add a test that verifies TransformService computes `WorldTransformComponent` correctly.

- [ ] **Step 1: Add WorldTransformComponent test**

In `Source/Test/UnitTests/EntityRegistryTests.cpp`, add `#include "../../Engine/Component/WorldTransformComponent.h"` at the top (alongside the other component includes).

Add a new test function before `RunEntityRegistryTests()`:

```cpp
static void TestTransformPropagation()
{
    TestRunner::StartTest("TransformService propagates WorldTransformComponent");

    auto* l_Registry = g_Engine->Get<EntityRegistry>();
    auto* l_TransformService = g_Engine->Get<TransformService>();
    bool l_TestPassed = l_Registry != nullptr && l_TransformService != nullptr;

    if (l_TestPassed)
    {
        // Spawn a root entity at (1, 2, 3)
        EntityID l_Root = l_Registry->Spawn(ObjectLifespan::Frame, "transform_test_root");
        auto& l_RootTransform = l_Registry->Emplace<TransformComponent>(l_Root);
        l_RootTransform.m_LocalPos = Vec3(1.f, 2.f, 3.f);
        // identity rotation, unit scale

        // Spawn a child entity at local (0, 1, 0)
        EntityID l_Child = l_Registry->Spawn(ObjectLifespan::Frame, "transform_test_child");
        auto& l_ChildTransform = l_Registry->Emplace<TransformComponent>(l_Child);
        l_ChildTransform.m_LocalPos = Vec3(0.f, 1.f, 0.f);

        l_TransformService->SetParent(l_Child, l_Root);

        // Run TransformService::Update() to propagate
        l_TransformService->Update();

        // Root world matrix translation column should be (1, 2, 3)
        auto* l_RootWorld = l_Registry->Get<WorldTransformComponent>(l_Root);
        l_TestPassed = l_RootWorld != nullptr;
        if (l_TestPassed)
        {
            // toTranslationMatrix stores translation in column 3: m03=x, m13=y, m23=z.
            // (Row-major storage, column-major math convention — see MathHelper.h lines 462-476.)
            l_TestPassed = l_RootWorld->m_WorldMatrix.m03 == 1.f
                        && l_RootWorld->m_WorldMatrix.m13 == 2.f
                        && l_RootWorld->m_WorldMatrix.m23 == 3.f;
        }

        // Child world position should be root position + child local position = (1, 3, 3)
        if (l_TestPassed)
        {
            auto* l_ChildWorld = l_Registry->Get<WorldTransformComponent>(l_Child);
            l_TestPassed = l_ChildWorld != nullptr;
            if (l_TestPassed)
            {
                l_TestPassed = l_ChildWorld->m_WorldMatrix.m03 == 1.f
                            && l_ChildWorld->m_WorldMatrix.m13 == 3.f
                            && l_ChildWorld->m_WorldMatrix.m23 == 3.f;
            }
        }

        l_Registry->CleanUp(ObjectLifespan::Frame);
    }

    TestRunner::EndTest(l_TestPassed);
}
```

**Important**: Before writing the translation element test assertions, verify how `Math::toTranslationMatrix` lays out the translation. Read `Source/Engine/Common/MathHelper.h` around lines 462–503 and check field names (`m00–m33` or `m[row][col]` layout). Adjust the `.m30/.m31/.m32` references if needed. If the matrix layout is column-major, translation will be in `m03/m13/m23`.

Also add `#include "../../Engine/Services/TransformService.h"` at the top of the test file.

Call `TestTransformPropagation()` from `RunEntityRegistryTests()`:

```cpp
void RunEntityRegistryTests()
{
    // ... existing calls ...
    TestTransformPropagation();
}
```

- [ ] **Step 2: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: zero errors.

- [ ] **Step 3: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: `TestTransformPropagation` passes. All other existing tests pass.

If `TestTransformPropagation` fails on the matrix element assertions: the world matrix layout differs from the assumption. Check `MathHelper.h::toTranslationMatrix` to find the actual translation fields and fix the test assertions.

- [ ] **Step 4: GPU validation test (final gate)**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code **0**.

- [ ] **Step 5: Commit**

```bash
git add Source/Test/UnitTests/EntityRegistryTests.cpp
git commit -m "test: add TransformService parent-child propagation unit test"
```

---

## Final verification checklist

After all tasks complete:

- [ ] `WorldTransformComponent.h` exists with `m_WorldMatrix` and `m_WorldRotationMatrix`
- [ ] `TransformComponent.h` has only `m_LocalPos`, `m_LocalRot`, `m_LocalScale` — no `m_WorldMatrix`, no `m_Dirty`
- [ ] `TransformService` is an `ISystem` registered with the engine (Setup/Init/Update/Terminate)
- [ ] `TransformService::Update()` runs after `EntityRegistry::Update()` and before `PerFrameDataService::Update()` in the engine loop
- [ ] `DrawCallService` reads `WorldTransformComponent::m_WorldMatrix` and `m_WorldRotationMatrix`
- [ ] `DX12RenderingServer::InitializeImpl` reads `WorldTransformComponent::m_WorldMatrix`
- [ ] `SetParent(child, parent)` / `ClearParent(child)` modify the linked-list correctly and mark dirty
- [ ] Scene loading callback resets all HierarchyNodes and forces traversal order rebuild
- [ ] All unit tests pass
- [ ] GPU validation exits 0
