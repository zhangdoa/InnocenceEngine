# ECS Phase 2: Component POD Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace all `Component`/`Object`-based component types with plain structs managed by `EntityRegistry`; delete `ComponentManager` and `EntityManager`.

**Architecture:** New component types are plain C++ structs with no base class. `EntityRegistry` owns all component storage via sparse sets keyed by `EntityID`. The old `EntityManager` + `ComponentManager` infrastructure stays alive until every caller is migrated, then deleted as a unit. Migration proceeds subsystem-by-subsystem; the build stays green at every commit.

**Tech Stack:** C++17, `EntityRegistry` + `TComponentStorage<T>` (Phase 1), `EntityID = uint32_t`, Math.h types, cmake build (`cmake --build build --config RelWithDebInfo -j`).

---

## Migration API Quick Reference

Every caller site maps directly:

| Old | New |
|-----|-----|
| `g_Engine->Get<EntityManager>()->Spawn(ser, lifespan, name)` | `g_Engine->Get<EntityRegistry>()->Spawn(lifespan, name)` |
| `g_Engine->Get<EntityManager>()->Destroy(entity)` | `g_Engine->Get<EntityRegistry>()->Destroy(entityID)` |
| `g_Engine->Get<EntityManager>()->Find(name)` | `g_Engine->Get<EntityRegistry>()->FindByName(name)` |
| `g_Engine->Get<ComponentManager>()->Spawn<T>(owner, …)` | `g_Engine->Get<EntityRegistry>()->Emplace<T>(entityID)` or `Emplace<T>(entityID, T{...})` to supply initial data |
| `g_Engine->Get<ComponentManager>()->Find<T>(entity)` | `g_Engine->Get<EntityRegistry>()->Get<T>(entityID)` |
| `g_Engine->Get<ComponentManager>()->GetAll<T>()` | `g_Engine->Get<EntityRegistry>()->Storage<T>().All()` |
| Parallel entity iteration | `Storage<T>().AllOwners()[i]` alongside `Storage<T>().All()[i]` |
| `g_Engine->Get<ComponentManager>()->Destroy<T>(comp)` | `g_Engine->Get<EntityRegistry>()->Remove<T>(entityID)` |
| `component->m_ObjectStatus == ObjectStatus::Activated` | pointer returned by `Get<T>()` is valid (registry handles lifetime) |
| `component->m_Owner` (old `Entity*`) | carry `EntityID` alongside; pass it explicitly |
| UUID lookups via `FindByUUID<T>()` | `EntityID` is the identity; no UUID needed |

Iteration pattern (replaces `GetAll<T>()` loops):

```cpp
auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
const auto& l_Lights  = l_Storage.All();
const auto& l_Owners  = l_Storage.AllOwners();
for (size_t i = 0; i < l_Lights.size(); i++)
{
    const LightComponent& l_Light  = l_Lights[i];
    EntityID              l_Entity = l_Owners[i];
    // optional companion component:
    auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_Entity);
}
```

---

## File Structure

**New files — component headers:**
- `Source/Engine/Component/TransformComponent.h` — local pos/rot/scale + cached world matrix + dirty flag
- `Source/Engine/Component/VisibilityComponent.h` — AABB, visible flag, cast-shadow flag
- `Source/Engine/Component/AnimationStateComponent.h` — clip name, elapsed time, loop/playing flags
- `Source/Engine/Component/RigidBodyComponent.h` — velocity, angular velocity, mass, simulation proxy
- `Source/Engine/Component/CollisionShapeComponent.h` — shape type, half-extents, local offset

**Modified component headers (strip base class and embedded Transform):**
- `Source/Engine/Component/LightComponent.h` — remove `Component` base, static type methods, `m_Transform`
- `Source/Engine/Component/CameraComponent.h` — remove `Component` base, static type methods, `m_Transform`
- `Source/Engine/Component/MeshComponent.h` — remove `Component` base, static type methods
- `Source/Engine/Component/MaterialComponent.h` — remove `Component` base, static type methods
- `Source/Engine/Component/SkeletonComponent.h` — remove `Component` base, static type methods

**Deleted:**
- `Source/Engine/Component/ModelComponent.h` (defines both `ModelComponent` and `DrawCallComponent`)
- `Source/Engine/Services/ComponentManager.h` + `ComponentManager.cpp`
- `Source/Engine/Services/EntityManager.h` + `EntityManager.cpp`
- `Source/Engine/Common/Entity.h` (if unreferenced after migration)

**Modified services — full list of migration targets:**
```
Source/Engine/Services/DrawCallService.cpp/.h
Source/Engine/Services/LightSystem.cpp
Source/Engine/Services/CameraSystem.cpp
Source/Engine/Services/LightDataService.cpp
Source/Engine/Services/AnimationService.cpp/.h
Source/Engine/Services/PhysicsSimulationService.cpp
Source/Engine/Services/BillboardDrawCallService.cpp
Source/Engine/Services/PerFrameDataService.cpp
Source/Engine/Services/AssetService.cpp
Source/Engine/Services/TemplateAssetService.cpp
Source/Engine/Services/SceneService.cpp
Source/Engine/RenderingServer/Common/IRenderingServer.cpp
Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp
Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp
Source/Engine/RenderingServer/VK/VKRenderingServer.cpp
Source/Engine/RenderingServer/VK/VKRenderingServer_VulkanObject.cpp
Source/Engine/RenderingServer/VK/VKRenderingServer_GraphicsDevice.cpp
Source/Engine/RenderingServer/VK/VKRenderingServer_EngineComponent.cpp
Source/Engine/RenderingServer/VK/VKRenderingServer_ComponentPool.cpp
Source/Engine/ThirdParty/AssimpWrapper/AssimpMeshProcessor.cpp
Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp
Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp
Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp
Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp
Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp
Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_POD.cpp
Source/Engine/RayTracer/RayTracer.cpp
Source/DefaultClient/LogicClient/World.inl
Source/DefaultClient/LogicClient/AnimationController.inl
Source/DefaultClient/LogicClient/Player.inl
Source/DefaultClient/RenderingClient/GIResolvePass.cpp
Source/DefaultClient/RenderingClient/DebugPass.cpp
Source/Tool/Baker/Baker.cpp
Source/Tool/Baker/BrickGenerator.cpp
Source/Editor/worldexplorer.cpp
Source/Editor/modelcomponentpropertyeditor.cpp
Source/Engine/Engine.cpp
```

---

## Task 1: Define New POD Component Type Headers

**Files:**
- Create: `Source/Engine/Component/TransformComponent.h`
- Create: `Source/Engine/Component/VisibilityComponent.h`
- Create: `Source/Engine/Component/AnimationStateComponent.h`
- Create: `Source/Engine/Component/RigidBodyComponent.h`
- Create: `Source/Engine/Component/CollisionShapeComponent.h`

- [ ] **Step 1: Create TransformComponent.h**

```cpp
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct TransformComponent
    {
        Vec3 m_LocalPos   = {};
        Vec4 m_LocalRot   = Vec4(0.f, 0.f, 0.f, 1.f);  // quaternion XYZW
        Vec3 m_LocalScale = Vec3(1.f, 1.f, 1.f);
        Mat4 m_WorldMatrix = {};
        bool m_Dirty = true;
    };
}
```

- [ ] **Step 2: Create VisibilityComponent.h**

```cpp
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct VisibilityComponent
    {
        AABB m_AABB        = {};
        bool m_Visible     = true;
        bool m_CastShadow  = true;
    };
}
```

- [ ] **Step 3: Create AnimationStateComponent.h**

```cpp
#pragma once
#include "../Common/STL14.h"  // std::string lives in STL14, not STL17

namespace Inno
{
    struct AnimationStateComponent
    {
        std::string m_ClipName;
        float m_ElapsedTime = 0.f;
        bool  m_Loop        = true;
        bool  m_Playing     = false;
    };
}
```

- [ ] **Step 4: Create RigidBodyComponent.h**

```cpp
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct RigidBodyComponent
    {
        Vec3  m_LinearVelocity  = {};
        Vec3  m_AngularVelocity = {};
        float m_Mass            = 1.f;
        void* m_SimulationProxy = nullptr;
    };
}
```

- [ ] **Step 5: Create CollisionShapeComponent.h**

```cpp
#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    enum class CollisionShapeType { Box, Sphere, Capsule };

    struct CollisionShapeComponent
    {
        CollisionShapeType m_ShapeType  = CollisionShapeType::Box;
        Vec3               m_HalfExtents = Vec3(0.5f, 0.5f, 0.5f);
        Vec3               m_LocalOffset = {};
    };
}
```

- [ ] **Step 6: Build — confirm headers compile cleanly**

```
cmake --build build --config RelWithDebInfo -j
```

Expected: no new errors.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Component/TransformComponent.h \
        Source/Engine/Component/VisibilityComponent.h \
        Source/Engine/Component/AnimationStateComponent.h \
        Source/Engine/Component/RigidBodyComponent.h \
        Source/Engine/Component/CollisionShapeComponent.h
git commit -m "feat: add POD component types for ECS Phase 2"
```

---

## Task 2: Unit Tests for New Component Types

**Files:**
- Modify: `Source/Test/UnitTests/EntityRegistryTests.cpp`

- [ ] **Step 1: Add includes at the top of EntityRegistryTests.cpp**

```cpp
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/VisibilityComponent.h"
#include "../../Engine/Component/RigidBodyComponent.h"
```

- [ ] **Step 2: Add the test function before `RunEntityRegistryUnitTests()`**

```cpp
static void TestNewComponentTypes()
{
    auto* l_Registry = g_Engine->Get<EntityRegistry>();

    auto l_Entity = l_Registry->Spawn(ObjectLifespan::Frame, "component_type_test");

    // TransformComponent
    auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
    l_Transform.m_LocalPos = Vec3(1.f, 2.f, 3.f);
    auto* l_TPtr = l_Registry->Get<TransformComponent>(l_Entity);
    assert(l_TPtr != nullptr && "TransformComponent should be retrievable");
    assert(l_TPtr->m_LocalPos.x == 1.f && "TransformComponent position x should be 1");
    assert(l_TPtr->m_Dirty == true && "TransformComponent should start dirty");

    // VisibilityComponent — default
    l_Registry->Emplace<VisibilityComponent>(l_Entity);
    auto* l_Vis = l_Registry->Get<VisibilityComponent>(l_Entity);
    assert(l_Vis != nullptr && "VisibilityComponent should be retrievable");
    assert(l_Vis->m_Visible == true && "VisibilityComponent should default to visible");

    // RigidBodyComponent
    l_Registry->Emplace<RigidBodyComponent>(l_Entity);
    assert(l_Registry->Has<RigidBodyComponent>(l_Entity) && "Entity should have RigidBodyComponent");

    // Multiple components on same entity
    assert(l_Registry->Has<TransformComponent>(l_Entity));
    assert(l_Registry->Has<VisibilityComponent>(l_Entity));
    assert(l_Registry->Has<RigidBodyComponent>(l_Entity));

    // CleanUp removes all Frame-lifespan entities and their components
    l_Registry->CleanUp(ObjectLifespan::Frame);
    assert(!l_Registry->IsValid(l_Entity) && "Entity should be invalid after CleanUp");

    Log(LogLevel::Success, "EntityRegistry new component types — PASSED");
}
```

- [ ] **Step 3: Call from `RunEntityRegistryUnitTests()`**

Add `TestNewComponentTypes();` inside the function body.

- [ ] **Step 4: Build**

```
cmake --build build --config RelWithDebInfo -j
```

- [ ] **Step 5: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: `EntityRegistry new component types — PASSED` plus all prior tests green.

- [ ] **Step 6: Commit**

```bash
git add Source/Test/UnitTests/EntityRegistryTests.cpp
git commit -m "test: unit tests for ECS Phase 2 POD component types"
```

---

## Task 3: Strip MeshComponent, MaterialComponent, SkeletonComponent

Remove the `Component` base class, `GetTypeID()`, `GetTypeName()` from these three headers. Keep all data fields exactly as they are. Change `class` to `struct`. Remove `#include "../Common/Object.h"` only if no other types from it are needed in the file.

**Files:**
- Modify: `Source/Engine/Component/MeshComponent.h`
- Modify: `Source/Engine/Component/MaterialComponent.h`
- Modify: `Source/Engine/Component/SkeletonComponent.h`

**Pattern:**

```cpp
// Before:
class MeshComponent : public Component {
    static uint32_t GetTypeID() { return 6; }
    static const char* GetTypeName() { return "MeshComponent"; }
    // ... data fields ...
};

// After:
struct MeshComponent {
    // ... data fields unchanged ...
};
```

- [ ] **Step 1: Read MeshComponent.h, then apply the pattern**

- [ ] **Step 2: Read MaterialComponent.h, then apply the pattern**

- [ ] **Step 3: Read SkeletonComponent.h, then apply the pattern**

- [ ] **Step 4: Build — fix cascade errors concretely**

```
cmake --build build --config RelWithDebInfo -j
```

Cascade errors are expected from call sites that access removed fields (`m_UUID`, `m_ObjectStatus`, `m_Owner`, `GetTypeID()`). Apply the following fixes to reach a green build:

- **`m_ObjectStatus` checks** (e.g., `if (comp->m_ObjectStatus != ObjectStatus::Activated)`) — delete the guard entirely; `EntityRegistry` manages lifetime, so every pointer returned by `Get<T>()` is valid.
- **`m_Owner` accesses** — the owner entity will be carried explicitly as `EntityID` after full migration. For now, if the owning file is in the Tasks 6–12 migration list, comment out the expression with `// TODO Phase2-migrate <filename>` so the line is unreachable but the compiler sees no error.
- **`GetTypeID()` / `GetTypeName()` calls** — same treatment: comment out with `// TODO Phase2-migrate`.
- **If a cascade error is in a header** (not a .cpp), fix it immediately rather than commenting out, since it would break every translation unit that includes it.

After applying the above, the build must be fully green before proceeding.

- [ ] **Step 5: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Component/MeshComponent.h \
        Source/Engine/Component/MaterialComponent.h \
        Source/Engine/Component/SkeletonComponent.h
git commit -m "refactor: strip MeshComponent/MaterialComponent/SkeletonComponent to plain structs"
```

---

## Task 4: Strip LightComponent

Remove `Component` base class, static type methods, and the embedded `m_Transform` field. Transform data will come from a `TransformComponent` on the same entity.

**Files:**
- Modify: `Source/Engine/Component/LightComponent.h`

- [ ] **Step 1: Read LightComponent.h**

- [ ] **Step 2: Apply the strip**

```cpp
// Before:
class LightComponent : public Component {
    static uint32_t GetTypeID() { return 3; }
    static const char* GetTypeName() { return "LightComponent"; }
    Transform m_Transform = {};
    Vec4 m_RGBColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    // ...
};

// After:
struct LightComponent {
    Vec4 m_RGBColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    Vec4 m_Shape = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    LightType m_LightType = LightType::Directional;
    float m_ColorTemperature = 5780.0f;
    float m_LuminousFlux = 1.0f;
    bool m_UseColorTemperature = true;
    std::vector<AABB> m_LitRegion_WorldSpace;
    std::vector<AABB> m_LitRegion_LightSpace;
    std::vector<Mat4> m_ViewMatrices;
    std::vector<Mat4> m_ProjectionMatrices;
};
```

Remove `#include "../Common/Object.h"`.

- [ ] **Step 3: Grep for `m_Transform` on LightComponent and annotate**

```
grep -rn "LightComponent" Source/ --include="*.cpp" --include="*.inl" | grep "m_Transform"
```

Add `// TODO Phase2-migrate` to each match to keep build green.

- [ ] **Step 4: Build + test**

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Component/LightComponent.h
git commit -m "refactor: strip LightComponent to plain struct, remove embedded Transform"
```

---

## Task 5: Strip CameraComponent

Same pattern as Task 4.

**Files:**
- Modify: `Source/Engine/Component/CameraComponent.h`

- [ ] **Step 1: Read CameraComponent.h**

- [ ] **Step 2: Apply the strip**

```cpp
// After:
struct CameraComponent {
    Mat4 m_projectionMatrix = {};
    Frustum m_frustum = {};
    Ray m_rayOfEye = {};
    float m_FOVX = 90.0f;
    float m_widthScale = 16.0f;
    float m_heightScale = 9.0f;
    float m_zNear = 0.001f;
    float m_zFar = 1000.0f;
    float m_WHRatio = m_widthScale / m_heightScale;
    float m_aperture = 2.2f;
    float m_shutterTime = 1.0f / 2000.0f;
    float m_ISO = 100.0f;
    std::vector<Vertex> m_splitFrustumVerticesWS;
};
```

`ICameraSystem` interface remains in the file unchanged.

- [ ] **Step 3: Grep and annotate `m_Transform` usages on CameraComponent**

- [ ] **Step 4: Build + test**

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Component/CameraComponent.h
git commit -m "refactor: strip CameraComponent to plain struct, remove embedded Transform"
```

---

## Task 6: Migrate DrawCallService

The new entity model: one entity per mesh+material pair, carrying `MeshComponent` + `MaterialComponent` + `TransformComponent` + `VisibilityComponent`. `ModelComponent` and `DrawCallComponent` are not used by this service after migration.

**Files:**
- Modify: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/Services/DrawCallService.h`

- [ ] **Step 1: Read both files in full**

- [ ] **Step 2: Replace GetAll<ModelComponent> iteration**

Old pattern:
```cpp
auto l_comps = g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>();
for (auto* l_model : l_comps) {
    if (l_model->m_ObjectStatus != ObjectStatus::Activated) continue;
    for (auto l_dcID : l_model->m_DrawCallComponents) {
        auto* l_dc = g_Engine->Get<ComponentManager>()->FindByUUID<DrawCallComponent>(l_dcID);
        // use l_dc->m_MeshComponent, l_dc->m_MaterialComponent
    }
}
```

New pattern:
```cpp
auto& l_MeshStorage = g_Engine->Get<EntityRegistry>()->Storage<MeshComponent>();
const auto& l_Meshes  = l_MeshStorage.All();
const auto& l_Owners  = l_MeshStorage.AllOwners();
for (size_t i = 0; i < l_Meshes.size(); i++)
{
    EntityID l_Entity = l_Owners[i];
    const MeshComponent& l_Mesh = l_Meshes[i];
    auto* l_Material = g_Engine->Get<EntityRegistry>()->Get<MaterialComponent>(l_Entity);
    auto* l_Vis      = g_Engine->Get<EntityRegistry>()->Get<VisibilityComponent>(l_Entity);
    if (!l_Material)                        continue;
    if (l_Vis && !l_Vis->m_Visible)        continue;
    // build DrawCallData from l_Mesh + *l_Material
}
```

- [ ] **Step 3: Replace includes**

Remove `#include "ComponentManager.h"` and `#include "EntityManager.h"`.
Add `#include "EntityRegistry.h"` (and `#include "../Component/TransformComponent.h"` etc. as needed).

- [ ] **Step 4: Build — fix remaining issues**

```
cmake --build build --config RelWithDebInfo -j
```

- [ ] **Step 5: Run tests + GPU gate**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Both must be 0.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/DrawCallService.cpp Source/Engine/Services/DrawCallService.h
git commit -m "refactor: migrate DrawCallService to EntityRegistry"
```

---

## Task 7: Migrate LightSystem, CameraSystem, LightDataService

`LightComponent` and `CameraComponent` no longer embed `m_Transform`; position/orientation now comes from `TransformComponent` looked up by EntityID.

**Files:**
- Modify: `Source/Engine/Services/LightSystem.cpp`
- Modify: `Source/Engine/Services/CameraSystem.cpp`
- Modify: `Source/Engine/Services/LightDataService.cpp`

- [ ] **Step 1: Read all three files**

- [ ] **Step 2: Apply migration in LightSystem**

Old:
```cpp
auto l_comps = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
for (auto* l_light : l_comps) {
    auto& l_pos = l_light->m_Transform.m_pos;
    // ...
}
```

New:
```cpp
auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
const auto& l_Lights = l_Storage.All();
const auto& l_Owners = l_Storage.AllOwners();
for (size_t i = 0; i < l_Lights.size(); i++)
{
    const LightComponent& l_Light = l_Lights[i];
    auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_Owners[i]);
    Vec3 l_pos = l_Transform ? l_Transform->m_LocalPos : Vec3{};
    // ...
}
```

- [ ] **Step 3: Apply same pattern in CameraSystem**

Camera position/orientation now from `TransformComponent`. The `ICameraSystem::GetMainCamera()` / `SetMainCamera()` pattern can keep using `CameraComponent*` — add a companion lookup for its entity's `TransformComponent` where needed.

- [ ] **Step 4: Apply migration in LightDataService**

- [ ] **Step 5: Build + test**

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/LightSystem.cpp \
        Source/Engine/Services/CameraSystem.cpp \
        Source/Engine/Services/LightDataService.cpp
git commit -m "refactor: migrate LightSystem and CameraSystem to EntityRegistry"
```

---

## Task 8: Migrate AnimationService and PhysicsSimulationService

**Files:**
- Modify: `Source/Engine/Services/AnimationService.cpp`
- Modify: `Source/Engine/Services/AnimationService.h`
- Modify: `Source/Engine/Services/PhysicsSimulationService.cpp`

- [ ] **Step 1: Read all three files**

- [ ] **Step 2: Update AnimationService.h — change public API signatures**

```cpp
// Old:
void PlayAnimation(ModelComponent* modelComp, const std::string& animPath, bool loop);
void StopAnimation(ModelComponent* modelComp, const std::string& animPath);

// New:
void PlayAnimation(EntityID entity, const std::string& animPath, bool loop);
void StopAnimation(EntityID entity);
```

Remove `#include` for `ModelComponent.h`, add `#include "../Common/EntityID.h"`.

- [ ] **Step 3: Update AnimationService.cpp implementation**

Replace internal `ComponentManager::Find<ModelComponent>` lookups with `EntityRegistry::Get<AnimationStateComponent>`.

- [ ] **Step 4: Migrate PhysicsSimulationService**

Replace `EntityManager::Spawn` + `ComponentManager::Spawn<ModelComponent>` with:
```cpp
auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Scene, name);
g_Engine->Get<EntityRegistry>()->Emplace<TransformComponent>(l_Entity);
g_Engine->Get<EntityRegistry>()->Emplace<RigidBodyComponent>(l_Entity);
g_Engine->Get<EntityRegistry>()->Emplace<CollisionShapeComponent>(l_Entity);
```

- [ ] **Step 5: Build + test**

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/AnimationService.cpp \
        Source/Engine/Services/AnimationService.h \
        Source/Engine/Services/PhysicsSimulationService.cpp
git commit -m "refactor: migrate AnimationService and PhysicsSimulationService to EntityRegistry"
```

---

## Task 9: Migrate AssimpWrapper (Asset Import Pipeline)

The importer builds entities at load time. Old: one `ModelComponent` per asset, child `DrawCallComponent` per submesh via UUID vector. New: one entity per submesh, with `MeshComponent` + `MaterialComponent` + `TransformComponent` + `VisibilityComponent` all on the same entity.

**Files:**
- Modify: `Source/Engine/ThirdParty/AssimpWrapper/AssimpMeshProcessor.cpp`
- Modify: `Source/Engine/ThirdParty/AssimpWrapper/AssimpMaterialProcessor.cpp`
- Modify: `Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp`
- Modify: `Source/Engine/ThirdParty/AssimpWrapper/AssimpImporter.cpp`

- [ ] **Step 1: Read AssimpMeshProcessor.cpp and AssimpImporter.cpp in full**

- [ ] **Step 2: Apply migration in AssimpMeshProcessor**

Old (from codebase audit):
```cpp
auto l_entity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, name);
auto* l_mesh  = g_Engine->Get<ComponentManager>()->Spawn<MeshComponent>(l_entity, true, ObjectLifespan::Frame);
// populate l_mesh ...
g_Engine->Get<EntityManager>()->Destroy(l_entity);
```

New:
```cpp
auto l_entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Frame, name);
auto& l_mesh  = g_Engine->Get<EntityRegistry>()->Emplace<MeshComponent>(l_entity);
// populate l_mesh ...
g_Engine->Get<EntityRegistry>()->Destroy(l_entity);
```

- [ ] **Step 3: Apply same pattern in AssimpMaterialProcessor and AssimpTextureProcessor**

- [ ] **Step 4: Migrate AssimpImporter — replace ModelComponent/DrawCallComponent construction**

Read the file first. For each submesh the importer processes:

```cpp
// Old: one ModelComponent owner, DrawCallComponent per submesh
auto* l_model = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(ownerEntity, true, lifespan);
l_model->m_DrawCallComponents.push_back(drawCallUUID);

// New: one entity per submesh
auto l_meshEntity = g_Engine->Get<EntityRegistry>()->Spawn(lifespan, meshName);
g_Engine->Get<EntityRegistry>()->Emplace<MeshComponent>(l_meshEntity, meshData);
g_Engine->Get<EntityRegistry>()->Emplace<MaterialComponent>(l_meshEntity, materialData);
g_Engine->Get<EntityRegistry>()->Emplace<TransformComponent>(l_meshEntity, transformData);
g_Engine->Get<EntityRegistry>()->Emplace<VisibilityComponent>(l_meshEntity);
```

- [ ] **Step 5: Build + test**

```
cmake --build build --config RelWithDebInfo -j
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/ThirdParty/AssimpWrapper/
git commit -m "refactor: migrate AssimpWrapper asset pipeline to EntityRegistry"
```

---

## Task 10: Migrate JSON Serialization and Scene Services

**Files:**
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_POD.cpp`
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp`
- Modify: `Source/Engine/Services/SceneService.cpp`
- Modify: `Source/Engine/Services/TemplateAssetService.cpp`
- Modify: `Source/Engine/Services/AssetService.cpp`

- [ ] **Step 1: Read JSONSerializer_Components.cpp and SceneService.cpp**

- [ ] **Step 2: Update JSONSerializer_Components to handle new component types**

For each component type serialized:
- Remove fields that no longer exist: `m_UUID`, `m_Owner`, `m_ObjectStatus`, `m_InstanceName`
- Add new component types: `TransformComponent`, `VisibilityComponent`, etc.
- Serialize `EntityID` as `uint32_t`

- [ ] **Step 3: Update JSONWrapper entity lifecycle calls**

Replace `EntityManager::Spawn` / `ComponentManager::Spawn` with `EntityRegistry` equivalents.

- [ ] **Step 4: Update SceneService**

Same pattern — entity spawn/destroy uses `EntityRegistry`.

- [ ] **Step 5: Update TemplateAssetService and AssetService**

Same pattern.

- [ ] **Step 6: Build + test**

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/ThirdParty/JSONWrapper/ \
        Source/Engine/Services/SceneService.cpp \
        Source/Engine/Services/TemplateAssetService.cpp \
        Source/Engine/Services/AssetService.cpp
git commit -m "refactor: migrate JSON serialization and scene services to EntityRegistry"
```

---

## Task 11: Migrate Logic Clients

**Files:**
- Modify: `Source/DefaultClient/LogicClient/World.inl`
- Modify: `Source/DefaultClient/LogicClient/AnimationController.inl`
- Modify: `Source/DefaultClient/LogicClient/Player.inl`

- [ ] **Step 1: Read World.inl in full**

- [ ] **Step 2: Migrate World.inl entity and component spawning**

Old:
```cpp
auto l_entity = g_Engine->Get<EntityManager>()->Spawn(true, ObjectLifespan::Scene, "playerCharacter");
auto* l_model = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(l_entity, true, ObjectLifespan::Scene);
l_model->m_Transform.m_pos = Vec3(0.f, 0.f, 0.f);
```

New:
```cpp
auto l_entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Scene, "playerCharacter");
auto& l_transform = g_Engine->Get<EntityRegistry>()->Emplace<TransformComponent>(l_entity);
l_transform.m_LocalPos = Vec3(0.f, 0.f, 0.f);
```

Replace `#include "../../Engine/Services/ComponentManager.h"` / `EntityManager.h` with `EntityRegistry.h`.

- [ ] **Step 3: Migrate AnimationController.inl**

`m_modelComponent` pointer becomes `m_entity (EntityID)`. Change:
```cpp
// Old:
m_modelComponent = g_Engine->Get<ComponentManager>()->Find<ModelComponent>(*l_entity);
g_Engine->Get<AnimationService>()->PlayAnimation(m_modelComponent, path, loop);

// New:
m_entity = *l_entityID;   // EntityID
g_Engine->Get<AnimationService>()->PlayAnimation(m_entity, path, loop);
```

- [ ] **Step 4: Migrate Player.inl**

Read the file first. Apply same entity spawn + component emplace pattern.

- [ ] **Step 5: Build + test**

- [ ] **Step 6: Commit**

```bash
git add Source/DefaultClient/LogicClient/World.inl \
        Source/DefaultClient/LogicClient/AnimationController.inl \
        Source/DefaultClient/LogicClient/Player.inl
git commit -m "refactor: migrate logic clients to EntityRegistry"
```

---

## Task 12: Migrate Rendering Server and Remaining Callers

**Files:**
- `Source/Engine/RenderingServer/Common/IRenderingServer.cpp`
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp`
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp`
- `Source/Engine/RenderingServer/VK/VKRenderingServer.cpp` (+ 3 VK files)
- `Source/Engine/Services/BillboardDrawCallService.cpp`
- `Source/Engine/Services/PerFrameDataService.cpp`
- `Source/DefaultClient/RenderingClient/GIResolvePass.cpp`
- `Source/DefaultClient/RenderingClient/DebugPass.cpp`
- `Source/Engine/RayTracer/RayTracer.cpp`
- `Source/Tool/Baker/Baker.cpp` + `BrickGenerator.cpp`
- `Source/Editor/worldexplorer.cpp` + `modelcomponentpropertyeditor.cpp`

- [ ] **Step 1: Read IRenderingServer.cpp and PerFrameDataService.cpp — apply migration**

Every `ComponentManager` call becomes the matching `EntityRegistry` call. Every `EntityManager::Spawn/Find/Destroy` becomes the matching `EntityRegistry` call. Build after each file before moving on.

- [ ] **Step 2: Read DX12RenderingServer_ComponentPool.cpp — apply migration**

This file allocates GPU resources that are tied to component lifetime. Read it completely before editing. The component pool pattern (allocating a GPU resource per component at spawn, deallocating at destroy) must be re-expressed using `EntityRegistry::Emplace` / `EntityRegistry::Remove` lifecycle hooks or equivalent logic. Do not assume it follows the same pattern as service files.

- [ ] **Step 3: Read DX12RenderingServer_EngineComponent_Protected.cpp — apply migration**

Read it fully. Apply the substitution table. Build.

- [ ] **Step 4: Read VKRenderingServer_ComponentPool.cpp — apply migration**

Same caution as Step 2: GPU resource lifetime is tied to component lifecycle. Read before editing.

- [ ] **Step 5: Read remaining VK files (VKRenderingServer.cpp, VKRenderingServer_VulkanObject.cpp, VKRenderingServer_GraphicsDevice.cpp, VKRenderingServer_EngineComponent.cpp) — apply migration**

- [ ] **Step 6: Apply to BillboardDrawCallService, PerFrameDataService, GIResolvePass, DebugPass, RayTracer, Baker, BrickGenerator**

Read each file before editing.

- [ ] **Step 7: Read Editor files and apply migration**

`worldexplorer.cpp` and `modelcomponentpropertyeditor.cpp` display `ModelComponent` fields in UI. After `ModelComponent` is gone, these must operate on the new component types (`TransformComponent`, `VisibilityComponent`, etc.). Read both files completely before deciding how to restructure the UI layer.

- [ ] **Step 8: Full build**

```
cmake --build build --config RelWithDebInfo -j
```

Fix all remaining errors. There should be no `// TODO Phase2-migrate` comments left at this point.

- [ ] **Step 9: Run tests + GPU gate**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Both must pass.

- [ ] **Step 10: Commit**

```bash
git add Source/Engine/RenderingServer/ \
        Source/Engine/Services/BillboardDrawCallService.cpp \
        Source/Engine/Services/PerFrameDataService.cpp \
        Source/DefaultClient/RenderingClient/GIResolvePass.cpp \
        Source/DefaultClient/RenderingClient/DebugPass.cpp \
        Source/Engine/RayTracer/RayTracer.cpp \
        Source/Tool/ \
        Source/Editor/
git commit -m "refactor: migrate rendering server and tool callers to EntityRegistry"
```

---

## Task 13: Delete ModelComponent and DrawCallComponent

**Files:**
- Delete: `Source/Engine/Component/ModelComponent.h`

- [ ] **Step 1: Verify zero remaining references**

```
grep -rn "ModelComponent\|DrawCallComponent" Source/ --include="*.cpp" --include="*.h" --include="*.inl" | grep -v "^Binary"
```

Expected: 0 results. If any remain, migrate them before proceeding.

- [ ] **Step 2: Delete the file**

```bash
git rm Source/Engine/Component/ModelComponent.h
```

- [ ] **Step 3: Build to confirm no broken includes**

```
cmake --build build --config RelWithDebInfo -j
```

- [ ] **Step 4: Commit**

```bash
git commit -m "refactor: delete ModelComponent and DrawCallComponent"
```

---

## Task 14: Delete ComponentManager, EntityManager, and Old Entity Type

**Files:**
- Modify: `Source/Engine/Engine.cpp` — remove registration + Terminate calls
- Modify: `Source/Engine/Engine.h` — remove includes if present
- Delete: `Source/Engine/Services/ComponentManager.h` + `ComponentManager.cpp`
- Delete: `Source/Engine/Services/EntityManager.h` + `EntityManager.cpp`
- Delete: `Source/Engine/Common/Entity.h` (if unreferenced)

- [ ] **Step 1: Verify zero remaining includes**

```
grep -rn "ComponentManager.h\|EntityManager.h" Source/ --include="*.cpp" --include="*.h" --include="*.inl"
```

Expected: only `EntityManager.cpp` includes `EntityManager.h` and `ComponentManager.cpp` includes `ComponentManager.h`.

- [ ] **Step 2: Read Engine.cpp — find registration and Terminate calls**

Search for lines that call `Get<ComponentManager>()` or `Get<EntityManager>()`. Remove Setup, Initialize, Update, and Terminate calls for both, plus any `new ComponentManager()` / `new EntityManager()` allocations.

- [ ] **Step 3: Delete the four files**

```bash
git rm Source/Engine/Services/ComponentManager.h \
       Source/Engine/Services/ComponentManager.cpp \
       Source/Engine/Services/EntityManager.h \
       Source/Engine/Services/EntityManager.cpp
```

- [ ] **Step 4: Check Entity.h**

```
grep -rn "Entity.h" Source/ --include="*.cpp" --include="*.h" --include="*.inl"
```

If the only remaining include is in files being deleted, delete it too:
```bash
git rm Source/Engine/Common/Entity.h
```

- [ ] **Step 5: Full build**

```
cmake --build build --config RelWithDebInfo -j
```

Expected: clean build with no references to the deleted systems.

- [ ] **Step 6: Run tests + GPU gate**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Both must exit 0.

- [ ] **Step 7: Final commit**

```bash
git add Source/Engine/Engine.cpp Source/Engine/Engine.h
git commit -m "feat: delete ComponentManager and EntityManager — ECS Phase 2 complete"
```
