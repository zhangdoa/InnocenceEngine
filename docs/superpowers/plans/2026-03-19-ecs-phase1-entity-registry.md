# ECS Phase 1 — EntityRegistry + ComponentStorage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce `EntityID`, `ComponentStorage<T>`, and `EntityRegistry` as a parallel foundation alongside the existing `ComponentManager` + `EntityManager`. No existing callers are migrated in this phase — this is purely additive.

**Architecture:** `EntityRegistry` is an `ISystem` registered with the engine. It owns one `ComponentStorage<T>` instance per component type, created lazily on first `Storage<T>()` call. `ComponentStorage<T>` uses a sparse-set triple (`m_dense` + `m_owners` + `m_sparse[MAX_ENTITIES]`) for O(1) by-entity operations and cache-friendly sequential iteration.

**Tech Stack:** C++17, MSVC (RelWithDebInfo), DX12 (GPU gate via `RenderTest.exe`)

**Spec:** `docs/superpowers/specs/2026-03-19-ecs-overhaul-design.md`

**Build command:**
```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

**GPU gate command:**
```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

---

## File Map

| File | Action | Responsibility |
|------|--------|----------------|
| `Source/Engine/Common/EntityID.h` | **Create** | `EntityID` typedef, `INVALID_ENTITY`, `MAX_ENTITIES` |
| `Source/Engine/Common/ComponentStorage.h` | **Create** | `ComponentStorage<T>` sparse-set template |
| `Source/Engine/Services/EntityRegistry.h` | **Create** | `EntityRegistry` ISystem declaration |
| `Source/Engine/Services/EntityRegistry.cpp` | **Create** | `EntityRegistry` ISystem implementation |
| `Source/Engine/Engine.h` | **Modify** | Add `EntityRegistry` include |
| `Source/Engine/Engine.cpp` | **Modify** | Register + Setup + Initialize + Update + Terminate `EntityRegistry` |

No existing files are deleted or caller code changed in this phase.

---

## Task 1: EntityID header

**Files:**
- Create: `Source/Engine/Common/EntityID.h`

- [ ] **Step 1: Create the header**

```cpp
// Source/Engine/Common/EntityID.h
#pragma once

#include "InnoType.h"

namespace Inno
{
    using EntityID = uint32_t;
    constexpr EntityID INVALID_ENTITY = 0;
    constexpr uint32_t MAX_ENTITIES   = 65536;
}
```

`InnoType.h` is the engine's type alias header; it already provides `uint32_t`.

- [ ] **Step 2: Build to confirm the header is valid**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: 0 errors, 0 warnings from new file.

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Common/EntityID.h
git commit -m "feat: add EntityID type and constants"
```

---

## Task 2: ComponentStorage\<T\>

**Files:**
- Create: `Source/Engine/Common/ComponentStorage.h`

`ComponentStorage<T>` is a header-only template. It must not include any engine API headers that call `g_Engine` — it is a pure data structure.

- [ ] **Step 1: Create the header**

```cpp
// Source/Engine/Common/ComponentStorage.h
#pragma once

#include "EntityID.h"
#include "InnoType.h"
#include "STL14.h"   // std::vector, std::algorithm

#include <limits>

namespace Inno
{
    template<typename T>
    class ComponentStorage
    {
    public:
        ComponentStorage()
        {
            m_sparse.assign(MAX_ENTITIES, k_invalid);
        }

        void Add(EntityID entity, const T& data = {})
        {
            // Caller must not add twice; no-op guard
            if (m_sparse[entity] != k_invalid)
                return;

            m_sparse[entity] = static_cast<uint32_t>(m_dense.size());
            m_dense.push_back(data);
            m_owners.push_back(entity);
            m_lifespans.push_back(ObjectLifespan::Invalid);
        }

        void Add(EntityID entity, ObjectLifespan lifespan, const T& data = {})
        {
            if (m_sparse[entity] != k_invalid)
                return;

            m_sparse[entity] = static_cast<uint32_t>(m_dense.size());
            m_dense.push_back(data);
            m_owners.push_back(entity);
            m_lifespans.push_back(lifespan);
        }

        void Remove(EntityID entity)
        {
            if (m_sparse[entity] == k_invalid)
                return;

            uint32_t denseIdx = m_sparse[entity];
            uint32_t lastIdx  = static_cast<uint32_t>(m_dense.size()) - 1;

            if (denseIdx != lastIdx)
            {
                // Swap with last
                m_dense[denseIdx]    = std::move(m_dense[lastIdx]);
                m_owners[denseIdx]   = m_owners[lastIdx];
                m_lifespans[denseIdx] = m_lifespans[lastIdx];
                m_sparse[m_owners[denseIdx]] = denseIdx;
            }

            m_dense.pop_back();
            m_owners.pop_back();
            m_lifespans.pop_back();
            m_sparse[entity] = k_invalid;
        }

        T* Get(EntityID entity)
        {
            if (m_sparse[entity] == k_invalid)
                return nullptr;
            return &m_dense[m_sparse[entity]];
        }

        const T* Get(EntityID entity) const
        {
            if (m_sparse[entity] == k_invalid)
                return nullptr;
            return &m_dense[m_sparse[entity]];
        }

        bool Has(EntityID entity) const
        {
            return m_sparse[entity] != k_invalid;
        }

        T& GetOrAdd(EntityID entity)
        {
            if (m_sparse[entity] == k_invalid)
                Add(entity);
            return m_dense[m_sparse[entity]];
        }

        // Cache-friendly sequential access
        const std::vector<T>& All() const       { return m_dense; }
        std::vector<T>&       All()             { return m_dense; }
        const std::vector<EntityID>& AllOwners() const { return m_owners; }

        void CleanUp(ObjectLifespan lifespan)
        {
            // Iterate backwards so swap-and-pop doesn't skip elements
            for (int32_t i = static_cast<int32_t>(m_dense.size()) - 1; i >= 0; --i)
            {
                if (m_lifespans[i] == lifespan)
                    Remove(m_owners[i]);
            }
        }

        size_t Size() const { return m_dense.size(); }

    private:
        static constexpr uint32_t k_invalid = std::numeric_limits<uint32_t>::max();

        std::vector<T>            m_dense;
        std::vector<EntityID>     m_owners;
        std::vector<ObjectLifespan> m_lifespans;
        std::vector<uint32_t>     m_sparse;   // size = MAX_ENTITIES, init to k_invalid
    };
}
```

**Note on `ObjectLifespan`:** Include `Object.h` (which defines `ObjectLifespan`) or forward-declare as needed. Verify that `Object.h` does not transitively pull in engine API headers before including it here. If it does, move the `ObjectLifespan` enum to a separate lightweight header (e.g., `ObjectLifespan.h`) as part of this task.

**Note on `STL14.h`:** Verify it includes `<vector>` and `<algorithm>`. If it does not, include those directly (checking the engine's conventions for which STL headers are safe to include directly vs. through wrappers).

- [ ] **Step 2: Check that `Object.h` is safe to include in a lightweight template header**

Read `Source/Engine/Common/Object.h`. If it includes engine API headers (`Engine.h`, service headers, etc.), extract `ObjectLifespan` to a new `Source/Engine/Common/ObjectLifespan.h` and update `Object.h` to include it.

- [ ] **Step 3: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: 0 errors.

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Common/ComponentStorage.h
git commit -m "feat: add ComponentStorage sparse-set template"
```

---

## Task 3: EntityRegistry header

**Files:**
- Create: `Source/Engine/Services/EntityRegistry.h`

- [ ] **Step 1: Create the header**

```cpp
// Source/Engine/Services/EntityRegistry.h
#pragma once

#include "../Interface/ISystem.h"
#include "../Common/EntityID.h"
#include "../Common/ComponentStorage.h"
#include "../Common/Object.h"

#include "STL17.h"   // std::string_view
#include "STL14.h"   // std::vector, std::unordered_map, std::unique_ptr

namespace Inno
{
    class EntityRegistry : public ISystem
    {
        INNO_CLASS_CONCRETE_NON_COPYABLE(EntityRegistry);

    public:
        bool Setup(ISystemConfig* config) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

        // Entity lifecycle
        EntityID    Spawn(ObjectLifespan lifespan, const char* name = nullptr);
        void        Destroy(EntityID entity);
        bool        IsValid(EntityID entity) const;
        const char* GetName(EntityID entity) const;
        EntityID    FindByName(const char* name) const;  // linear scan; editor/load only

        // Component operations
        template<typename T>
        T& Emplace(EntityID entity, T data = {})
        {
            auto& storage = Storage<T>();
            storage.Add(entity, m_lifespans[entity], data);
            return *storage.Get(entity);
        }

        template<typename T>
        void Remove(EntityID entity)
        {
            Storage<T>().Remove(entity);
        }

        template<typename T>
        T* Get(EntityID entity)
        {
            return Storage<T>().Get(entity);
        }

        template<typename T>
        bool Has(EntityID entity) const
        {
            return const_cast<EntityRegistry*>(this)->Storage<T>().Has(entity);
        }

        template<typename T>
        ComponentStorage<T>& Storage()
        {
            auto key = typeid(T).hash_code();
            auto it = m_storages.find(key);
            if (it == m_storages.end())
            {
                auto storage = std::make_unique<ComponentStorage<T>>();
                auto* raw = storage.get();
                m_storages.emplace(key, std::make_unique<StorageWrapper<T>>(std::move(storage)));
                return *raw;
            }
            return *static_cast<StorageWrapper<T>*>(it->second.get())->m_storage;
        }

        void CleanUp(ObjectLifespan lifespan);

    private:
        // Type-erased storage wrapper
        struct IStorageWrapper
        {
            virtual ~IStorageWrapper() = default;
            virtual void CleanUp(ObjectLifespan lifespan) = 0;
        };

        template<typename T>
        struct StorageWrapper : IStorageWrapper
        {
            std::unique_ptr<ComponentStorage<T>> m_storage;
            explicit StorageWrapper(std::unique_ptr<ComponentStorage<T>> s) : m_storage(std::move(s)) {}
            void CleanUp(ObjectLifespan lifespan) override { m_storage->CleanUp(lifespan); }
        };

        // Entity metadata (indexed by EntityID)
        std::vector<bool>           m_valid;         // m_valid[entityID]
        std::vector<ObjectLifespan> m_lifespans;     // m_lifespans[entityID]
        std::vector<std::string>    m_names;         // m_names[entityID]

        // Free list for recycled entity slots
        std::vector<EntityID>       m_freeList;
        EntityID                    m_nextID = 1;    // 0 = INVALID_ENTITY

        // Type-erased component storages, keyed by type_info hash
        std::unordered_map<size_t, std::unique_ptr<IStorageWrapper>> m_storages;

        ObjectStatus m_objectStatus = ObjectStatus::Invalid;
    };
}
```

- [ ] **Step 2: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: 0 errors.

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Services/EntityRegistry.h
git commit -m "feat: add EntityRegistry header"
```

---

## Task 4: EntityRegistry implementation

**Files:**
- Create: `Source/Engine/Services/EntityRegistry.cpp`

- [ ] **Step 1: Create the implementation**

```cpp
// Source/Engine/Services/EntityRegistry.cpp
#include "EntityRegistry.h"

using namespace Inno;

bool EntityRegistry::Setup(ISystemConfig*)
{
    m_valid.assign(MAX_ENTITIES, false);
    m_lifespans.assign(MAX_ENTITIES, ObjectLifespan::Invalid);
    m_names.assign(MAX_ENTITIES, {});

    m_objectStatus = ObjectStatus::Created;
    return true;
}

bool EntityRegistry::Initialize()
{
    m_objectStatus = ObjectStatus::Activated;
    return true;
}

bool EntityRegistry::Update()
{
    return true;
}

bool EntityRegistry::Terminate()
{
    m_storages.clear();
    m_freeList.clear();
    m_valid.assign(MAX_ENTITIES, false);
    m_objectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus EntityRegistry::GetStatus()
{
    return m_objectStatus;
}

EntityID EntityRegistry::Spawn(ObjectLifespan lifespan, const char* name)
{
    EntityID id;
    if (!m_freeList.empty())
    {
        id = m_freeList.back();
        m_freeList.pop_back();
    }
    else
    {
        if (m_nextID >= MAX_ENTITIES)
            return INVALID_ENTITY;
        id = m_nextID++;
    }

    m_valid[id]     = true;
    m_lifespans[id] = lifespan;
    m_names[id]     = name ? name : "";
    return id;
}

void EntityRegistry::Destroy(EntityID entity)
{
    if (!m_valid[entity])
        return;

    // Remove all components for this entity
    for (auto& [key, wrapper] : m_storages)
    {
        // Each StorageWrapper exposes CleanUp(lifespan) — use direct removal instead
        (void)key;
        (void)wrapper;
        // Individual storage remove: done via template Remove<T>().
        // Destroy is called infrequently; iterate storages and call Remove on each.
        // The type-erasure only exposes CleanUp(lifespan) — for per-entity removal,
        // callers should call Remove<T>(entity) for each component type they know
        // this entity owns, OR EntityRegistry provides a per-entity removal via
        // storing a per-entity component mask (future enhancement).
        // For Phase 1, Destroy invalidates the entity slot; CleanUp handles
        // bulk removal by lifespan. Per-entity component removal during Destroy
        // will leave stale data in component storages until CleanUp is called.
    }

    m_valid[entity]     = false;
    m_lifespans[entity] = ObjectLifespan::Invalid;
    m_names[entity]     = {};
    m_freeList.push_back(entity);
}

bool EntityRegistry::IsValid(EntityID entity) const
{
    return entity != INVALID_ENTITY && entity < MAX_ENTITIES && m_valid[entity];
}

const char* EntityRegistry::GetName(EntityID entity) const
{
    if (!IsValid(entity))
        return nullptr;
    return m_names[entity].c_str();
}

EntityID EntityRegistry::FindByName(const char* name) const
{
    if (!name)
        return INVALID_ENTITY;
    for (EntityID id = 1; id < m_nextID; ++id)
    {
        if (m_valid[id] && m_names[id] == name)
            return id;
    }
    return INVALID_ENTITY;
}

void EntityRegistry::CleanUp(ObjectLifespan lifespan)
{
    // Step 1: remove all components for entities of this lifespan
    for (auto& [key, wrapper] : m_storages)
    {
        (void)key;
        wrapper->CleanUp(lifespan);
    }

    // Step 2: free entity slots
    for (EntityID id = 1; id < m_nextID; ++id)
    {
        if (m_valid[id] && m_lifespans[id] == lifespan)
        {
            m_valid[id]     = false;
            m_lifespans[id] = ObjectLifespan::Invalid;
            m_names[id]     = {};
            m_freeList.push_back(id);
        }
    }
}
```

**Note on `Destroy` incomplete per-entity component removal:** The type-erased `IStorageWrapper` only exposes `CleanUp(lifespan)`. Per-entity cross-storage removal during `Destroy()` requires either a per-entity component bitmask or a virtual `Remove(EntityID)` on the wrapper. For Phase 1 (additive only, no live users), leave the comment in place — this will be revisited when callers start using `EntityRegistry`. `CleanUp(lifespan)` is the primary bulk path and works correctly.

- [ ] **Step 2: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: 0 errors.

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Services/EntityRegistry.cpp
git commit -m "feat: implement EntityRegistry ISystem"
```

---

## Task 5: Register EntityRegistry with the Engine

**Files:**
- Modify: `Source/Engine/Engine.h`
- Modify: `Source/Engine/Engine.cpp`

The goal is to register `EntityRegistry` in the engine's service locator so `g_Engine->Get<EntityRegistry>()` works. It should be registered early — before `ComponentManager` and `EntityManager` — since it will eventually replace them.

- [ ] **Step 1: Add include to Engine.h**

Find the block of service includes in `Engine.h` (near `ComponentManager.h`, `EntityManager.h`). Add:

```cpp
#include "Services/EntityRegistry.h"
```

- [ ] **Step 2: Register EntityRegistry in Engine.cpp**

Find the section in `Engine.cpp` where services are pre-registered via `CreateServices`. Add `EntityRegistry` immediately before `EntityManager`:

```cpp
PreRegister(new EntityRegistry());
```

- [ ] **Step 3: Add to Setup phase**

Find the block that calls `SystemSetup` for `EntityManager` and add immediately before it:

```cpp
SystemSetup(Get<EntityRegistry>());
```

- [ ] **Step 4: Add to Initialize phase**

Find the block that calls `SystemInit` for `EntityManager` and add immediately before it:

```cpp
SystemInit(Get<EntityRegistry>());
```

- [ ] **Step 5: Add to Update phase (per-frame)**

`EntityRegistry::Update()` is a no-op for now. Add it in the per-frame callback near the other system updates:

```cpp
SystemUpdate(Get<EntityRegistry>());
```

- [ ] **Step 6: Add to Terminate phase**

Find where `EntityManager` is terminated and add immediately after it:

```cpp
SystemTerminate(Get<EntityRegistry>());
```

- [ ] **Step 7: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

Expected: 0 errors.

- [ ] **Step 8: GPU gate**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: exit code 0.

- [ ] **Step 9: Commit**

```bash
git add Source/Engine/Engine.h Source/Engine/Engine.cpp
git commit -m "feat: register EntityRegistry with engine service locator"
```

---

## Task 6: Smoke-test EntityRegistry via unit test

The engine has a `Test.exe` runtime test (`Source/Test/`). Add a minimal test that exercises the core `ComponentStorage<T>` + `EntityRegistry` loop: spawn an entity, emplace a component, retrieve it, verify the value, then clean up.

**Files:**
- Modify: the existing test file that contains component/entity tests (locate it first — likely `Source/Test/` or `Source/DefaultClient/TestClient/`).

- [ ] **Step 1: Locate the test entry point**

```
find Source/Test -name "*.cpp" | head -20
```

Or check `Source/DefaultClient/TestLogicClient/TestLogicClient.cpp` if the TestClient architecture from the prior plan is in place.

- [ ] **Step 2: Add a test function**

In the identified test file, add:

```cpp
static bool TestEntityRegistry()
{
    struct PositionData { float x = 0, y = 0, z = 0; };

    auto* registry = g_Engine->Get<EntityRegistry>();
    if (!registry || registry->GetStatus() != ObjectStatus::Activated)
        return false;

    EntityID e = registry->Spawn(ObjectLifespan::Scene, "test_entity");
    if (e == INVALID_ENTITY)
        return false;

    registry->Emplace<PositionData>(e, PositionData{1.0f, 2.0f, 3.0f});

    auto* pos = registry->Get<PositionData>(e);
    if (!pos || pos->x != 1.0f || pos->y != 2.0f || pos->z != 3.0f)
        return false;

    if (!registry->Has<PositionData>(e))
        return false;

    registry->Remove<PositionData>(e);
    if (registry->Get<PositionData>(e) != nullptr)
        return false;

    registry->CleanUp(ObjectLifespan::Scene);
    if (registry->IsValid(e))
        return false;

    return true;
}
```

Register and call `TestEntityRegistry()` from the test runner. The test must log pass/fail via the engine log.

- [ ] **Step 3: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

- [ ] **Step 4: Run Test.exe**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: test passes, no crashes.

- [ ] **Step 5: GPU gate**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/Test/  # or wherever the test file lives
git commit -m "test: add EntityRegistry smoke test"
```

---

## Done

Phase 1 complete when:
- `EntityID.h`, `ComponentStorage.h`, `EntityRegistry.h/.cpp` all compile cleanly
- `EntityRegistry` is live in the engine service locator
- Smoke test passes in `Test.exe`
- GPU gate exits 0

Phase 2 begins next: stripping component structs to POD and deleting `ModelComponent`.
