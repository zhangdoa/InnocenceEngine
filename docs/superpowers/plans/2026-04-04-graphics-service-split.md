# Graphics Service Split & AssetHandle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the monolithic `IGraphicsService` into three focused services (GraphicsHardwareService, GraphicsResourceService, FrameManagementService) and introduce `AssetHandle<T>` so runtime components reference loaded assets without owning GPU state.

**Architecture:** Runtime components (MeshComponent, MaterialComponent) become lightweight references holding only an `AssetHandle`. The AssetService resolves disk addresses to loaded data; the GraphicsResourceService materializes loaded data into GPU resources and produces the GPU-side handle. FrameManagementService orchestrates per-frame command recording, execution, and presentation. GraphicsHardwareService owns the device, factories, and persistent hardware objects.

**Tech Stack:** C++17, DirectX 12 (primary backend), engine wrappers (STL14.h/STL17.h)

---

## Phase 0: Preparatory Cleanup

Before splitting services, fix the immediate issues blocking correct scene transitions and establish the invariants the new architecture relies on.

---

### Task 0.1: Commit Current Bug Fixes

The working tree has uncommitted fixes from the TDR investigation. These are prerequisite for any further work.

**Files:**
- `Source/Engine/Services/Common/IGraphicsService.cpp` (GPU drain + material fix)
- `Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp` (DrawInstanced warning + m_TLASReady guard)
- `Source/Engine/Services/IGraphicsService.h` (m_TLASReady field)
- `Source/Engine/Services/DX12/DX12GraphicsService_GraphicsDevice_Protected.cpp` (m_TLASReady management)
- `Source/DefaultClient/LogicClient/World.inl` (restore scene transition auto-test)

- [ ] **Step 1: Verify build passes**

Run: `powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"`

- [ ] **Step 2: Commit all fixes**

```bash
git add Source/Engine/Services/Common/IGraphicsService.cpp \
        Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp \
        Source/Engine/Services/IGraphicsService.h \
        Source/Engine/Services/DX12/DX12GraphicsService_GraphicsDevice_Protected.cpp \
        Source/DefaultClient/LogicClient/World.inl
git commit -m "fix: GPU drain before scene unloading, material cleanup, DrawInstanced warnings, TLAS guard"
```

---

## Phase 1: AssetHandle Introduction

Introduce a typed `AssetHandle<T>` that decouples runtime components from GPU resource ownership. This is the foundation everything else builds on.

---

### Task 1.1: Define AssetHandle and AssetRegistry

**Files:**
- Create: `Source/Engine/Common/AssetHandle.h`

The `AssetHandle` is a lightweight typed identifier for a loaded asset. It contains an index into a flat array (like the existing `GPUMeshResourceHandle` pattern but generalized). The asset registry maps handles to loaded asset data.

- [ ] **Step 1: Write AssetHandle**

```cpp
// Source/Engine/Common/AssetHandle.h
#pragma once
#include "STL14.h"

namespace Inno
{
    // Forward declarations for asset data types
    struct MeshAssetData;
    struct TextureAssetData;
    struct MaterialAssetData;

    template<typename T>
    struct AssetHandle
    {
        uint32_t m_Index = UINT32_MAX;
        uint32_t m_Generation = 0;

        bool IsValid() const { return m_Index != UINT32_MAX; }

        bool operator==(const AssetHandle& other) const
        {
            return m_Index == other.m_Index && m_Generation == other.m_Generation;
        }
        bool operator!=(const AssetHandle& other) const { return !(*this == other); }
    };

    using MeshAssetHandle = AssetHandle<MeshAssetData>;
    using TextureAssetHandle = AssetHandle<TextureAssetData>;
    using MaterialAssetHandle = AssetHandle<MaterialAssetData>;
}

namespace std
{
    template<typename T>
    struct hash<Inno::AssetHandle<T>>
    {
        size_t operator()(const Inno::AssetHandle<T>& h) const
        {
            return hash<uint64_t>()(static_cast<uint64_t>(h.m_Index) << 32 | h.m_Generation);
        }
    };
}
```

- [ ] **Step 2: Build**

Run: `powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"`

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Common/AssetHandle.h
git commit -m "feat: add typed AssetHandle for asset indirection"
```

---

### Task 1.2: Define Asset Data Structs

**Files:**
- Create: `Source/Engine/Common/AssetData.h`

Asset data structs hold the materialized (loaded) form of an asset. For meshes, this includes GPU buffer views and the AABB. For textures, the texture descriptor and GPU resource pointers. These replace the scattered fields currently embedded across `GPUMeshResource`, `TextureComponent`, and `MaterialComponent`.

- [ ] **Step 1: Write AssetData types**

```cpp
// Source/Engine/Common/AssetData.h
#pragma once
#include "GraphicsPrimitive.h"
#include "AssetHandle.h"
#include "Math.h"

namespace Inno
{
    enum class AssetResidency : uint8_t
    {
        Unloaded,
        Loading,
        Resident,
        Released
    };

    struct MeshAssetData
    {
        ObjectName m_Name;
        ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
        AssetResidency m_Residency = AssetResidency::Unloaded;

        // CPU-side cached data (for readback, path tracer, BVH)
        void* m_MappedMemory_VB = nullptr;
        void* m_MappedMemory_IB = nullptr;

        // GPU-side views (populated by GraphicsResourceService)
        GPUBufferView m_VertexBufferView;
        GPUBufferView m_IndexBufferView;
        AABB m_AABB;

        uint32_t GetVertexCount() const;
        uint32_t GetIndexCount() const;
    };

    struct TextureAssetData
    {
        ObjectName m_Name;
        ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
        AssetResidency m_Residency = AssetResidency::Unloaded;

        TextureDesc m_TextureDesc;
        // GPU resource references stored externally by GraphicsResourceService
    };

    struct MaterialAssetData
    {
        ObjectName m_Name;
        ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
        AssetResidency m_Residency = AssetResidency::Unloaded;

        MaterialAttributes m_Attributes;
        // Texture slot references as asset handles
        std::vector<TextureAssetHandle> m_TextureSlots;
    };
}
```

- [ ] **Step 2: Build**
- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Common/AssetData.h
git commit -m "feat: add MeshAssetData, TextureAssetData, MaterialAssetData structs"
```

---

### Task 1.3: Add AssetRegistry to AssetService

**Files:**
- Modify: `Source/Engine/Services/AssetService.h`
- Modify: `Source/Engine/Services/AssetService.cpp`

The AssetService becomes the authority for asset lifecycle. It owns flat arrays of `MeshAssetData`, `TextureAssetData`, `MaterialAssetData`. It allocates handles, tracks residency, and manages lifespan-based unloading.

- [ ] **Step 1: Add registry storage and allocation API to AssetService**

Add to `AssetService.h`:
```cpp
// Asset registry — allocate, query, release by lifespan
static MeshAssetHandle AllocateMeshAsset(const char* name, ObjectLifespan lifespan);
static TextureAssetHandle AllocateTextureAsset(const char* name, ObjectLifespan lifespan);
static MaterialAssetHandle AllocateMaterialAsset(const char* name, ObjectLifespan lifespan);

static MeshAssetData* GetMeshAsset(MeshAssetHandle handle);
static TextureAssetData* GetTextureAsset(TextureAssetHandle handle);
static MaterialAssetData* GetMaterialAsset(MaterialAssetHandle handle);

static void ReleaseAssetsByLifespan(ObjectLifespan lifespan);
```

Implementation follows the same pattern as the existing `AllocateMeshResource` (LUT + free list + flat vector), but generalized for all asset types and with generation counters for safe handle validation.

- [ ] **Step 2: Implement registry in AssetService.cpp**

Use the existing `m_MeshResourceLUT` / `m_FreeMeshResourceSlots` pattern as reference. Each asset type gets:
- `std::vector<T>` flat storage
- `std::vector<uint32_t>` free list
- `ThreadSafeUnorderedMap<std::string, AssetHandle<T>>` name LUT
- `std::vector<uint32_t>` generation counters (one per slot)

`ReleaseAssetsByLifespan(ObjectLifespan)` iterates all asset arrays and releases entries matching the given lifespan, calling into GraphicsResourceService to free GPU resources.

- [ ] **Step 3: Build and verify**
- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Services/AssetService.h Source/Engine/Services/AssetService.cpp
git commit -m "feat: add asset registry with typed handles to AssetService"
```

---

### Task 1.4: Migrate MeshComponent to AssetHandle

**Files:**
- Modify: `Source/Engine/Component/MeshComponent.h`
- Modify: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp` (initialization path)
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp` (BLAS lookup)
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` (geometry buffer rebuild)

This is the critical migration. MeshComponent changes from holding `GPUMeshResourceHandle m_GPUResource` to holding `MeshAssetHandle m_Asset`. All call sites that read `m_GPUResource` are updated to go through `AssetService::GetMeshAsset()`.

- [ ] **Step 1: Update MeshComponent**

```cpp
// Source/Engine/Component/MeshComponent.h
#pragma once
#include "../Common/AssetHandle.h"
#include "../Common/GraphicsPrimitive.h"

namespace Inno
{
    struct MeshComponent
    {
        static uint32_t GetTypeID() { return 6; }

        MeshAssetHandle m_Asset;
        ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
        ObjectName m_InstanceName;
    };
}
```

- [ ] **Step 2: Update DrawCallService::UpdateDrawCalls**

Replace `GetMeshResource(l_mesh.m_GPUResource)` with `AssetService::GetMeshAsset(l_mesh.m_Asset)`. The `MeshAssetData` has the same `m_VertexBufferView` / `m_IndexBufferView` fields.

- [ ] **Step 3: Update JSONWrapper mesh loading**

For template (non-customized) meshes: instead of `component = *template`, set `component.m_Asset = template->m_Asset` and `component.m_ObjectStatus = ObjectStatus::Activated`.

For customized meshes: call `AssetService::AllocateMeshAsset()` to get a handle, then enqueue GPU initialization.

- [ ] **Step 4: Update IGraphicsService mesh initialization**

`IGraphicsService::Initialize(MeshComponent*, vertices, indices, owner)` now:
1. Calls `AssetService::AllocateMeshAsset(name, lifespan)` to get the handle
2. Sets `mesh->m_Asset = handle`
3. Queues the GPU init task (referencing the asset handle, not the component pointer)

- [ ] **Step 5: Update DX12 BLAS lookup in InitializeImpl(EntityID)**

Replace `m_DX12MeshResources.find(l_mesh->m_GPUResource.m_Index)` with lookup via the asset handle.

- [ ] **Step 6: Update GPUPathTracerPass::RebuildGeometryBuffers**

Replace `GetMeshResource(l_mesh.m_GPUResource)` with `AssetService::GetMeshAsset(l_mesh.m_Asset)`.

- [ ] **Step 7: Build and test**

Run build. Run `draw_instanced` offscreen test for smoke test. Manual testing needed for scene transitions.

- [ ] **Step 8: Commit**

```bash
git commit -m "refactor: migrate MeshComponent from GPUMeshResourceHandle to MeshAssetHandle"
```

---

### Task 1.5: Migrate MaterialComponent to AssetHandle

**Files:**
- Modify: `Source/Engine/Component/MaterialComponent.h`
- Modify: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`

Similar to the mesh migration. MaterialComponent currently holds `m_materialAttributes`, `m_TextureComponents` (texture names), and `m_ObjectStatus`. Replace with `MaterialAssetHandle m_Asset` referencing `MaterialAssetData` in the registry.

- [ ] **Step 1: Update MaterialComponent to hold MaterialAssetHandle**
- [ ] **Step 2: Update DrawCallService material buffer population**
- [ ] **Step 3: Update JSON loading for materials**
- [ ] **Step 4: Update IGraphicsService material initialization path**
- [ ] **Step 5: Build and test**
- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: migrate MaterialComponent to MaterialAssetHandle"
```

---

### Task 1.6: Migrate TextureComponent References

**Files:**
- Modify: Material asset data (texture slot references become `TextureAssetHandle`)
- Modify: `Source/Engine/Services/DrawCallService.cpp` (texture index lookup)
- Modify: Rendering passes that bind textures by name

TextureComponent is more complex because it's used both as a scene asset (loaded textures) and as a rendering pipeline resource (render targets, swap chain images). Only scene-loaded textures migrate to AssetHandle; pipeline textures remain as pool-managed components owned by the GraphicsResourceService (later).

- [ ] **Step 1: Identify texture usage categories**

Categorize every `TextureComponent` usage:
- **Scene asset textures** (loaded from disk, referenced by materials) → migrate to `TextureAssetHandle`
- **Pipeline render targets** (created by render passes, owned by graphics service) → stay as pool-managed components
- **Engine textures** (BRDF LUT, noise, default textures) → persistence-scoped assets

- [ ] **Step 2: Update material texture resolution**

Materials currently store texture names as strings. Change to store `TextureAssetHandle` in `MaterialAssetData::m_TextureSlots`. During material loading, resolve texture names to handles via `AssetService::AllocateTextureAsset()`.

- [ ] **Step 3: Update texture binding in DrawCallService**

Replace `FindTextureByName` + `GetIndex` chain with asset handle lookup.

- [ ] **Step 4: Build and test**
- [ ] **Step 5: Commit**

```bash
git commit -m "refactor: migrate scene textures to TextureAssetHandle, separate from pipeline textures"
```

---

### Task 1.7: Lifespan-Based Asset Cleanup on Scene Transition

**Files:**
- Modify: `Source/Engine/Services/SceneService.cpp`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

Replace the current `OnSceneUnloading()` approach (which manually iterates scene entities and invalidates components) with `AssetService::ReleaseAssetsByLifespan(ObjectLifespan::Scene)`. The GPU drain stays (it's still needed), but the manual component iteration goes away.

- [ ] **Step 1: Add AssetService::ReleaseAssetsByLifespan implementation**

For each asset type, iterate the flat array. For entries matching the lifespan:
1. Call GraphicsResourceService to release GPU resources (BLAS, buffers, descriptors)
2. Reset the asset data entry
3. Push the slot index onto the free list
4. Remove from the name LUT
5. Increment the generation counter (invalidates outstanding handles)

- [ ] **Step 2: Simplify OnSceneUnloading**

Remove the per-entity mesh/material invalidation loop. Replace `ReleaseAllMeshResources(Scene)` with `AssetService::ReleaseAssetsByLifespan(Scene)`. Keep the GPU drain and the init queue cleanup.

- [ ] **Step 3: Remove the `component = *template` copy pattern from JSONWrapper**

Template mesh loading now just copies the asset handle: `component.m_Asset = templateComponent->m_Asset`. No GPU state is aliased.

- [ ] **Step 4: Build and test scene transitions**
- [ ] **Step 5: Commit**

```bash
git commit -m "refactor: lifespan-based asset cleanup replaces manual component invalidation"
```

---

## Phase 2: GraphicsHardwareService Extraction

Extract device/factory creation and persistent hardware objects into a dedicated service.

---

### Task 2.1: Define GraphicsHardwareService Interface

**Files:**
- Create: `Source/Engine/Services/GraphicsHardwareService.h`
- Create: `Source/Engine/Services/DX12/DX12GraphicsHardwareService.h`
- Create: `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp`

**Responsibilities extracted from IGraphicsService:**
- `CreatePhysicalDevices` (DXGI adapter enumeration)
- `CreateDebugCallback` (D3D12 debug layer)
- Device creation (`ID3D12Device`)
- Global command queue creation (`CreateGlobalCommandQueues`)
- Global command allocator creation (`CreateGlobalCommandAllocators`)
- Global descriptor heap creation (`CreateGlobalDescriptorHeaps`)
- Fence/sync primitive creation (`CreateSyncPrimitives`)
- PIX/RenderDoc capture integration (`BeginCapture`/`EndCapture`)
- Mipmap generator (hardware utility)

**Interface:**
```cpp
class GraphicsHardwareService : public IService
{
public:
    bool Setup(IServiceConfig*) override;
    bool Initialize() override;
    bool Terminate() override;

    // Device access
    virtual void* GetDevice() = 0;
    virtual void* GetCommandQueue(GPUEngineType) = 0;
    virtual void* GetCommandAllocator(GPUEngineType) = 0;

    // Sync primitives
    virtual ISemaphore* CreateSemaphore() = 0;
    virtual uint64_t GetSemaphoreValue(GPUEngineType) = 0;
    virtual bool SignalOnGPU(ISemaphore*, GPUEngineType) = 0;
    virtual bool WaitOnCPU(uint64_t, GPUEngineType) = 0;
    virtual bool WaitOnGPU(ISemaphore*, GPUEngineType waitQueue, GPUEngineType signalQueue) = 0;

    // Command list management
    virtual CommandListComponent* CreateCommandList(GPUEngineType) = 0;
    virtual bool Open(CommandListComponent*, GPUEngineType) = 0;
    virtual bool Close(CommandListComponent*, GPUEngineType) = 0;
    virtual bool Execute(CommandListComponent*, GPUEngineType) = 0;

    // Debug/capture
    virtual bool BeginCapture() = 0;
    virtual bool EndCapture() = 0;
    virtual bool HasGPUError() const = 0;
};
```

- [ ] **Step 1: Create GraphicsHardwareService.h with the interface above**
- [ ] **Step 2: Create DX12GraphicsHardwareService extracting device/queue/allocator/fence code**

Source: `DX12GraphicsService_GraphicsDevice_Private.cpp` (CreatePhysicalDevices, CreateGlobalCommandQueues, CreateGlobalCommandAllocators, CreateSyncPrimitives, CreateGlobalDescriptorHeaps)
Source: `DX12GraphicsService_APISpecific.cpp` (GetDevice, GetGlobalCommandAllocator, GetGlobalCommandQueue, GetDescriptorHeapAccessor)

- [ ] **Step 3: Register in Engine.cpp**

Add `GraphicsHardwareService` to the service creation order, before `GraphicsResourceService` and `FrameManagementService`.

- [ ] **Step 4: Wire IGraphicsService to delegate to GraphicsHardwareService**

Initially, IGraphicsService can hold a pointer to GraphicsHardwareService and forward calls. The existing code keeps working during migration.

- [ ] **Step 5: Build and test**
- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: extract GraphicsHardwareService from IGraphicsService"
```

---

### Task 2.2: Migrate All Device Access to GraphicsHardwareService

**Files:**
- Modify: All DX12 `.cpp` files that call `m_device->` directly
- Modify: All services/passes that call `getGraphicsService()->GetDevice()`

Replace direct `m_device` access within the old DX12GraphicsService with `g_Engine->Get<GraphicsHardwareService>()->GetDevice()`. This is a mechanical search-and-replace across the DX12 backend files.

- [ ] **Step 1: Grep for m_device usage, categorize by file**
- [ ] **Step 2: Replace m_device references with hardware service delegation**
- [ ] **Step 3: Build and test**
- [ ] **Step 4: Commit**

```bash
git commit -m "refactor: migrate all device access to GraphicsHardwareService"
```

---

## Phase 3: GraphicsResourceService Extraction

Extract resource pool management and GPU resource creation into a dedicated service.

---

### Task 3.1: Define GraphicsResourceService Interface

**Files:**
- Create: `Source/Engine/Services/GraphicsResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12GraphicsResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12GraphicsResourceService.cpp`

**Responsibilities extracted from IGraphicsService:**
- All `AddXxxComponent` methods (component pool allocation)
- All `Delete(Xxx*)` methods (pool deallocation)
- All `InitializeImpl(Xxx*)` methods (GPU resource creation + upload)
- `GPUHandlePools` ownership
- `m_MeshResources` / `m_DX12MeshResources` ownership
- Descriptor heap accessor management
- `WriteMappedMemory`, `Upload`, `UploadToGPU`
- Mesh/texture/material asset → GPU materialization
- `ReleaseFromPool`, `ReleaseMeshGPUResourceImpl`

**Interface:**
```cpp
class GraphicsResourceService : public IService
{
public:
    // Pool allocation
    virtual TextureComponent*       AddTextureComponent(const char* name = "") = 0;
    virtual RenderPassComponent*    AddRenderPassComponent(const char* name = "") = 0;
    virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
    virtual SamplerComponent*       AddSamplerComponent(const char* name = "") = 0;
    virtual GPUBufferComponent*     AddGPUBufferComponent(const char* name = "") = 0;
    virtual CommandListComponent*   AddCommandListComponent(const char* name = "") = 0;

    // Pool deallocation
    virtual bool Delete(TextureComponent*) = 0;
    virtual bool Delete(GPUBufferComponent*) = 0;
    // ... etc for each pooled type

    // GPU materialization (called by AssetService or internally)
    virtual bool MaterializeMesh(MeshAssetHandle, std::vector<Vertex>&, std::vector<Index>&) = 0;
    virtual bool MaterializeTexture(TextureAssetHandle, void* data) = 0;
    virtual bool MaterializeMaterial(MaterialAssetHandle) = 0;

    // GPU resource release (called by AssetService on lifespan cleanup)
    virtual bool ReleaseMeshGPUResources(MeshAssetHandle) = 0;
    virtual bool ReleaseTextureGPUResources(TextureAssetHandle) = 0;

    // Initialization queue processing (called per frame by FrameManagementService)
    bool ProcessDeferredInitializations();

    // Query
    virtual MeshAssetData* GetMeshAsset(MeshAssetHandle) = 0;
    virtual std::optional<uint32_t> GetTextureIndex(TextureComponent*, Accessibility) = 0;
    virtual TextureComponent* FindTextureByName(const char*) = 0;
};
```

- [ ] **Step 1: Create GraphicsResourceService.h**
- [ ] **Step 2: Create DX12GraphicsResourceService with pool + initialization code**

Move from IGraphicsService.cpp: `InitializePool`, `TerminatePool`, all `AddXxxComponent`, `InitializeComponents`
Move from DX12GraphicsService_ComponentPool.cpp: all `Delete`, all `InitializeImpl`, `UploadToGPU`

- [ ] **Step 3: Move GPUHandlePools and mesh resource arrays**
- [ ] **Step 4: Register in Engine.cpp, wire callers**
- [ ] **Step 5: Build and test**
- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: extract GraphicsResourceService from IGraphicsService"
```

---

### Task 3.2: Migrate Rendering Passes to GraphicsResourceService

**Files:**
- Modify: All rendering pass `.cpp` files in `Source/DefaultClient/RenderingClient/`
- Modify: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/Services/PerFrameDataService.cpp`
- Modify: `Source/Engine/Services/LightDataService.cpp`

Replace `getGraphicsService()->AddGPUBufferComponent(...)`, `getGraphicsService()->Initialize(...)`, etc. with `g_Engine->Get<GraphicsResourceService>()->...`.

This is a large but mechanical migration. Each rendering pass and data service that allocates GPU resources switches from `getGraphicsService()` to `Get<GraphicsResourceService>()`.

- [ ] **Step 1: Migrate DrawCallService, PerFrameDataService, LightDataService**
- [ ] **Step 2: Migrate all rendering passes (OpaquePass, LightPass, etc.)**
- [ ] **Step 3: Build and test**
- [ ] **Step 4: Commit**

```bash
git commit -m "refactor: migrate all GPU resource allocation to GraphicsResourceService"
```

---

## Phase 4: FrameManagementService Extraction

Extract the frame loop, command list orchestration, and swap chain presentation into a dedicated service.

---

### Task 4.1: Define FrameManagementService Interface

**Files:**
- Create: `Source/Engine/Services/FrameManagementService.h`
- Create: `Source/Engine/Services/DX12/DX12FrameManagementService.h`
- Create: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp`

**Responsibilities extracted from IGraphicsService:**
- `Update()` frame loop (the BeginFrame → InitComponents → Callbacks → PrepareGlobal → Execute → Present sequence)
- Swap chain ownership and `Present()`/`Resize()`
- Frame index tracking (`m_CurrentFrame`, `GetCurrentFrame()`, `GetNextFrame()`)
- Per-frame fence value tracking (`m_GraphicsSemaphoreValues[]`, `m_ComputeSemaphoreValues[]`)
- Callback registration (`SetUploadHeapPreparationCallback`, `SetCommandPreparationCallback`, `SetCommandExecutionCallback`)
- Swap chain render pass and its command list
- Global command lists (`m_GlobalGraphicsCommandLists`)
- `PrepareGlobalCommands()`, `ExecuteGlobalCommands()`, `PrepareSwapChainCommands()`, `ExecuteSwapChainCommands()`
- `PrepareRayTracing()`

**Interface:**
```cpp
class FrameManagementService : public IService
{
public:
    bool Setup(IServiceConfig*) override;
    bool Initialize() override;
    bool Update() override;  // The main frame tick
    bool Terminate() override;

    // Frame queries
    uint32_t GetCurrentFrame();
    uint32_t GetSwapChainImageCount();
    uint32_t GetFrameCountSinceLaunch();

    // Callbacks from Engine
    void SetUploadHeapPreparationCallback(std::function<bool()>&&);
    void SetCommandPreparationCallback(std::function<bool()>&&);
    void SetCommandExecutionCallback(std::function<bool()>&&);

    // Swap chain
    RenderPassComponent* GetSwapChainRenderPassComponent();
    virtual bool Resize() = 0;

    // User pipeline output
    bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&&);
    GPUResourceComponent* GetUserPipelineOutput();

    // Scene transition support
    void DrainGPU();  // Signal+wait all queues — used before resource teardown
};
```

- [ ] **Step 1: Create FrameManagementService.h**
- [ ] **Step 2: Create DX12FrameManagementService**

Move from IGraphicsService.cpp: `Update()`, swap chain setup, global command list management, callback storage
Move from DX12GraphicsService_GraphicsDevice_Protected.cpp: `BeginFrame`, `PresentImpl`, `EndFrame`, `PrepareRayTracing`, swap chain image management, `ResizeImpl`

- [ ] **Step 3: Register in Engine.cpp**

Replace `getGraphicsService()->Update()` with `Get<FrameManagementService>()->Update()`.

- [ ] **Step 4: Update Engine callback setup**

`Engine.cpp` currently sets callbacks on `m_GraphicsService`. Change to set them on `FrameManagementService`.

- [ ] **Step 5: Build and test**
- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: extract FrameManagementService from IGraphicsService"
```

---

### Task 4.2: Migrate Frame Queries to FrameManagementService

**Files:**
- Modify: All files that call `getGraphicsService()->GetCurrentFrame()`
- Modify: All files that call `getGraphicsService()->GetSwapChainRenderPassComponent()`
- Modify: `Source/Engine/Services/HIDService.cpp` (Resize)

Replace `getGraphicsService()->GetCurrentFrame()` with `Get<FrameManagementService>()->GetCurrentFrame()` across all services and rendering passes.

- [ ] **Step 1: Grep and migrate all GetCurrentFrame callers**
- [ ] **Step 2: Grep and migrate all swap chain / resize callers**
- [ ] **Step 3: Build and test**
- [ ] **Step 4: Commit**

```bash
git commit -m "refactor: migrate frame queries to FrameManagementService"
```

---

## Phase 5: Command Recording API Migration

Move command list recording APIs out of the old IGraphicsService.

---

### Task 5.1: Move Command Recording to FrameManagementService or Dedicated API

**Files:**
- Modify: `Source/Engine/Services/FrameManagementService.h` (or create a CommandRecordingAPI)
- Modify: All rendering passes that call `BindGPUResource`, `ExecuteIndirect`, `DispatchRays`, etc.

Command recording APIs (`CommandListBegin`, `BindRenderPassComponent`, `BindGPUResource`, `ExecuteIndirect`, `DrawInstanced`, `Dispatch`, `DispatchRays`, `TryToTransitState`, etc.) are tightly coupled to the frame lifecycle. They live on FrameManagementService (since they operate within a frame context) or on GraphicsHardwareService (since they're hardware API wrappers).

**Decision:** Place them on GraphicsHardwareService — they're thin wrappers around DX12 command list APIs and don't carry frame state. FrameManagementService orchestrates WHEN they're called; GraphicsHardwareService provides the HOW.

- [ ] **Step 1: Move command recording APIs to GraphicsHardwareService**
- [ ] **Step 2: Update all rendering pass callers**
- [ ] **Step 3: Build and test**
- [ ] **Step 4: Commit**

```bash
git commit -m "refactor: move command recording APIs to GraphicsHardwareService"
```

---

## Phase 6: Retire IGraphicsService

### Task 6.1: Remove IGraphicsService

**Files:**
- Delete: `Source/Engine/Services/IGraphicsService.h`
- Delete: `Source/Engine/Services/Common/IGraphicsService.cpp`
- Delete: `Source/Engine/Services/DX12/DX12GraphicsService.h`
- Delete: All `DX12GraphicsService_*.cpp` files
- Modify: `Source/Engine/Engine.h` — remove `getGraphicsService()`
- Modify: `Source/Engine/Engine.cpp` — remove graphics service creation, replace with three new services

At this point, all functionality has been migrated:
- Hardware → GraphicsHardwareService
- Resources → GraphicsResourceService
- Frame loop → FrameManagementService
- Asset data → AssetService

- [ ] **Step 1: Verify no remaining references to IGraphicsService/getGraphicsService**
- [ ] **Step 2: Remove old files**
- [ ] **Step 3: Update CMakeLists.txt for new file organization**
- [ ] **Step 4: Full build and test suite**
- [ ] **Step 5: Commit**

```bash
git commit -m "refactor: retire IGraphicsService, split complete"
```

---

## Dependency Graph

```
Phase 0 ── Phase 1 ── Phase 2 ── Phase 3 ── Phase 4 ── Phase 5 ── Phase 6
 (fixes)    (Asset     (Hardware)  (Resource)  (Frame)    (CmdList)   (Retire)
             Handle)
```

Each phase produces a compiling, runnable engine. No phase breaks the build.

---

## Architectural Adjustments & Notes

1. **Command recording placement:** The plan puts command recording APIs on GraphicsHardwareService since they're DX12 API wrappers. If this feels wrong during implementation, they could alternatively be a separate `CommandRecordingService` — but three services is already a clean split.

2. **Pipeline render targets vs. scene textures:** The plan explicitly separates these in Task 1.6. Pipeline textures (render targets, depth buffers) stay as pool-managed `TextureComponent` objects owned by GraphicsResourceService. Only scene-loaded textures (albedo maps, normal maps) migrate to `TextureAssetHandle`.

3. **TLAS instance management:** Currently `InitializeImpl(EntityID)` creates per-entity TLAS instance descs and is never called for scene entities. This should be redesigned: FrameManagementService's `PrepareRayTracing` should discover entities with mesh assets and build TLAS instances dynamically each frame (or when the entity set changes), rather than relying on a one-time initialization queue that nobody calls.

4. **Vulkan/Metal backends:** The plan focuses on DX12. The VK and MT backends would follow the same split pattern but are out of scope for this plan.

5. **The `component = *template` pattern:** Eliminated in Task 1.4/1.7. After migration, template mesh loading copies only the asset handle, not internal GPU state.

6. **Engine rename:** The user mentioned renaming Engine to ServiceManager/ServiceScheduler. This is orthogonal to the graphics split and can be done separately.
