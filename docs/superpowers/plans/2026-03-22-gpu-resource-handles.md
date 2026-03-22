# GPU Resource Handle Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Strip Component/Object inheritance from all 8 GPU resource handle types, move pool ownership into IRenderingServer, and delete ComponentManager.

**Architecture:** GPUResourceComponent loses its Component base and becomes a plain struct (retaining ObjectStatus, InstanceName, and GPU binding fields). GPUResourceComponent.h is NOT deleted — it remains as the polymorphic base for `IRenderingServer::BindGPUResource(GPUResourceComponent*)`. RenderPass, ShaderProgram, and CommandList structs similarly lose their Component base. IRenderingServer::Setup() calls IRenderingServer::InitializePool() which now allocates TObjectPool<T> directly per type, replacing the 8 ComponentManager::RegisterType calls. All consumers updated to use IRenderingServer::AddXComponent() and FindXByName() instead of ComponentManager.

**Tech Stack:** C++17, MSVC, TObjectPool from `Source/Engine/Common/ObjectPool.h`, ThreadSafeUnorderedMap from `Source/Engine/Common/ThreadSafeUnorderedMap.h`, ThreadSafeVector from `Source/Engine/Common/ThreadSafeVector.h`.

---

## File Map

**Modified component headers:**
- `Source/Engine/Component/GPUResourceComponent.h` — strip `Component` base, add `ObjectStatus` + `ObjectName` inline fields
- `Source/Engine/Component/RenderPassComponent.h` — strip `Component` base, add `ObjectStatus` + `ObjectName` inline
- `Source/Engine/Component/ShaderProgramComponent.h` — same
- `Source/Engine/Component/CommandListComponent.h` — same
- `Source/Engine/Component/MaterialComponent.h` — `m_TextureComponents: vector<uint64_t>` → `vector<string>`

**Modified rendering server:**
- `Source/Engine/RenderingServer/IRenderingServer.h` — add pool + LUT + pointer-list fields for 8 types
- `Source/Engine/RenderingServer/Common/IRenderingServer.cpp` — rewrite `InitializePool()` + `TerminatePool()`, replace `AddComponent<T>` free func with pool-backed implementation; add `FindTextureByName()` helper

**Modified DX12 rendering server (UUID key migration):**
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer.h` — update stale `// Key: Component m_UUID` comment to `// Key: Component pointer (as uint64_t)`
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp` — `Delete(TextureComponent*)`: replace `texture->m_UUID` key with `reinterpret_cast<uint64_t>(texture)` (same pattern as mesh at line 60)
- `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp` — `Initialize(TextureComponent*, ...)`: replace `texture->m_UUID` at lines 265 and 293 with same pointer-cast key

**Modified consumers:**
- `Source/Engine/Services/TemplateAssetService.cpp` — Spawn<T> → AddXComponent; FindByUUID → FindXByName via IRenderingServer
- `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` — FindByUUID<TextureComponent> → FindTextureByName; read/write path strings
- `Source/Editor/worldexplorer.cpp` — Spawn<TextureComponent> → AddTextureComponent; Destroy → Delete
- `Source/Engine/Services/SceneService.cpp` — remove ComponentManager include and CleanUp call
- `Source/Engine/Services/DrawCallService.cpp` — remove the dead commented-out `FindByUUID<TextureComponent>` line (line 195) and any ComponentManager include

**Deleted:**
- `Source/Engine/Services/ComponentManager.h` — deleted after all usages removed
- `Source/Engine/Component/GPUResourceComponent.h` — NOT deleted; stripped of Component base (still used by Texture/GPUBuffer/Sampler for BindGPUResource polymorphism)

**Engine registration:**
- `Source/Engine/Engine.cpp` — remove `ComponentManager` `Get<>` service and related includes

---

## Task 1: Strip Component base from GPUResourceComponent

**Files:**
- Modify: `Source/Engine/Component/GPUResourceComponent.h`
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp`
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp`

GPUResourceComponent currently inherits `Component` → `Object`. After this task it is a standalone struct.
Fields dropped: `m_UUID`, `m_Serializable`, `m_ObjectLifespan`, `m_Owner` (these come from Object/Component).
Fields added inline: `ObjectStatus m_ObjectStatus` and `ObjectName m_InstanceName` (these were on Object; the rendering code checks them directly).

- [ ] **Step 1: Read GPUResourceComponent.h in full**

```
Source/Engine/Component/GPUResourceComponent.h
```

Confirm current content: `class GPUResourceComponent : public Component { ... }` with fields:
`m_GPUResourceType`, `m_CPUAccessibility`, `m_GPUAccessibility`, `m_ReadState`, `m_WriteState`, `m_ReadHandles`, `m_WriteHandles`.

- [ ] **Step 2: Read Object.h and Component.h to confirm which types you need to include for ObjectStatus / ObjectName**

```
Source/Engine/Common/Object.h
```

`ObjectStatus` and `ObjectName` (a typedef over `FixedSizeString<128>`) are defined in `Object.h`.

- [ ] **Step 3: Rewrite GPUResourceComponent.h**

Replace the class with a struct, keep the include for `GraphicsPrimitive.h` (defines `GPUResourceType`, `Accessibility`, `DescriptorHandle`), keep `Object.h` for `ObjectStatus`/`ObjectName` type definitions (do NOT inherit), add the two new inline fields:

```cpp
#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"

namespace Inno
{
    struct GPUResourceComponent
    {
        GPUResourceType   m_GPUResourceType  = GPUResourceType::Sampler;
        Accessibility     m_CPUAccessibility = Accessibility::WriteOnly;
        Accessibility     m_GPUAccessibility = Accessibility::ReadOnly;
        uint32_t          m_ReadState        = 0;
        uint32_t          m_WriteState       = 0;
        std::vector<DescriptorHandle> m_ReadHandles;
        std::vector<DescriptorHandle> m_WriteHandles;
        ObjectStatus      m_ObjectStatus     = ObjectStatus::Invalid;
        ObjectName        m_InstanceName     = "";
    };
}
```

- [ ] **Step 4: Migrate DX12 texture buffer map keys from m_UUID to pointer**

Stripping `m_UUID` from `GPUResourceComponent` breaks `DX12RenderingServer`'s texture buffer maps, which are still keyed by `texture->m_UUID`. Migrate them to use `reinterpret_cast<uint64_t>(texture)` as the key — matching the existing mesh pattern at `DX12RenderingServer_ComponentPool.cpp:60` (marked `TODO Phase2-migrate`).

**In `DX12RenderingServer_ComponentPool.cpp` — `Delete(TextureComponent*)`:**
```cpp
// Before:
auto componentUUID = texture->m_UUID;

// After:
// TODO Phase2-migrate: switched from m_UUID to pointer key (m_UUID removed from GPUResourceComponent)
auto componentUUID = reinterpret_cast<uint64_t>(texture);
```

**In `DX12RenderingServer_EngineComponent_Protected.cpp` — `Initialize(TextureComponent*, ...)` lines 265 and 293:**
```cpp
// Before (line 265):
m_TextureBuffers_Default[texture->m_UUID] = defaultHeapBuffer;

// After:
m_TextureBuffers_Default[reinterpret_cast<uint64_t>(texture)] = defaultHeapBuffer;

// Before (line 293):
m_TextureBuffers_Upload[texture->m_UUID] = l_uploadHeapBuffer;

// After:
m_TextureBuffers_Upload[reinterpret_cast<uint64_t>(texture)] = l_uploadHeapBuffer;
```

Both `m_TextureBuffers_Default` and `m_TextureBuffers_Upload` are `std::unordered_map<uint64_t, ComPtr<ID3D12Resource>>` — the pointer cast is a safe, stable key since the texture's pool slot address does not change between Initialize and Delete.

Also update the stale comment at `DX12RenderingServer.h:231`:
```cpp
// Before:
// Key: Component m_UUID, Value: DX12 GPU resources

// After:
// Key: Component pointer (as uint64_t), Value: DX12 GPU resources
```

Add `Source/Engine/RenderingServer/DX12/DX12RenderingServer.h` to the git add in Step 6.

- [ ] **Step 5: Build to verify TextureComponent, GPUBufferComponent, SamplerComponent still compile**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: build succeeds. These three types inherit GPUResourceComponent and pick up the new fields automatically.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Component/GPUResourceComponent.h
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer_EngineComponent_Protected.cpp
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer.h
git commit -m "refactor: strip Component base from GPUResourceComponent, inline ObjectStatus/InstanceName; migrate DX12 texture buffer map keys from m_UUID to pointer"
```

---

## Task 2: Strip Component base from RenderPass, ShaderProgram, CommandList

**Files:**
- Modify: `Source/Engine/Component/RenderPassComponent.h`
- Modify: `Source/Engine/Component/ShaderProgramComponent.h`
- Modify: `Source/Engine/Component/CommandListComponent.h`

These three types inherit `Component` directly (not via GPUResourceComponent). They don't participate in the `BindGPUResource(GPUResourceComponent*)` polymorphism, so they can safely become standalone structs.

The rendering server checks `m_ObjectStatus` on these after initialize, and logs use `m_InstanceName`. Both fields move inline.

- [ ] **Step 1: Update RenderPassComponent.h**

Change `class RenderPassComponent : public Component` to `struct RenderPassComponent`. Add `ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;` and `ObjectName m_InstanceName = "";` as the first two fields. Keep `#include "../Common/Object.h"` for the type definitions; remove it only if the types are available from GraphicsPrimitive.h (they are not — keep it).

Remove the `static uint32_t GetTypeID()` and `static const char* GetTypeName()` methods — these were only needed by ComponentManager's template machinery and are no longer required.

```cpp
#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
    struct RenderPassComponent
    {
        ObjectStatus    m_ObjectStatus  = ObjectStatus::Invalid;
        ObjectName      m_InstanceName  = "";

        ShaderProgramComponent* m_ShaderProgram = nullptr;

        RenderPassDesc m_RenderPassDesc = {};
        std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

        size_t m_CurrentFrame = 0;

        std::function<void()> m_OnResize;
        std::function<void(CommandListComponent*)> m_CustomCommandsFunc;

        IOutputMergerTarget* m_OutputMergerTarget   = nullptr;
        IPipelineStateObject* m_PipelineStateObject = nullptr;
        std::vector<ISemaphore*> m_Semaphores;
    };
}
```

- [ ] **Step 2: Update ShaderProgramComponent.h**

Same pattern: `struct ShaderProgramComponent`, remove `Component` base, add `m_ObjectStatus` + `m_InstanceName` as first two fields. Remove `GetTypeID()` and `GetTypeName()`.

```cpp
#pragma once
#include "../Common/Object.h"
#include <vector>

namespace Inno
{
    using ShaderFilePath = FixedSizeString<128>;

    struct ShaderFilePaths
    {
        ShaderFilePath m_VSPath = "";
        ShaderFilePath m_HSPath = "";
        ShaderFilePath m_DSPath = "";
        ShaderFilePath m_GSPath = "";
        ShaderFilePath m_PSPath = "";
        ShaderFilePath m_CSPath = "";
        ShaderFilePath m_RayGenPath     = "";
        ShaderFilePath m_AnyHitPath     = "";
        ShaderFilePath m_ClosestHitPath = "";
        ShaderFilePath m_MissPath       = "";
    };

    struct ShaderProgramComponent
    {
        ObjectStatus     m_ObjectStatus     = ObjectStatus::Invalid;
        ObjectName       m_InstanceName     = "";

        ShaderFilePaths  m_ShaderFilePaths  = {};

        std::vector<uint8_t> m_VSBuffer;
        std::vector<uint8_t> m_HSBuffer;
        std::vector<uint8_t> m_DSBuffer;
        std::vector<uint8_t> m_GSBuffer;
        std::vector<uint8_t> m_PSBuffer;
        std::vector<uint8_t> m_CSBuffer;
        std::vector<uint8_t> m_RayGenBuffer;
        std::vector<uint8_t> m_AnyHitBuffer;
        std::vector<uint8_t> m_ClosestHitBuffer;
        std::vector<uint8_t> m_MissBuffer;
    };
}
```

- [ ] **Step 3: Update CommandListComponent.h**

```cpp
#pragma once
#include "../Common/Object.h"
#include "../Common/GraphicsPrimitive.h"

namespace Inno
{
    struct CommandListComponent
    {
        ObjectStatus   m_ObjectStatus = ObjectStatus::Invalid;
        ObjectName     m_InstanceName = "";

        uint64_t       m_CommandList  = 0;
        GPUEngineType  m_Type         = GPUEngineType::Graphics;
    };
}
```

- [ ] **Step 4: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: build succeeds. If DX12 concrete types (e.g. `DX12RenderPassComponent`) inherit `RenderPassComponent`, they will compile fine since the struct API is unchanged.

- [ ] **Step 5: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: all tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Component/RenderPassComponent.h
git add Source/Engine/Component/ShaderProgramComponent.h
git add Source/Engine/Component/CommandListComponent.h
git commit -m "refactor: strip Component base from RenderPass/ShaderProgram/CommandList structs"
```

---

## Task 3: Move pool ownership into IRenderingServer

**Files:**
- Modify: `Source/Engine/RenderingServer/IRenderingServer.h`
- Modify: `Source/Engine/RenderingServer/Common/IRenderingServer.cpp`
- Modify: `Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp` — add `ReleaseFromPool` calls to all 8 Delete overrides (Step 6)

This is the core mechanical change. `InitializePool()` creates 8 `TObjectPool<T>*` directly. The `AddComponent<T>` free function is replaced by direct pool + LUT management. `TerminatePool()` destroys all pools.

### IRenderingServer.h changes

Add private pool fields after the `m_needResize` member (end of the private section):

```cpp
// GPU resource handle pools (owned by IRenderingServer)
struct GPUHandlePools
{
    TObjectPool<MeshComponent>*          Meshes          = nullptr;
    TObjectPool<TextureComponent>*       Textures        = nullptr;
    TObjectPool<MaterialComponent>*      Materials       = nullptr;
    TObjectPool<RenderPassComponent>*    RenderPasses    = nullptr;
    TObjectPool<ShaderProgramComponent>* ShaderPrograms  = nullptr;
    TObjectPool<SamplerComponent>*       Samplers        = nullptr;
    TObjectPool<GPUBufferComponent>*     GPUBuffers      = nullptr;
    TObjectPool<CommandListComponent>*   CommandLists    = nullptr;

    ThreadSafeUnorderedMap<std::string, MeshComponent*>          MeshLUT;
    ThreadSafeUnorderedMap<std::string, TextureComponent*>       TextureLUT;
    ThreadSafeUnorderedMap<std::string, MaterialComponent*>      MaterialLUT;
    ThreadSafeUnorderedMap<std::string, RenderPassComponent*>    RenderPassLUT;
    ThreadSafeUnorderedMap<std::string, ShaderProgramComponent*> ShaderProgramLUT;
    ThreadSafeUnorderedMap<std::string, SamplerComponent*>       SamplerLUT;
    ThreadSafeUnorderedMap<std::string, GPUBufferComponent*>     GPUBufferLUT;
    ThreadSafeUnorderedMap<std::string, CommandListComponent*>   CommandListLUT;

    ThreadSafeVector<MeshComponent*>          MeshPointers;
    ThreadSafeVector<TextureComponent*>       TexturePointers;
    ThreadSafeVector<MaterialComponent*>      MaterialPointers;
    ThreadSafeVector<RenderPassComponent*>    RenderPassPointers;
    ThreadSafeVector<ShaderProgramComponent*> ShaderProgramPointers;
    ThreadSafeVector<SamplerComponent*>       SamplerPointers;
    ThreadSafeVector<GPUBufferComponent*>     GPUBufferPointers;
    ThreadSafeVector<CommandListComponent*>   CommandListPointers;
};
GPUHandlePools m_GPUHandlePools;
```

Also add to `IRenderingServer.h` public section the new lookup API needed by TemplateAssetService and JSONSerializer:

```cpp
TextureComponent*  FindTextureByName(const char* name);
MeshComponent*     FindMeshByName(const char* name);
MaterialComponent* FindMaterialByName(const char* name);
```

### IRenderingServer.cpp changes

- [ ] **Step 1: Read IRenderingServer.cpp lines 1–60 (InitializePool, TerminatePool, Setup)**

Confirms the `TODO Phase2-migrate: Task 14` comment and the 8 `RegisterType<T>` calls.

- [ ] **Step 2: Rewrite InitializePool()**

Remove the 8 `RegisterType<T>` calls and the ComponentManager include. Add pool creation:

```cpp
bool IRenderingServer::InitializePool()
{
    auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

    m_GPUHandlePools.Meshes         = TObjectPool<MeshComponent>::Create(l_cap.maxMeshes);
    m_GPUHandlePools.Textures       = TObjectPool<TextureComponent>::Create(l_cap.maxTextures);
    m_GPUHandlePools.Materials      = TObjectPool<MaterialComponent>::Create(l_cap.maxMaterials);
    m_GPUHandlePools.RenderPasses   = TObjectPool<RenderPassComponent>::Create(128);
    m_GPUHandlePools.ShaderPrograms = TObjectPool<ShaderProgramComponent>::Create(256);
    m_GPUHandlePools.Samplers       = TObjectPool<SamplerComponent>::Create(256);
    m_GPUHandlePools.GPUBuffers     = TObjectPool<GPUBufferComponent>::Create(l_cap.maxBuffers);
    m_GPUHandlePools.CommandLists   = TObjectPool<CommandListComponent>::Create(256);

    return true;
}
```

- [ ] **Step 3: Rewrite TerminatePool()**

Add pool destruction. The correct static API is `TObjectPool<T>::Destruct(pool)` (confirmed at `ObjectPool.h:152`). `Destruct` frees the heap allocation made by `TObjectPool<T>::Create()`. Do NOT use `Destroy` — that is the instance method for returning individual elements to the pool, not for tearing down the pool itself.

```cpp
bool IRenderingServer::TerminatePool()
{
    TObjectPool<MeshComponent>::Destruct(m_GPUHandlePools.Meshes);
    TObjectPool<TextureComponent>::Destruct(m_GPUHandlePools.Textures);
    TObjectPool<MaterialComponent>::Destruct(m_GPUHandlePools.Materials);
    TObjectPool<RenderPassComponent>::Destruct(m_GPUHandlePools.RenderPasses);
    TObjectPool<ShaderProgramComponent>::Destruct(m_GPUHandlePools.ShaderPrograms);
    TObjectPool<SamplerComponent>::Destruct(m_GPUHandlePools.Samplers);
    TObjectPool<GPUBufferComponent>::Destruct(m_GPUHandlePools.GPUBuffers);
    TObjectPool<CommandListComponent>::Destruct(m_GPUHandlePools.CommandLists);
    return true;
}

- [ ] **Step 4: Replace the AddComponent<T> free function with a pool-backed helper**

The `AddComponent<T>` free function (lines ~250–280) currently calls `EntityRegistry::Spawn()` then `ComponentManager::Spawn<T>()`. Replace with a template function that uses the correct pool + LUT from `m_GPUHandlePools`:

```cpp
template <typename T>
static T* AllocateGPUHandle(TObjectPool<T>* pool,
                             ThreadSafeUnorderedMap<std::string, T*>& lut,
                             ThreadSafeVector<T*>& pointers,
                             const char* name)
{
    if (!name || name[0] == '\0')
    {
        Log(Error, "GPU handle name cannot be empty.");
        return nullptr;
    }

    auto l_existing = lut.find(name);
    if (l_existing != lut.end())
        return l_existing->second;

    auto l_ptr = pool->Spawn();
    if (!l_ptr)
    {
        Log(Error, "GPU handle pool exhausted for name: ", name);
        return nullptr;
    }

    l_ptr->m_ObjectStatus = ObjectStatus::Created;
    l_ptr->m_InstanceName = ObjectName(name);

    lut.emplace(name, l_ptr);
    pointers.emplace_back(l_ptr);
    return l_ptr;
}
```

Then replace all 8 `AddXComponent` implementations (replace each existing one-liner that delegated to `AddComponent<T>`):

```cpp
MeshComponent* IRenderingServer::AddMeshComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.Meshes,
                             m_GPUHandlePools.MeshLUT,
                             m_GPUHandlePools.MeshPointers, name);
}
TextureComponent* IRenderingServer::AddTextureComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.Textures,
                             m_GPUHandlePools.TextureLUT,
                             m_GPUHandlePools.TexturePointers, name);
}
MaterialComponent* IRenderingServer::AddMaterialComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.Materials,
                             m_GPUHandlePools.MaterialLUT,
                             m_GPUHandlePools.MaterialPointers, name);
}
RenderPassComponent* IRenderingServer::AddRenderPassComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.RenderPasses,
                             m_GPUHandlePools.RenderPassLUT,
                             m_GPUHandlePools.RenderPassPointers, name);
}
ShaderProgramComponent* IRenderingServer::AddShaderProgramComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.ShaderPrograms,
                             m_GPUHandlePools.ShaderProgramLUT,
                             m_GPUHandlePools.ShaderProgramPointers, name);
}
SamplerComponent* IRenderingServer::AddSamplerComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.Samplers,
                             m_GPUHandlePools.SamplerLUT,
                             m_GPUHandlePools.SamplerPointers, name);
}
GPUBufferComponent* IRenderingServer::AddGPUBufferComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.GPUBuffers,
                             m_GPUHandlePools.GPUBufferLUT,
                             m_GPUHandlePools.GPUBufferPointers, name);
}
CommandListComponent* IRenderingServer::AddCommandListComponent(const char* name)
{
    return AllocateGPUHandle(m_GPUHandlePools.CommandLists,
                             m_GPUHandlePools.CommandListLUT,
                             m_GPUHandlePools.CommandListPointers, name);
}
```

- [ ] **Step 5: Implement the Find* methods**

```cpp
TextureComponent* IRenderingServer::FindTextureByName(const char* name)
{
    auto l_result = m_GPUHandlePools.TextureLUT.find(name);
    return (l_result != m_GPUHandlePools.TextureLUT.end()) ? l_result->second : nullptr;
}
MeshComponent* IRenderingServer::FindMeshByName(const char* name)
{
    auto l_result = m_GPUHandlePools.MeshLUT.find(name);
    return (l_result != m_GPUHandlePools.MeshLUT.end()) ? l_result->second : nullptr;
}
MaterialComponent* IRenderingServer::FindMaterialByName(const char* name)
{
    auto l_result = m_GPUHandlePools.MaterialLUT.find(name);
    return (l_result != m_GPUHandlePools.MaterialLUT.end()) ? l_result->second : nullptr;
}
```

- [ ] **Step 6: Update Delete() to clean up the LUT and pointer list**

**Wiring pattern:** IRenderingServer's `Delete(T*)` methods are currently pure virtual (`= 0`). The DX12 override handles GPU resource teardown. Add pool/LUT cleanup by making the base class `Delete(T*)` non-pure with a default that calls a new virtual `DeleteImpl(T*)`:

```
// IRenderingServer.h: Change "= 0" to non-pure with pool cleanup
// virtual bool Delete(MeshComponent* mesh) = 0;   ← old
//
// Pattern: base class calls ReleaseFromPool, then calls virtual DeleteImpl
```

Concrete approach: in IRenderingServer.h, change `Delete(T*)` overloads from `= 0` to non-virtual wrappers that call (a) `virtual bool DeleteImpl(T*)` (DX12 overrides this for GPU teardown) then (b) `ReleaseFromPool(T*)` (pool cleanup).

Read `Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp` to confirm the current DX12 Delete override structure before writing the wrapper. If changing from pure virtual to non-virtual + virtual DeleteImpl is too invasive for this task's scope, alternatively: add a `protected` non-virtual `ReleaseFromPool<T>` helper that each DX12 Delete override calls explicitly at the END of its function body, after GPU teardown.

**The simpler approach (preferred):** keep `Delete(T*)` virtual as-is in the DX12 overrides, and add a `static` file-scope helper in `IRenderingServer.cpp` (all data is passed by parameter — no member access needed):

**Critical ordering:** the three cleanup operations MUST be in the order below. `pool->Destroy(ptr)` calls `ptr->~T()` then zeroes the slot memory — after that, `ptr->m_InstanceName` is garbage. The LUT erase reads `ptr->m_InstanceName.c_str()` as its key, so it must execute **before** `pool->Destroy`.

```cpp
template <typename T>
static void ReleaseFromPool(TObjectPool<T>* pool,
                             ThreadSafeUnorderedMap<std::string, T*>& lut,
                             ThreadSafeVector<T*>& pointers,
                             T* ptr)
{
    if (!ptr) return;
    lut.erase(std::string(ptr->m_InstanceName.c_str()));  // MUST be first: reads ptr->m_InstanceName before it is zeroed
    pointers.eraseByValue(ptr);                           // ThreadSafeVector::eraseByValue confirmed at ThreadSafeVector.h:116
    pool->Destroy(ptr);                                   // MUST be last: destructs the object and zeroes the pool slot
}
```

Then in each DX12 Delete override in `DX12RenderingServer_ComponentPool.cpp`, add a call to `ReleaseFromPool` **at the very end, after all GPU teardown, just before `return true`**. Do NOT insert it before any error-path `return false`.

Concrete before/after for `Delete(MeshComponent*)` (current form at line 57):

```cpp
// BEFORE (current):
bool DX12RenderingServer::Delete(MeshComponent* mesh)
{
    auto componentUUID = reinterpret_cast<uint64_t>(mesh);
    // ... ComPtr cleanup for vertex/index/BLAS/scratch buffers ...
    return true;
}

// AFTER (add the ReleaseFromPool call immediately before return true):
bool DX12RenderingServer::Delete(MeshComponent* mesh)
{
    auto componentUUID = reinterpret_cast<uint64_t>(mesh);
    // ... ComPtr cleanup for vertex/index/BLAS/scratch buffers (unchanged) ...
    ReleaseFromPool(m_GPUHandlePools.Meshes,
                    m_GPUHandlePools.MeshLUT,
                    m_GPUHandlePools.MeshPointers, mesh);
    return true;
}
```

Apply the same pattern to all 8 overrides. The pool/LUT names per type:
| Type | Pool field | LUT field | Pointer list field |
|------|------------|-----------|-------------------|
| MeshComponent | `Meshes` | `MeshLUT` | `MeshPointers` |
| TextureComponent | `Textures` | `TextureLUT` | `TexturePointers` |
| MaterialComponent | `Materials` | `MaterialLUT` | `MaterialPointers` |
| RenderPassComponent | `RenderPasses` | `RenderPassLUT` | `RenderPassPointers` |
| ShaderProgramComponent | `ShaderPrograms` | `ShaderProgramLUT` | `ShaderProgramPointers` |
| SamplerComponent | `Samplers` | `SamplerLUT` | `SamplerPointers` |
| GPUBufferComponent | `GPUBuffers` | `GPUBufferLUT` | `GPUBufferPointers` |
| CommandListComponent | `CommandLists` | `CommandListLUT` | `CommandListPointers` |

(Verify the exact field names match what you declared in `GPUHandlePools` in Step 2.)

> **Advisory:** `SceneService::AddComponentToSceneHierarchyMap<T>()` calls `ComponentManager::GetAll<T>()` but only via commented-out `TODO Phase2-migrate` calls — no live GetAll callers exist for GPU resource types. No additional action needed.

- [ ] **Step 7: Remove the ComponentManager include and all remaining ComponentManager references from IRenderingServer.cpp**

The `#include "../../Services/ComponentManager.h"` at line 16 and any remaining `g_Engine->Get<ComponentManager>()` calls should be gone after steps 2–6.

- [ ] **Step 8: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: succeeds. DX12RenderingServer's virtual Delete overrides still compile because they receive the same `T*` pointers.

- [ ] **Step 9: Commit**

```bash
git add Source/Engine/RenderingServer/IRenderingServer.h
git add Source/Engine/RenderingServer/Common/IRenderingServer.cpp
git add Source/Engine/RenderingServer/DX12/DX12RenderingServer_ComponentPool.cpp
git commit -m "refactor: move GPU handle pool ownership from ComponentManager into IRenderingServer"
```

---

## Task 4: Update TemplateAssetService

**Files:**
- Modify: `Source/Engine/Services/TemplateAssetService.cpp`

TemplateAssetService currently calls `ComponentManager::Spawn<T>` and `FindByUUID<T>` to allocate and look up GPU resource components. After this task, it uses `IRenderingServer::AddXComponent(name)` and `IRenderingServer::FindXByName(name)`.

- [ ] **Step 1: Read TemplateAssetService.cpp lines 1–160**

Confirm the pattern: each asset type (Texture, Material, Mesh) is loaded by:
1. Checking if it's already loaded (`FindByUUID`)
2. If not, spawning a new component via ComponentManager
3. Loading the asset into the component

- [ ] **Step 2: Replace Texture loading pattern**

Before:
```cpp
auto entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Persistence, (std::string(name) + "/").c_str());
texturePtr = componentManager->FindByUUID<TextureComponent>(loadedTexture);
// ...
texturePtr = componentManager->Spawn<TextureComponent>(entity, true, ObjectLifespan::Persistence);
```

After:
```cpp
texturePtr = g_Engine->getRenderingServer()->FindTextureByName(name);
if (!texturePtr)
    texturePtr = g_Engine->getRenderingServer()->AddTextureComponent(name);
```

The EntityRegistry::Spawn call is no longer needed — GPU handles aren't entities.

- [ ] **Step 3: Apply the same replacement to Material and Mesh loading**

Same pattern: `FindMaterialByName` / `AddMaterialComponent`, `FindMeshByName` / `AddMeshComponent`.

- [ ] **Step 4: Remove the ComponentManager include and the `auto componentManager = ...` lines**

- [ ] **Step 5: Build and run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/TemplateAssetService.cpp
git commit -m "refactor: replace ComponentManager Spawn/Find with IRenderingServer pool API in TemplateAssetService"
```

---

## Task 5: Update MaterialComponent and JSONSerializer for path-based texture refs

**Files:**
- Modify: `Source/Engine/Component/MaterialComponent.h`
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`

`MaterialComponent::m_TextureComponents` stores `vector<uint64_t>` UUID keys. The serialization write path (`to_json`) loops over these UUIDs, looks up each via `ComponentManager::FindByUUID<TextureComponent>`, and writes `m_InstanceName.c_str()` as a `"Name"` string. The deserialization path (`Load(MaterialComponent)`) is currently a stub (`TODO Phase2-migrate`) that only calls `reserve` — it never populates the field. After this task, the field becomes `vector<string>` and both paths operate on names directly with no ComponentManager dependency.

- [ ] **Step 1: Change MaterialComponent.h**

```cpp
// Before:
std::vector<uint64_t> m_TextureComponents;

// After:
std::vector<std::string> m_TextureComponents;
```

Note: MaterialComponent already includes `TextureComponent.h`. No other change needed in the header.

- [ ] **Step 2: Read JSONSerializer_Components.cpp lines 60–175**

Two distinct code paths handle `m_TextureComponents`:
- **Serialization** (`to_json`, ~lines 75–86): loops over UUID elements, calls `ComponentManager::FindByUUID<TextureComponent>`, writes `m_InstanceName.c_str()` as `"Name"`.
- **Deserialization** (`Load(MaterialComponent)`, ~lines 155–160): a stub with `TODO Phase2-migrate` comment — it calls `reserve` but never populates `m_TextureComponents`. There is no live `FindByUUID` call in the deserialization path.

- [ ] **Step 3: Implement the deserialization stub (lines 155–160)**

The stub currently reads the `"TextureComponents"` JSON array but does nothing with it. After `m_TextureComponents` becomes `vector<string>`, implement the loop body to populate it:

Before:
```cpp
// TODO Phase2-migrate: TextureComponent still inherits Component, restore loading when migrated
if (j.find("TextureComponents") != j.end())
{
    auto l_j = j["TextureComponents"];
    component.m_TextureComponents.reserve(l_j.size());
}
```

After:
```cpp
if (j.find("TextureComponents") != j.end())
{
    auto l_j = j["TextureComponents"];
    component.m_TextureComponents.reserve(l_j.size());
    for (const auto& l_entry : l_j)
    {
        component.m_TextureComponents.push_back(l_entry["Name"].get<std::string>());
    }
}
```

Remove the `TODO Phase2-migrate` comment.

- [ ] **Step 4: Update the serialization write path (to_json, lines 75–86)**

The current write path uses UUID elements to look up the component. After `m_TextureComponents` becomes `vector<string>`, the names are already stored — no lookup needed. Replace the entire loop:

Before:
```cpp
json textureComponents = json::array();
for (auto textureComponentID : component.m_TextureComponents)
{
    auto textureComponent = g_Engine->Get<ComponentManager>()->FindByUUID<TextureComponent>(textureComponentID);
    if (textureComponent)
    {
        json textureJson;
        textureJson["Name"] = textureComponent->m_InstanceName.c_str();
        textureComponents.push_back(textureJson);
    }
}
j["TextureComponents"] = textureComponents;
```

After:
```cpp
json textureComponents = json::array();
for (const auto& textureName : component.m_TextureComponents)
{
    json textureJson;
    textureJson["Name"] = textureName;
    textureComponents.push_back(textureJson);
}
j["TextureComponents"] = textureComponents;
```

- [ ] **Step 5: Remove the ComponentManager include from JSONSerializer_Components.cpp**

- [ ] **Step 6: Build and run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Component/MaterialComponent.h
git add Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp
git commit -m "refactor: MaterialComponent.m_TextureComponents switches from UUID to asset name strings"
```

---

## Task 6: Update worldexplorer and SceneService

**Files:**
- Modify: `Source/Editor/worldexplorer.cpp`
- Modify: `Source/Engine/Services/SceneService.cpp`

- [ ] **Step 1: Read worldexplorer.cpp lines 200–280**

There are two ComponentManager usages to understand:

1. `addComponent<T>()` (line 200–228): Generic template tagged `TODO Phase2-migrate: Task 14 — migrate to EntityRegistry::Emplace<T>`. This migration is for ECS gameplay types, not GPU resources — **leave this function unchanged**. It is not called for TextureComponent from any live code path.

2. `destroyComponent()` (line 267–271): The TextureComponent branch is a **live, uncommented** call:
   ```cpp
   g_Engine->Get<ComponentManager>()->Destroy(reinterpret_cast<TextureComponent*>(component));
   ```
   This is the only active ComponentManager call in worldexplorer and the only change needed.

- [ ] **Step 2: Replace the TextureComponent Destroy in destroyComponent()**

Before (line 270):
```cpp
// TODO Phase2-migrate: Task 13 — TextureComponent is GPU-resource managed; migrate when Task 13 clarifies ownership
g_Engine->Get<ComponentManager>()->Destroy(reinterpret_cast<TextureComponent*>(component));
```

After:
```cpp
g_Engine->getRenderingServer()->Delete(reinterpret_cast<TextureComponent*>(component));
```

Remove the `TODO Phase2-migrate` comment — this task resolves it.

- [ ] **Step 3: Remove the ComponentManager include from worldexplorer.cpp**

After Step 2, worldexplorer.cpp no longer calls ComponentManager. Remove `#include "ComponentManager.h"` (or whatever path it uses — find it by searching the top of the file).

Note: `addComponent<T>()` still references `ComponentManager` via its `TODO` comment and body — that function will be migrated in a later ECS task (Task 14, EntityRegistry). Do NOT remove the include if `addComponent<T>()` still needs it. Check whether `addComponent<T>()` is actually still active (line 211 still uses `ComponentManager::Spawn<T>`). If it is, skip this step and add a note that the include can be removed when Task 14 is done.

- [ ] **Step 4: Read SceneService.cpp around the ComponentManager CleanUp call**

The call `g_Engine->Get<ComponentManager>()->CleanUp(ObjectLifespan::Scene)` is a no-op for GPU resources (all Persistence). Remove the call and the ComponentManager include.

Also delete the dead `AddComponentToSceneHierarchyMap<T>()` function body (lines ~218–235). All call sites are commented-out `TODO Phase2-migrate` blocks, so the template is never instantiated and doesn't compile — but it still references `ComponentManager::GetAll<T>()`, which will be gone in Task 7. Delete the entire template definition (or convert to a `static_assert(false)` stub). Deleting it is cleaner since it has no active callers.

- [ ] **Step 5: Clean up DrawCallService.cpp**

Open `Source/Engine/Services/DrawCallService.cpp`. Line 195 has a dead commented-out line:
```cpp
// auto* l_texture = g_Engine->Get<ComponentManager>()->FindByUUID<TextureComponent>(l_textureID);
```
Delete this line entirely. If `DrawCallService.cpp` has a `#include "ComponentManager.h"` (check line ~4), remove that include too.

- [ ] **Step 6: Build and run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

- [ ] **Step 7: Commit**

```bash
git add Source/Editor/worldexplorer.cpp
git add Source/Engine/Services/SceneService.cpp
git add Source/Engine/Services/DrawCallService.cpp
git commit -m "refactor: replace ComponentManager usage in worldexplorer, SceneService, DrawCallService"
```

---

## Task 7: Delete ComponentManager and clean up Engine.cpp

**Files:**
- Delete: `Source/Engine/Services/ComponentManager.h`
- Modify: `Source/Engine/Engine.cpp`
- Modify: `Build/Source/Engine/Services/Services.vcxproj` (remove file entry)

- [ ] **Step 1: Verify no remaining ComponentManager includes**

```
powershell.exe -Command "Get-ChildItem -Path 'C:\GitRepo\InnocenceEngine\Source' -Recurse -Include '*.h','*.cpp' | Select-String 'ComponentManager' | Select-Object -ExpandProperty Filename | Sort-Object -Unique" 2>&1
```

Expected: zero results (or only `ComponentManager.h` itself). If any other file is listed, fix that file before continuing. Note: build outputs and generated files in the `Build/` directory are not scanned here — the full rebuild in Step 5 will catch any remaining references from generated unity builds or precompiled headers.

- [ ] **Step 2: Delete ComponentManager.h**

```bash
git rm Source/Engine/Services/ComponentManager.h
```

- [ ] **Step 3: Remove ComponentManager from Engine.cpp**

Find and remove:
- `#include "Services/ComponentManager.h"`
- `Get<ComponentManager>()` registration in `CreateServices()`
- Any Setup/Initialize/Terminate/Update calls on ComponentManager

- [ ] **Step 4: Remove ComponentManager from the Visual Studio project file**

```
Build/Source/Engine/Services/Services.vcxproj
```

Find the `<ClInclude Include="..\..\..\..\Source\Engine\Services\ComponentManager.h" />` entry and remove it.

- [ ] **Step 5: Full rebuild**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: zero errors, zero warnings about ComponentManager.

- [ ] **Step 6: Run full test suite**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: Test.exe all pass, RenderTest.exe exits 0.

- [ ] **Step 7: Commit**

```bash
git rm Source/Engine/Services/ComponentManager.h
git add Source/Engine/Engine.cpp
git add Build/Source/Engine/Services/Services.vcxproj
git commit -m "refactor: delete ComponentManager — GPU resource pools now owned by IRenderingServer"
```
