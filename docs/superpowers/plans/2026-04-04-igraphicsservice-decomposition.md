# IGraphicsService Decomposition Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move actual implementations from the IGraphicsService monolith into GraphicsResourceService, GraphicsHardwareService, and FrameManagementService, then delete the delegation wrappers and thin out IGraphicsService to just a DX12 backend interface.

**Architecture:** The three focused service interfaces already exist and all callers are migrated. Currently they are thin delegation wrappers forwarding to IGraphicsService. This plan moves the shared state (pools, queues, counters, callbacks) and non-virtual method implementations directly into the focused services, leaving IGraphicsService as just the pure virtual DX12-specific backend interface. Each focused service holds a pointer to the backend for DX12-specific calls.

**Tech Stack:** C++17, DX12, MSVC

**Key insight:** IGraphicsService has two roles — (1) common implementation (pools, queues, frame loop) and (2) virtual interface for DX12-specific overrides. We move (1) into the focused services and keep (2) as a thin backend interface.

---

## Current State

- `IGraphicsService` (header: 344 lines, impl: 1285 lines) — monolith owning pools, init queues, frame counters, callbacks, frame loop, resize logic
- `DX12GraphicsService` inherits `IGraphicsService`, overrides ~40 virtual methods across 8 .cpp files
- Three delegation wrappers exist: `DX12GraphicsHardwareService`, `DX12GraphicsResourceService`, `DX12FrameManagementService` — each forwards all calls to `m_Backend` (the IGraphicsService instance)
- All external callers already use the focused services; `getGraphicsService()` is private

## Target State

- `GraphicsResourceService` — concrete class owning pools, init queues, mesh resources, scene lifecycle. Calls DX12 backend for `InitializeImpl()`, `Delete()`, pool overrides
- `FrameManagementService` — concrete class owning frame counters, swap chain, callbacks, the main Update loop, resize. Calls DX12 backend for `BeginFrame()`, `PresentImpl()`, `EndFrame()`, swap chain images
- `GraphicsHardwareService` — concrete class owning GPU sync convenience methods. Calls DX12 backend for low-level sync, command list ops, device access
- `IGraphicsService` — thin abstract interface with only DX12-specific pure virtual methods (no member data, no common implementation)
- Delegation wrappers deleted

## Method + Data Assignment

### → GraphicsResourceService
**Data to move from IGraphicsService:**
- `GPUHandlePools m_GPUHandlePools` (pools, LUTs, pointer vectors)
- `m_MeshResources`, `m_FreeMeshResourceSlots`, `m_MeshResourceLUT`
- Init queues: `m_uninitializedMeshes/Textures/Materials/GPUBuffers/RenderPasses/Entities`
- Init tracking sets: `m_initializedTextures/Materials/Entities`
- Raytracing: `m_TLASBufferComponent`, `m_ScratchBufferComponent`, `m_RaytracingInstanceBufferComponent`, `m_TLASReady`, `m_RaytracingInstanceDescs`
- `ReleaseFromPool` template

**Methods to move from IGraphicsService.cpp:**
- `InitializePool()`, `TerminatePool()` — pool lifecycle
- All `Add*Component()` methods + `AllocateGPUHandle` helper
- `FindTextureByName()`, `FindMaterialByName()`
- All `Initialize()` overloads (queuing methods)
- `InitializeImpl(MaterialComponent*)`, `InitializeImpl(RenderPassComponent*)` — non-virtual base impls
- `InitializeComponents()` — processes all init queues
- `CreateOutputMergerTargets()`, `InitializeOutputMergerTargets()`
- `DeleteRenderTargets()`
- `AllocateMeshResource()`, `ReleaseMeshResource()`, `ReleaseAllMeshResources()`, `GetMeshResource()`
- `OnSceneUnloading()`
- `WriteMappedMemory()`
- `Upload<T>()` templates
- Static `Accessibility` member definitions

**DX12 backend calls needed:** `InitializeImpl(Mesh/Texture/Shader/Sampler/Buffer/Entity/CommandList)`, `AddPipelineStateObject()`, `AddSemaphore()`, `Add(OutputMergerTarget)`, all `Delete()` overloads, `OnOutputMergerTargetsCreated()`, `CreatePipelineStateObject()`, `CreateFenceEvents()`, `ReleaseMeshGPUResourceImpl()`, `UploadToGPU()`, `Clear()`, `Copy()`, `GenerateMipmap()`, `GetIndex()`, `ReadRenderTargetSample()`, `ReadTextureBackToCPU()`, `OnSceneLoadingStart()`

### → FrameManagementService
**Data to move from IGraphicsService:**
- `m_CurrentFrame`, `m_PreviousFrame`, `m_FrameCountSinceLaunch`
- `m_swapChainImageCount`
- `m_GraphicsSemaphoreValues`, `m_ComputeSemaphoreValues`, `m_CopySemaphoreValues`
- `m_SwapChainRenderPassComp`, `m_SwapChainShaderProgramComp`, `m_SwapChainSamplerComp`
- `m_GlobalGraphicsCommandLists`, `m_GlobalSemaphore`
- `m_GetUserPipelineOutputFunc`
- Callbacks: `m_UploadHeapPreparationCallback`, `m_CommandPreparationCallback`, `m_CommandExecutionCallback`
- `m_needResize`

**Methods to move from IGraphicsService.cpp:**
- Frame queries: `GetCurrentFrame()`, `GetPreviousFrame()`, `GetNextFrame()`, `GetSwapChainImageCount()`, `GetFrameCountSinceLaunch()`
- Callbacks: `SetUploadHeapPreparationCallback()`, `SetCommandPreparationCallback()`, `SetCommandExecutionCallback()`
- `SetUserPipelineOutput()`, `GetUserPipelineOutput()`
- `GetSwapChainRenderPassComponent()`
- `Resize()`, `Present()` — including the resize drain logic
- `InitializeSwapChainRenderPassComponent()`
- The entire `Update()` frame loop (calling InitializeComponents on resource service, BeginFrame/EndFrame on backend, etc.)
- `PrepareGlobalCommands()`, `ExecuteGlobalCommands()` — these orchestrate uploads + raytracing prep
- `PrepareSwapChainCommands()`, `ExecuteSwapChainCommands()`
- `ExecuteResize()`, `PreResize()`, `PostResize()`

**DX12 backend calls needed:** `BeginFrame()`, `EndFrame()`, `PresentImpl()`, `ResizeImpl()`, `GetSwapChainImages()`, `AssignSwapChainImages()`, `ReleaseSwapChainImages()`, `PrepareRayTracing()`

**Cross-service calls:** FrameManagementService::Update() needs to call GraphicsResourceService::InitializeComponents() and use GraphicsHardwareService for sync/command ops.

### → GraphicsHardwareService
**Data:** None significant (convenience methods only delegate to backend)

**Methods already moved (convenience overloads in GraphicsHardwareService.cpp):**
- `SignalOnGPU(RenderPassComponent*, ...)`, `WaitOnGPU(RenderPassComponent*, ...)`
- `CommandListBegin(...)`, `CommandListEnd(...)`
- `ChangeRenderTargetStates(...)`

**DX12 backend calls:** All low-level sync/command methods are already pure virtual, forwarded to DX12GraphicsService. This service is mostly correct as-is — it just needs to own these convenience implementations instead of delegating.

---

### Task 1: Move resource pool data + methods into GraphicsResourceService

**Files:**
- Modify: `Source/Engine/Services/GraphicsResourceService.h`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsResourceService.h`
- Create: `Source/Engine/Services/Common/GraphicsResourceServiceImpl.cpp`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsResourceService.cpp`
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

This is the largest task. Move all pool data, init queues, init task structs, mesh resources, and their method implementations from IGraphicsService into GraphicsResourceService.

- [ ] **Step 1: Add member data to GraphicsResourceService.h**

Add the pool data, init queues, mesh resources, and raytracing buffers as protected members. Add the init task structs. Add `SetBackend(IGraphicsService*)` and `m_Backend` pointer. Add protected/private method declarations for `InitializePool`, `TerminatePool`, `InitializeComponents`, `CreateOutputMergerTargets`, `InitializeOutputMergerTargets`, `DeleteRenderTargets`, `AllocateMeshResource`, `ReleaseMeshResource`, `ReleaseAllMeshResources`.

Make `Setup()`, `Initialize()`, `Terminate()` non-pure (they'll manage pool lifecycle). Keep the public API methods as pure virtual — the DX12 wrapper still overrides them to call backend-specific InitializeImpl methods.

Wait — actually, with the pool data IN GraphicsResourceService, the Add* methods can be implemented directly here (they're just pool allocation). Only the DX12-specific `InitializeImpl` calls need the backend.

Change approach: make Add* methods concrete (non-virtual) in GraphicsResourceService since they just do pool allocation. Make Initialize overloads concrete since they just queue work. Keep Delete* as pure virtual since DX12 needs to release DX12-specific resources.

- [ ] **Step 2: Create GraphicsResourceServiceImpl.cpp**

Move from `IGraphicsService.cpp`:
- `Accessibility` static member definitions
- `AllocateGPUHandle` template
- `InitializePool()`, `TerminatePool()`
- All `Add*Component()` methods
- `FindTextureByName()`, `FindMaterialByName()`
- All `Initialize()` overloads (queuing)
- `InitializeImpl(MaterialComponent*)` and `InitializeImpl(RenderPassComponent*)` base implementations
- `InitializeComponents()`
- `CreateOutputMergerTargets()`, `InitializeOutputMergerTargets()`
- `DeleteRenderTargets()`
- `AllocateMeshResource()`, `ReleaseMeshResource()`, `ReleaseAllMeshResources()`, `GetMeshResource()`
- `OnSceneUnloading()`
- `WriteMappedMemory()`

These methods now operate on `this->m_GPUHandlePools` etc. instead of the IGraphicsService's members. For DX12-specific calls (InitializeImpl, Delete, CreatePipelineStateObject, etc.), call through `m_Backend->`.

- [ ] **Step 3: Update DX12GraphicsResourceService**

Remove delegation — DX12GraphicsResourceService inherits from GraphicsResourceService which now has real implementations. Override only the methods that need DX12-specific behavior (the Delete overloads that release DX12 resources).

Actually, DX12GraphicsResourceService can be eliminated entirely if we move the DX12-specific resource operations (Delete, UploadToGPU, etc.) into a backend interface that GraphicsResourceService calls through `m_Backend`.

For now, simplify: DX12GraphicsResourceService becomes a thin concrete class that:
- Inherits GraphicsResourceService (which now owns pools + common logic)
- Overrides `Setup()`/`Initialize()`/`Terminate()` to chain pool init + DX12 init
- Delegates DX12-specific operations to `m_Backend`

- [ ] **Step 4: Remove moved code from IGraphicsService**

Remove pool data, init queues, mesh resources, init task structs, and all moved method declarations/implementations from `IGraphicsService.h` and `IGraphicsService.cpp`.

- [ ] **Step 5: Build and test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Services/GraphicsResourceService.h Source/Engine/Services/Common/GraphicsResourceServiceImpl.cpp Source/Engine/Services/DX12/DX12GraphicsResourceService.h Source/Engine/Services/DX12/DX12GraphicsResourceService.cpp Source/Engine/Services/IGraphicsService.h Source/Engine/Services/Common/IGraphicsService.cpp
git commit -m "refactor: move resource pools and init queues from IGraphicsService to GraphicsResourceService"
```

---

### Task 2: Move frame management data + methods into FrameManagementService

**Files:**
- Modify: `Source/Engine/Services/FrameManagementService.h`
- Create: `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp`
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.h`
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp`
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

- [ ] **Step 1: Add member data to FrameManagementService.h**

Add frame counters, swap chain components, global command lists, semaphore value vectors, callbacks, resize flag, and user pipeline output function as protected members. Add `SetBackend(IGraphicsService*)` and a pointer to `GraphicsResourceService*` (needed for `InitializeComponents()` and `PrepareGlobalCommands()` which interact with resource service state). Add `SetResourceService(GraphicsResourceService*)`.

Make frame query methods concrete (just return member data). Make callback setters concrete (just store the callback). Keep `Present()` virtual for DX12 override needs.

- [ ] **Step 2: Create FrameManagementServiceImpl.cpp**

Move from `IGraphicsService.cpp`:
- Frame queries: `GetCurrentFrame()`, `GetPreviousFrame()`, `GetNextFrame()`, `GetSwapChainImageCount()`, `GetFrameCountSinceLaunch()`
- `SetUploadHeapPreparationCallback()`, `SetCommandPreparationCallback()`, `SetCommandExecutionCallback()`
- `SetUserPipelineOutput()`, `GetUserPipelineOutput()`
- `GetSwapChainRenderPassComponent()`
- `Resize()` — sets `m_needResize` flag
- `Present()` — calls `m_Backend->PresentImpl()`, handles resize drain
- `InitializeSwapChainRenderPassComponent()`
- The `Update()` frame loop — this is the main orchestrator. It calls:
  - `m_Backend->WaitOnCPU()` for frame sync
  - `m_Backend->BeginFrame()`
  - `m_ResourceService->InitializeComponents()` (cross-service call)
  - Callbacks
  - `PrepareGlobalCommands()` / `ExecuteGlobalCommands()` (move these here)
  - `PrepareSwapChainCommands()` / `ExecuteSwapChainCommands()` (move these here)
  - `Present()`
  - `m_Backend->EndFrame()`
  - `m_FrameCountSinceLaunch++`
- `ExecuteResize()`, `PreResize()`, `PostResize()` — resize logic
- `PrepareGlobalCommands()` — uploads GPU buffers, calls `m_Backend->PrepareRayTracing()`
- `ExecuteGlobalCommands()` — executes global command list
- `PrepareSwapChainCommands()` — binds user pipeline output to swap chain pass
- `ExecuteSwapChainCommands()` — executes swap chain command list

Note: `PrepareGlobalCommands()` iterates `m_GPUHandlePools.GPUBufferPointers` — this data is now in GraphicsResourceService. Add an accessor: `GraphicsResourceService::GetGPUBufferPointers()`.

Similarly, `PreResize()`/`PostResize()` iterate `m_GPUHandlePools.RenderPassPointers` — add `GraphicsResourceService::GetRenderPassPointers()`.

- [ ] **Step 3: Update DX12FrameManagementService**

Remove all delegation. DX12FrameManagementService now inherits from FrameManagementService which has real implementations. It only needs to override methods with DX12-specific behavior, which is none — all DX12 specifics go through `m_Backend`. So DX12FrameManagementService can be deleted entirely, replaced by the concrete FrameManagementService with a backend pointer.

- [ ] **Step 4: Remove moved code from IGraphicsService**

Remove frame counters, swap chain data, callbacks, global command lists, and all moved method implementations from IGraphicsService.

- [ ] **Step 5: Build and test** (same commands as Task 1 Step 5)

- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: move frame loop and swap chain management from IGraphicsService to FrameManagementService"
```

---

### Task 3: Move hardware service convenience methods and clean up

**Files:**
- Modify: `Source/Engine/Services/GraphicsHardwareService.h`
- Create: `Source/Engine/Services/Common/GraphicsHardwareServiceImpl.cpp` (if needed — may already exist)
- Modify: `Source/Engine/Services/DX12/DX12GraphicsHardwareService.h`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp`
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

- [ ] **Step 1: Move convenience methods into GraphicsHardwareService**

The `SignalOnGPU(RenderPassComponent*, ...)` and `WaitOnGPU(RenderPassComponent*, ...)` convenience overloads are already in `GraphicsHardwareService.h` as non-virtual methods. Verify they're implemented in a `.cpp` file (not in IGraphicsService.cpp). If they're still in IGraphicsService.cpp, move them.

Also move `CommandListBegin(RenderPassComponent*, ...)`, `CommandListEnd(RenderPassComponent*, ...)`, and `ChangeRenderTargetStates(...)` if they have non-virtual base implementations in IGraphicsService.cpp.

- [ ] **Step 2: Update DX12GraphicsHardwareService**

DX12GraphicsHardwareService delegates low-level sync/command calls to `m_Backend`. This stays as-is — the DX12-specific implementations live in DX12GraphicsService and are called through the backend pointer.

- [ ] **Step 3: Remove moved code from IGraphicsService**

Remove the convenience method implementations.

- [ ] **Step 4: Build and test**

- [ ] **Step 5: Commit**

```bash
git commit -m "refactor: move hardware convenience methods from IGraphicsService to GraphicsHardwareService"
```

---

### Task 4: Thin out IGraphicsService to backend-only interface

**Files:**
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`
- Modify: `Source/Engine/Engine.cpp`

- [ ] **Step 1: Strip IGraphicsService to pure virtual DX12 backend methods only**

IGraphicsService should now only contain:
- Pure virtual DX12-specific methods: `InitializeImpl(*)`, `Delete(*)`, `CreateHardwareResources()`, `ReleaseHardwareResources()`, device/swap chain methods, `BeginFrame()`, `EndFrame()`, `PresentImpl()`, `ResizeImpl()`, command list operations, sync primitives, etc.
- No member data (all moved to focused services)
- No common implementation (all moved)
- `Setup()`, `Initialize()`, `Update()`, `Terminate()` as virtual overrides that DX12GraphicsService still implements for its own DX12-specific lifecycle

`IGraphicsService.cpp` should be nearly empty — just the DX12 backend interface. Most implementation has moved to `GraphicsResourceServiceImpl.cpp` and `FrameManagementServiceImpl.cpp`.

- [ ] **Step 2: Update Engine.cpp**

The focused services are now concrete with real implementations. Update Engine::CreateServices():
- `GraphicsResourceService` gets `SetBackend(m_GraphicsService.get())`
- `FrameManagementService` gets `SetBackend(m_GraphicsService.get())` and `SetResourceService(resourceService)`
- `GraphicsHardwareService` gets `SetBackend(m_GraphicsService.get())`
- The main `m_GraphicsService->Setup()` / `Initialize()` / `Update()` / `Terminate()` lifecycle in Engine now routes through the focused services:
  - `Setup()`: GraphicsResourceService::Setup() (inits pools), then backend->CreateHardwareResources(), then FrameManagementService::Setup() (creates swap chain, command lists)
  - `Update()`: FrameManagementService::Update() (the frame loop)
  - `Terminate()`: FrameManagementService::Terminate(), GraphicsResourceService::Terminate() (destroys pools), backend->ReleaseHardwareResources()

- [ ] **Step 3: Delete delegation wrappers (if fully redundant)**

If DX12GraphicsResourceService, DX12FrameManagementService, and DX12GraphicsHardwareService are now purely delegation with no overrides, they can be replaced with the concrete base classes configured with `SetBackend()`.

Delete files if fully redundant:
- `Source/Engine/Services/DX12/DX12GraphicsResourceService.h/.cpp` — if GraphicsResourceService is now concrete
- `Source/Engine/Services/DX12/DX12FrameManagementService.h/.cpp` — if FrameManagementService is now concrete

Keep DX12GraphicsHardwareService if it still overrides low-level DX12 command operations (it likely does — the pure virtual sync/command methods are DX12-specific).

- [ ] **Step 4: Build and test**

- [ ] **Step 5: Commit**

```bash
git commit -m "refactor: thin IGraphicsService to pure backend interface, remove delegation wrappers"
```

---

### Task 5: Final validation

- [ ] **Step 1: Full build**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

- [ ] **Step 2: GPU validation — draw_instanced**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code 0

- [ ] **Step 3: GPU validation — pixel_readback**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test pixel_readback' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code 0

- [ ] **Step 4: Verify IGraphicsService is thin**

Verify `IGraphicsService.h` has no member data and `IGraphicsService.cpp` has no common implementation — only DX12-specific virtual method stubs.

- [ ] **Step 5: Verify no external IGraphicsService references**

```bash
grep -r "IGraphicsService" Source/ --include="*.cpp" --include="*.h" | grep -v "DX12\|VK\|MT\|Headless\|IGraphicsService.h\|IGraphicsService.cpp\|Engine.h\|Engine.cpp"
```

Should return nothing — only backend implementations and Engine internals reference IGraphicsService.
