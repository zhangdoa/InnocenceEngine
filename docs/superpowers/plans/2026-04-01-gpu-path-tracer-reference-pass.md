# GPU Path Tracer Reference Pass Implementation Plan

> **Path note (TASK-200, 2026-04-29):** All `Res/Shaders/HLSL/*.hlsl` paths in this plan are wrong. Current shader source lives in `Source/Shaders/HLSL/`; the `Res/Shaders/` tree is an obsolete shadow that has been deleted. Translate `Res/Shaders/HLSL/X.hlsl` → `Source/Shaders/HLSL/X.hlsl` when reading this plan.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `GPUPathTracerPass`, a progressive GPU DXR path tracer toggled by B key, that accumulates a ground-truth reference image and validates the probe-based `RadianceCacheRaytracingPass`. Also removes the offline `GIResolvePass`.

**Architecture:** Iterative DXR (no recursion depth > 1): the ray gen shader loops over path bounces. Shadow rays use `RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` + a second miss shader — no shadow hit group needed. Geometry data (normals) is accessed via globally-bound mega vertex/index buffers built on scene load from CPU-accessible upload heap memory (`mesh->m_MappedMemory_VB`). Accumulation buffer stores a running per-pixel HDR average; a tonemap compute shader writes the swap-chain output.

**Tech Stack:** DX12, DXR SM 6.0, HLSL 2021, C++17, engine INNO_CLASS_SINGLETON pattern.

---

## File Map

| File | Action | Purpose |
|------|--------|---------|
| `Source/DefaultClient/RenderingClient/GIResolvePass.h` | **Delete** | Offline GI bake — superseded |
| `Source/DefaultClient/RenderingClient/GIResolvePass.cpp` | **Delete** | Owns B key + scene callbacks |
| `Source/Engine/Component/ShaderProgramComponent.h` | Modify | Add `m_ShadowMissPath` / `m_ShadowMissBuffer` |
| `Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp` | Modify | Load shadow miss shader; change TLAS `InstanceID` EntityID → drawCallIndex |
| `Source/Engine/Services/DX12/DX12GraphicsService_GraphicsDevice_Protected.cpp` | Modify | PSO: add shadow miss DXIL lib, increase payload to 48 B, write 4-slot shader table |
| `Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp` | Modify | `DispatchRays`: handle 4-slot table layout (2 miss shaders) |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h` | **Create** | Pass class declaration |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` | **Create** | Pass C++ implementation |
| `Res/Shaders/HLSL/GPUPathTracerRayGen.hlsl` | **Create** | Iterative Monte Carlo path loop |
| `Res/Shaders/HLSL/GPUPathTracerClosestHit.hlsl` | **Create** | Reads mega vertex buffer, returns surface data |
| `Res/Shaders/HLSL/GPUPathTracerMiss.hlsl` | **Create** | Sky gradient |
| `Res/Shaders/HLSL/GPUPathTracerShadowMiss.hlsl` | **Create** | Sets `isShadowed = false` |
| `Res/Shaders/HLSL/GPUPathTracerToneMap.hlsl` | **Create** | Compute: ACES tonemap of accumulation buffer |
| `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp` | Modify | B key, toggle, Setup/Initialize/Update/Prepare/Execute hooks |

CMakeLists uses `file(GLOB *.cpp)` — no build file changes needed.

---

## Task 1: Delete GIResolvePass

**Files:**
- Delete: `Source/DefaultClient/RenderingClient/GIResolvePass.h`
- Delete: `Source/DefaultClient/RenderingClient/GIResolvePass.cpp`

- [ ] **Step 1: Delete files**

```bash
git rm Source/DefaultClient/RenderingClient/GIResolvePass.h \
       Source/DefaultClient/RenderingClient/GIResolvePass.cpp
```

- [ ] **Step 2: Verify no remaining references**

```bash
grep -r "GIResolvePass" Source/ Res/ --include="*.h" --include="*.cpp" --include="*.hlsl"
```

Expected: no output (GIResolvePass was self-contained via CMake glob, not included anywhere).

- [ ] **Step 3: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
```

Expected: 0 errors.

- [ ] **Step 4: Commit**

```bash
git add -u
git commit -m "feat: remove GIResolvePass offline bake (superseded by GPU path tracer)"
```

---

## Task 2: Add Shadow Miss to ShaderProgramComponent

**Files:**
- Modify: `Source/Engine/Component/ShaderProgramComponent.h`

- [ ] **Step 1: Add `m_ShadowMissPath` to `ShaderFilePaths` and `m_ShadowMissBuffer` to `ShaderProgramComponent`**

In `Source/Engine/Component/ShaderProgramComponent.h`, add after `m_MissPath`:

```cpp
// In struct ShaderFilePaths:
ShaderFilePath m_MissPath       = "";
ShaderFilePath m_ShadowMissPath = "";  // ADD THIS LINE
```

And after `m_MissBuffer`:

```cpp
// In struct ShaderProgramComponent:
std::vector<uint8_t> m_MissBuffer;
std::vector<uint8_t> m_ShadowMissBuffer;  // ADD THIS LINE
```

- [ ] **Step 2: Load shadow miss shader in `DX12GraphicsService_EngineComponent_Protected.cpp`**

There are two shader loading paths in the same file: a `#ifdef USE_DXIL` path and an `#else` path. Add loading for shadow miss to both.

In the `#ifdef USE_DXIL` block (around line 434–437), after the existing Miss block:

```cpp
if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
{
    LoadShaderFile(shaderProgram->m_ShadowMissBuffer, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath);
}
```

In the `#else` block (around line 513–520), after the existing Miss block:

```cpp
if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
{
    if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath))
    {
        shaderProgram->m_ShadowMissBuffer.resize(tempBuffer->GetBufferSize());
        std::memcpy(shaderProgram->m_ShadowMissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
    }
}
```

- [ ] **Step 3: Build to verify no compilation errors**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
```

Expected: 0 errors.

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Component/ShaderProgramComponent.h \
        Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp
git commit -m "feat: add ShadowMiss shader slot to ShaderProgramComponent"
```

---

## Task 3: Extend DXR PSO and DispatchRays for Two Miss Shaders

**Files:**
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_GraphicsDevice_Protected.cpp` (function `CreateRaytracingPipelineStateObject`)
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp` (function `DispatchRays`)

- [ ] **Step 1: Extend `CreateRaytracingPipelineStateObject` in `DX12GraphicsService_GraphicsDevice_Protected.cpp`**

Read the current function (lines ~339–452). Replace it entirely with:

```cpp
bool DX12GraphicsService::CreateRaytracingPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
    auto l_SPC = RenderPassComp->m_ShaderProgram;

    if (!PSO->m_RootSignature)
    {
        Log(Error, RenderPassComp->m_InstanceName, " Global root signature is null!");
        return false;
    }

    LoadRaytracingShaders(RenderPassComp);

    const bool hasShadowMiss = !l_SPC->m_ShadowMissBuffer.empty();

    D3D12_DXIL_LIBRARY_DESC rayGenLib = {};
    rayGenLib.DXILLibrary.pShaderBytecode = &l_SPC->m_RayGenBuffer[0];
    rayGenLib.DXILLibrary.BytecodeLength = l_SPC->m_RayGenBuffer.size();

    D3D12_DXIL_LIBRARY_DESC closestHitLib = {};
    closestHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ClosestHitBuffer[0];
    closestHitLib.DXILLibrary.BytecodeLength = l_SPC->m_ClosestHitBuffer.size();

    D3D12_DXIL_LIBRARY_DESC anyHitLib = {};
    anyHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_AnyHitBuffer[0];
    anyHitLib.DXILLibrary.BytecodeLength = l_SPC->m_AnyHitBuffer.size();

    D3D12_DXIL_LIBRARY_DESC missLib = {};
    missLib.DXILLibrary.pShaderBytecode = &l_SPC->m_MissBuffer[0];
    missLib.DXILLibrary.BytecodeLength = l_SPC->m_MissBuffer.size();

    D3D12_DXIL_LIBRARY_DESC shadowMissLib = {};
    if (hasShadowMiss)
    {
        shadowMissLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ShadowMissBuffer[0];
        shadowMissLib.DXILLibrary.BytecodeLength = l_SPC->m_ShadowMissBuffer.size();
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = L"HitGroup";
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
    hitGroupDesc.AnyHitShaderImport = L"AnyHitShader";
    hitGroupDesc.IntersectionShaderImport = nullptr;

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = 48;  // PathTracerPayload: 44B; ShadowPayload: 4B; round up to 48
    shaderConfig.MaxAttributeSizeInBytes = 8; // barycentrics

    D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { PSO->m_RootSignature.Get() };

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
    pipelineCfg.MaxTraceRecursionDepth = 1;

    // Up to 10 subobjects: 5 DXIL libs + hit group + shader config + global RS + pipeline config + (optional shadow miss)
    D3D12_STATE_SUBOBJECT subobjects[10] = {};
    uint32_t subIdx = 0;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[subIdx++].pDesc = &rayGenLib;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[subIdx++].pDesc = &closestHitLib;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[subIdx++].pDesc = &anyHitLib;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[subIdx++].pDesc = &missLib;

    if (hasShadowMiss)
    {
        subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        subobjects[subIdx++].pDesc = &shadowMissLib;
    }

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    subobjects[subIdx++].pDesc = &hitGroupDesc;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    subobjects[subIdx++].pDesc = &shaderConfig;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    subobjects[subIdx++].pDesc = &globalSig;

    subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    subobjects[subIdx++].pDesc = &pipelineCfg;

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = subIdx;
    stateObjectDesc.pSubobjects = subobjects;

    HRESULT l_HResult = m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&PSO->m_RaytracingPSO));
    if (FAILED(l_HResult))
    {
        Log(Error, RenderPassComp->m_InstanceName, " Can't create Raytracing PSO.");
        return false;
    }

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    SetObjectName(RenderPassComp, PSO->m_RaytracingPSO, "RaytracingPSO");
#endif

    Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing PSO has been created.");

    // Shader table layout:
    //   hasShadowMiss == false: [RayGen][Miss][HitGroup]           (3 slots)
    //   hasShadowMiss == true:  [RayGen][Miss][ShadowMiss][HitGroup] (4 slots)
    const uint32_t numSlots = hasShadowMiss ? 4 : 3;
    auto l_shaderIDBufferSize = numSlots * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    auto l_shaderIDBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(l_shaderIDBufferSize);
    PSO->m_RaytracingShaderIDBuffer = CreateUploadHeapBuffer(&l_shaderIDBufferDesc);

    ID3D12StateObjectProperties* props;
    PSO->m_RaytracingPSO->QueryInterface(&props);

    void* data;
    auto writeId = [&](const wchar_t* name) {
        void* id = props->GetShaderIdentifier(name);
        memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        data = static_cast<char*>(data) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    };

    PSO->m_RaytracingShaderIDBuffer->Map(0, nullptr, &data);
    writeId(L"RayGenShader");
    writeId(L"MissShader");
    if (hasShadowMiss)
        writeId(L"ShadowMissShader");
    writeId(L"HitGroup");
    PSO->m_RaytracingShaderIDBuffer->Unmap(0, nullptr);
    props->Release();

    Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing shader IDs have been written.");

    return true;
}
```

- [ ] **Step 2: Extend `DispatchRays` in `DX12GraphicsService_CommandListAPI.cpp`**

Read the current function (around line 830). Replace the shader table address computation with:

```cpp
bool DX12GraphicsService::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
    if (!renderPass || !commandList)
    {
        Log(Error, "Null parameters in DispatchRays");
        return false;
    }

    auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
    auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(reinterpret_cast<DX12CommandListComponent*>(commandList)->m_CommandList.Get());

    auto l_shaderIDBufferVirtualAddress = l_PSO->m_RaytracingShaderIDBuffer->GetGPUVirtualAddress();

    // Detect layout from shader table buffer size:
    // 3-slot: [RayGen][Miss][HitGroup]
    // 4-slot: [RayGen][Miss][ShadowMiss][HitGroup]
    D3D12_RESOURCE_DESC l_bufDesc = l_PSO->m_RaytracingShaderIDBuffer->GetDesc();
    const bool hasShadowMiss = (l_bufDesc.Width >= 4 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

    dispatchDesc.RayGenerationShaderRecord.StartAddress = l_shaderIDBufferVirtualAddress;
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    dispatchDesc.MissShaderTable.StartAddress = l_shaderIDBufferVirtualAddress + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    dispatchDesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    dispatchDesc.MissShaderTable.SizeInBytes = hasShadowMiss
        ? 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
        : D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    const uint64_t hitGroupOffset = hasShadowMiss
        ? 3 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
        : 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

    dispatchDesc.HitGroupTable.StartAddress = l_shaderIDBufferVirtualAddress + hitGroupOffset;
    dispatchDesc.HitGroupTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    dispatchDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    dispatchDesc.Width = dimensionX;
    dispatchDesc.Height = dimensionY;
    dispatchDesc.Depth = dimensionZ;

    l_commandList->SetPipelineState1(l_PSO->m_RaytracingPSO.Get());
    l_commandList->DispatchRays(&dispatchDesc);

    return true;
}
```

- [ ] **Step 3: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: 0 errors.

- [ ] **Step 4: GPU validation test (existing RadianceCacheRaytracingPass must still pass)**

```powershell
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected exit code: 0.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Services/DX12/DX12GraphicsService_GraphicsDevice_Protected.cpp \
        Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp
git commit -m "feat: extend DXR backend for shadow miss shader (2-miss table layout, 48B payload)"
```

---

## Task 4: Fix TLAS InstanceID (EntityID → drawCallIndex)

**Files:**
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp` (function `InitializeImpl(EntityID)`, lines ~704–749)

- [ ] **Step 1: Change `InstanceID` from `Entity` to sequential position in the instance desc list**

In `InitializeImpl(EntityID Entity)`, change the line:

```cpp
instanceDesc.InstanceID = static_cast<UINT>(Entity);
```

To:

```cpp
instanceDesc.InstanceID = static_cast<UINT>(l_descList->m_Descs.size());
```

This assigns a zero-based sequential index equal to the position this instance will occupy in the list (before push), matching `DrawCallService::GetGPUModelData()` draw-call order.

- [ ] **Step 2: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
```

Expected: 0 errors.

- [ ] **Step 3: GPU validation test**

```powershell
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: 0.  (RadianceCacheClosestHit does not use `InstanceID()`, so this change is safe.)

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp
git commit -m "fix: set TLAS InstanceID to draw-order index instead of EntityID"
```

---

## Task 5: GPUPathTracerPass Header

**Files:**
- Create: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.h`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include "../../Engine/Services/IGraphicsService.h"
#include "../../Engine/Common/Math.h"

using namespace Inno;

class GPUPathTracerPass
{
public:
    INNO_CLASS_SINGLETON(GPUPathTracerPass);

    bool Setup(IServiceConfig* systemConfig = nullptr);
    bool Initialize();
    bool Terminate();
    bool Update();
    bool PrepareCommandList();

    RenderPassComponent* GetRenderPassComp();
    GPUResourceComponent* GetResult();

    void ResetAccumulation();

private:
    ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

    // DXR ray tracing pass
    RenderPassComponent*    m_RayTracingRenderPassComp  = nullptr;
    ShaderProgramComponent* m_RayTracingSPC             = nullptr;
    CommandListComponent*   m_CommandListComp_Graphics  = nullptr;
    CommandListComponent*   m_CommandListComp_Compute   = nullptr;

    // Tonemap compute pass
    RenderPassComponent*    m_ToneMapRenderPassComp     = nullptr;
    ShaderProgramComponent* m_ToneMapSPC                = nullptr;
    CommandListComponent*   m_ToneMapCommandList        = nullptr;

    // Owned GPU resources
    TextureComponent*       m_AccumulationBuffer        = nullptr;  // HDR float4 accumulator
    TextureComponent*       m_ToneMapOutput             = nullptr;  // LDR output → becomes m_Canvas
    GPUBufferComponent*     m_FrameCountCB              = nullptr;  // uint32_t frameCount

    // Geometry mega-buffers (rebuilt on scene load)
    GPUBufferComponent*     m_MegaVertexBuffer          = nullptr;  // ByteAddressBuffer: all {float3 pos, float3 normal}
    GPUBufferComponent*     m_MegaIndexBuffer           = nullptr;  // ByteAddressBuffer: all uint32 indices
    GPUBufferComponent*     m_MeshOffsetBuffer          = nullptr;  // StructuredBuffer<uint2>: {vertexOffset, indexOffset}

    // Camera movement detection
    Mat4        m_PrevViewMatrix = {};
    uint32_t    m_FrameCount     = 1;

    // Scene callbacks
    std::function<void()> f_sceneLoadedCallback;
    std::function<void()> f_sceneUnloadingCallback;

    ShaderStage m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

    void RebuildGeometryBuffers();
};
```

- [ ] **Step 2: Verify the file compiles as part of the project**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
```

Expected: 0 errors (header is only visible when included, nothing includes it yet).

---

## Task 6: GPUPathTracerPass::Setup() and Initialize()

**Files:**
- Create/modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`

- [ ] **Step 1: Create the cpp file with includes and Setup()**

`GPUPathTracerPass.cpp`:

```cpp
#include "GPUPathTracerPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Setup(IServiceConfig*)
{
    auto l_graphicsService = g_Engine->getGraphicsService();
    auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>();

    // ── Ray tracing pass ─────────────────────────────────────────────────────

    m_RayTracingSPC = l_graphicsService->AddShaderProgramComponent("GPUPathTracerPass/RT/");
    m_RayTracingSPC->m_ShaderFilePaths.m_RayGenPath     = "GPUPathTracerRayGen.hlsl/";
    m_RayTracingSPC->m_ShaderFilePaths.m_ClosestHitPath = "GPUPathTracerClosestHit.hlsl/";
    m_RayTracingSPC->m_ShaderFilePaths.m_MissPath       = "GPUPathTracerMiss.hlsl/";
    m_RayTracingSPC->m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl/";

    m_RayTracingRenderPassComp = l_graphicsService->AddRenderPassComponent("GPUPathTracerPass/RT/");

    auto l_desc = l_renderingConfig->GetDefaultRenderPassDesc();
    l_desc.m_GPUEngineType     = GPUEngineType::Compute;
    l_desc.m_RenderTargetCount = 0;
    l_desc.m_UseRaytracing     = true;
    l_desc.m_UseOutputMerger   = false;

    m_RayTracingRenderPassComp->m_RenderPassDesc = l_desc;
    m_RayTracingRenderPassComp->m_ShaderProgram  = m_RayTracingSPC;

    // Resource bindings for the ray tracing pass:
    //   b0  PerFrameCB
    //   b1  FrameCountCB
    //   t0  TLAS
    //   t1  MaterialBuffer     (DrawCallService::GetMaterialBuffer())
    //   t2  MegaVertexBuffer
    //   t3  MegaIndexBuffer
    //   t4  MeshOffsetBuffer
    //   u0  AccumulationBuffer (RW)

    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs.resize(8);

    // b0 - PerFrameCB
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex  = 0;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage      = m_ShaderStage;

    // b1 - FrameCountCB
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex  = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage      = m_ShaderStage;

    // t0 - TLAS
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType         = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex         = 0;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUBufferUsage          = GPUBufferUsage::TLAS;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility    = Accessibility::ReadOnly;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility   = Accessibility::ReadWrite;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage             = m_ShaderStage;

    // t1 - MaterialBuffer
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex  = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage      = m_ShaderStage;

    // t2 - MegaVertexBuffer
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex  = 2;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage      = m_ShaderStage;

    // t3 - MegaIndexBuffer
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex  = 3;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage      = m_ShaderStage;

    // t4 - MeshOffsetBuffer
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType  = GPUResourceType::Buffer;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex  = 4;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage      = m_ShaderStage;

    // u0 - AccumulationBuffer (RW)
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType         = GPUResourceType::Image;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex      = 2;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex         = 0;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage            = TextureUsage::ComputeOnly;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility    = Accessibility::ReadWrite;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility   = Accessibility::ReadWrite;
    m_RayTracingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage             = m_ShaderStage;

    m_CommandListComp_Graphics = l_graphicsService->AddCommandListComponent("GPUPathTracerPass/RT/Graphics/");
    m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

    m_CommandListComp_Compute = l_graphicsService->AddCommandListComponent("GPUPathTracerPass/RT/Compute/");
    m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

    // ── Tonemap pass ─────────────────────────────────────────────────────────

    m_ToneMapSPC = l_graphicsService->AddShaderProgramComponent("GPUPathTracerPass/ToneMap/");
    m_ToneMapSPC->m_ShaderFilePaths.m_CSPath = "GPUPathTracerToneMap.hlsl/";

    m_ToneMapRenderPassComp = l_graphicsService->AddRenderPassComponent("GPUPathTracerPass/ToneMap/");

    auto l_toneDesc = l_renderingConfig->GetDefaultRenderPassDesc();
    l_toneDesc.m_GPUEngineType     = GPUEngineType::Compute;
    l_toneDesc.m_RenderTargetCount = 0;
    l_toneDesc.m_UseOutputMerger   = false;

    m_ToneMapRenderPassComp->m_RenderPassDesc = l_toneDesc;
    m_ToneMapRenderPassComp->m_ShaderProgram  = m_ToneMapSPC;

    // Tonemap bindings:
    //   b0  PerFrameCB
    //   t0  AccumulationBuffer (read)
    //   u0  ToneMapOutput      (write)

    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType  = GPUResourceType::Buffer;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex  = 0;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage      = ShaderStage::Compute;

    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType  = GPUResourceType::Image;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex  = 0;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage     = TextureUsage::ComputeOnly;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage      = ShaderStage::Compute;

    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType         = GPUResourceType::Image;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex      = 2;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex         = 0;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage            = TextureUsage::ComputeOnly;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility    = Accessibility::ReadWrite;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility   = Accessibility::ReadWrite;
    m_ToneMapRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage             = ShaderStage::Compute;

    m_ToneMapCommandList = l_graphicsService->AddCommandListComponent("GPUPathTracerPass/ToneMap/");
    m_ToneMapCommandList->m_Type = GPUEngineType::Compute;

    // ── Scene callbacks ───────────────────────────────────────────────────────

    f_sceneLoadedCallback = [this]() { RebuildGeometryBuffers(); m_FrameCount = 1; };
    g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadedCallback);

    f_sceneUnloadingCallback = [this]() { m_FrameCount = 1; };
    g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneUnloadingCallback);

    m_ObjectStatus = ObjectStatus::Created;
    return true;
}
```

- [ ] **Step 2: Add Initialize() and Terminate()**

Append to `GPUPathTracerPass.cpp`:

```cpp
bool GPUPathTracerPass::Initialize()
{
    auto l_graphicsService = g_Engine->getGraphicsService();
    auto l_renderingCfg    = g_Engine->Get<RenderingConfigurationService>();

    l_graphicsService->Initialize(m_RayTracingSPC);
    l_graphicsService->Initialize(m_RayTracingRenderPassComp);
    l_graphicsService->Initialize(m_CommandListComp_Graphics);
    l_graphicsService->Initialize(m_CommandListComp_Compute);

    l_graphicsService->Initialize(m_ToneMapSPC);
    l_graphicsService->Initialize(m_ToneMapRenderPassComp);
    l_graphicsService->Initialize(m_ToneMapCommandList);

    // Accumulation buffer: float4 HDR at screen resolution
    m_AccumulationBuffer = l_graphicsService->AddTextureComponent("GPUPathTracerPass/AccumBuffer/");
    m_AccumulationBuffer->m_TextureDesc = l_renderingCfg->GetDefaultRenderPassDesc().m_RenderTargetDesc;
    m_AccumulationBuffer->m_TextureDesc.PixelDataFormat     = TexturePixelDataFormat::RGBA;
    m_AccumulationBuffer->m_TextureDesc.PixelDataType       = TexturePixelDataType::Float32;
    m_AccumulationBuffer->m_TextureDesc.Usage               = TextureUsage::ComputeOnly;
    m_AccumulationBuffer->m_TextureDesc.Width  = l_renderingCfg->GetRenderingConfig().renderingServer->m_RenderWidth;
    m_AccumulationBuffer->m_TextureDesc.Height = l_renderingCfg->GetRenderingConfig().renderingServer->m_RenderHeight;
    l_graphicsService->Initialize(m_AccumulationBuffer);

    // ToneMap output: RGBA8 at screen resolution (the display output)
    m_ToneMapOutput = l_graphicsService->AddTextureComponent("GPUPathTracerPass/ToneMapOutput/");
    m_ToneMapOutput->m_TextureDesc = m_AccumulationBuffer->m_TextureDesc;
    m_ToneMapOutput->m_TextureDesc.PixelDataType = TexturePixelDataType::UByte;
    l_graphicsService->Initialize(m_ToneMapOutput);

    // FrameCountCB: single uint32_t
    m_FrameCountCB = l_graphicsService->AddGPUBufferComponent("GPUPathTracerPass/FrameCountCB/");
    m_FrameCountCB->m_ElementCount    = 1;
    m_FrameCountCB->m_ElementSize     = sizeof(uint32_t);
    m_FrameCountCB->m_GPUAccessibility = Accessibility::ReadOnly;
    l_graphicsService->Initialize(m_FrameCountCB);

    m_ObjectStatus = ObjectStatus::Suspended;
    return true;
}

bool GPUPathTracerPass::Terminate()
{
    auto l_graphicsService = g_Engine->getGraphicsService();

    l_graphicsService->Delete(m_ToneMapCommandList);
    l_graphicsService->Delete(m_ToneMapRenderPassComp);
    l_graphicsService->Delete(m_ToneMapSPC);

    l_graphicsService->Delete(m_CommandListComp_Compute);
    l_graphicsService->Delete(m_CommandListComp_Graphics);
    l_graphicsService->Delete(m_RayTracingRenderPassComp);
    l_graphicsService->Delete(m_RayTracingSPC);

    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

RenderPassComponent* GPUPathTracerPass::GetRenderPassComp()
{
    return m_RayTracingRenderPassComp;
}

GPUResourceComponent* GPUPathTracerPass::GetResult()
{
    return m_ToneMapOutput;
}
```

- [ ] **Step 3: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: 0 errors.

---

## Task 7: GPUPathTracerPass::RebuildGeometryBuffers() and Update()

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`

The geometry mega-buffers store compacted vertex data for all draw-call meshes in draw-call order.
`Vertex` (= `TVertex<float>`) layout: `{ Vec3 m_pos; Vec3 m_normal; Vec3 m_tangent; Vec2 m_texCoord; float m_pad1[4]; float m_pad2; }` = 64 bytes.
We only store `{ float3 pos; float3 normal; }` = 24 bytes per vertex in the mega buffer to save bandwidth.

- [ ] **Step 1: Add RebuildGeometryBuffers()**

Append to `GPUPathTracerPass.cpp`:

```cpp
void GPUPathTracerPass::RebuildGeometryBuffers()
{
    auto l_graphicsService = g_Engine->getGraphicsService();
    const auto& l_modelData = g_Engine->Get<DrawCallService>()->GetGPUModelData();
    const uint32_t N = static_cast<uint32_t>(l_modelData.size());

    if (N == 0)
        return;

    // Compact vertex format for path tracer: {float3 pos, float3 normal} = 24 bytes
    struct PTVertex { float px, py, pz, nx, ny, nz; };
    // Each index is uint32_t = 4 bytes

    // Count totals
    uint32_t totalVertices = 0;
    uint32_t totalIndices  = 0;
    for (const auto& md : l_modelData)
    {
        totalVertices += md.m_VertexCount;
        totalIndices  += md.m_IndexCount;
    }

    std::vector<PTVertex>   megaVerts(totalVertices);
    std::vector<uint32_t>   megaIdxs(totalIndices);
    // MeshOffsetBuffer: {uint32_t vertexOffset, uint32_t indexOffset} per instance
    std::vector<uint32_t>   offsets(N * 2);

    uint32_t vBase = 0, iBase = 0;
    for (uint32_t i = 0; i < N; ++i)
    {
        const auto& md = l_modelData[i];

        // Retrieve the MeshComponent owning these buffers via the mesh pointer stored in DrawCallInfo.
        // m_MappedMemory_VB / m_MappedMemory_IB point to persistently-mapped Upload Heap memory.
        // GPUModelData gives us the GPU VA; we look up the mesh from the DrawCallInfo vector instead.
        const auto& drawCallInfo = g_Engine->Get<DrawCallService>()->GetDrawCallInfo();
        MeshComponent* mesh = drawCallInfo[i].mesh;

        offsets[i * 2 + 0] = vBase;
        offsets[i * 2 + 1] = iBase;

        // Copy vertices (strided read from upload heap)
        const uint8_t* vSrc   = static_cast<const uint8_t*>(mesh->m_MappedMemory_VB);
        const uint32_t stride = mesh->m_VertexBufferView.m_StrideInBytes;  // = sizeof(Vertex) = 64
        for (uint32_t v = 0; v < md.m_VertexCount; ++v)
        {
            // Vertex layout: pos(3f) normal(3f) tangent(3f) texCoord(2f) pad1(4f) pad2(1f)
            const float* vf = reinterpret_cast<const float*>(vSrc + v * stride);
            megaVerts[vBase + v] = { vf[0], vf[1], vf[2], vf[3], vf[4], vf[5] };
        }

        // Copy indices
        const uint32_t* iSrc = static_cast<const uint32_t*>(mesh->m_MappedMemory_IB);
        for (uint32_t idx = 0; idx < md.m_IndexCount; ++idx)
            megaIdxs[iBase + idx] = iSrc[idx];

        vBase += md.m_VertexCount;
        iBase += md.m_IndexCount;
    }

    // Release old buffers if they exist
    if (m_MegaVertexBuffer) l_graphicsService->Delete(m_MegaVertexBuffer);
    if (m_MegaIndexBuffer)  l_graphicsService->Delete(m_MegaIndexBuffer);
    if (m_MeshOffsetBuffer) l_graphicsService->Delete(m_MeshOffsetBuffer);

    // Create and upload mega vertex buffer (ByteAddressBuffer)
    m_MegaVertexBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerPass/MegaVB/");
    m_MegaVertexBuffer->m_ElementCount     = totalVertices;
    m_MegaVertexBuffer->m_ElementSize      = sizeof(PTVertex);  // 24
    m_MegaVertexBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
    l_graphicsService->Initialize(m_MegaVertexBuffer);
    l_graphicsService->UpdateGPUBuffer(m_MegaVertexBuffer, megaVerts.data(), megaVerts.size() * sizeof(PTVertex));

    // Create and upload mega index buffer
    m_MegaIndexBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerPass/MegaIB/");
    m_MegaIndexBuffer->m_ElementCount     = totalIndices;
    m_MegaIndexBuffer->m_ElementSize      = sizeof(uint32_t);
    m_MegaIndexBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
    l_graphicsService->Initialize(m_MegaIndexBuffer);
    l_graphicsService->UpdateGPUBuffer(m_MegaIndexBuffer, megaIdxs.data(), megaIdxs.size() * sizeof(uint32_t));

    // Create and upload mesh offset buffer
    m_MeshOffsetBuffer = l_graphicsService->AddGPUBufferComponent("GPUPathTracerPass/MeshOffsets/");
    m_MeshOffsetBuffer->m_ElementCount     = N;
    m_MeshOffsetBuffer->m_ElementSize      = sizeof(uint32_t) * 2;
    m_MeshOffsetBuffer->m_GPUAccessibility = Accessibility::ReadOnly;
    l_graphicsService->Initialize(m_MeshOffsetBuffer);
    l_graphicsService->UpdateGPUBuffer(m_MeshOffsetBuffer, offsets.data(), offsets.size() * sizeof(uint32_t));
}
```

Note: `DrawCallService::GetDrawCallInfo()` may not exist yet — verify its availability. If only `GetGPUModelData()` is public, look up the mesh via `EntityRegistry` using the draw call entity info. Adjust if needed.

- [ ] **Step 2: Add Update()**

Append to `GPUPathTracerPass.cpp`:

```cpp
bool GPUPathTracerPass::Update()
{
    if (m_ObjectStatus != ObjectStatus::Activated && m_ObjectStatus != ObjectStatus::Suspended)
        return false;

    auto l_perFrameService = g_Engine->Get<PerFrameDataService>();
    const Mat4& viewMatrix = l_perFrameService->GetCurrentPerFrameConstantBuffer().v;

    const bool cameraChanged = (viewMatrix != m_PrevViewMatrix);
    m_PrevViewMatrix = viewMatrix;

    if (cameraChanged)
    {
        m_FrameCount = 1;
    }
    else
    {
        ++m_FrameCount;
    }

    g_Engine->getGraphicsService()->UpdateGPUBuffer(m_FrameCountCB, &m_FrameCount, sizeof(uint32_t));

    return true;
}

void GPUPathTracerPass::ResetAccumulation()
{
    m_FrameCount = 1;
}
```

- [ ] **Step 3: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: 0 errors. (If `GetDrawCallInfo()` doesn't exist, read `DrawCallService.h` and adjust to use the available API for accessing mesh pointers.)

---

## Task 8: HLSL Shaders

**Files:**
- Create: `Res/Shaders/HLSL/GPUPathTracerRayGen.hlsl`
- Create: `Res/Shaders/HLSL/GPUPathTracerClosestHit.hlsl`
- Create: `Res/Shaders/HLSL/GPUPathTracerMiss.hlsl`
- Create: `Res/Shaders/HLSL/GPUPathTracerShadowMiss.hlsl`
- Create: `Res/Shaders/HLSL/GPUPathTracerToneMap.hlsl`

Before writing, read `Res/Shaders/HLSL/common/common.hlsl` to understand the `PerFrame_CB` struct layout (especially `sun_direction`, `sun_illuminance`, `v`, `viewportSize`). Read `Res/Shaders/HLSL/RayTracingTypes.hlsl` to see existing shared types.

- [ ] **Step 1: Create GPUPathTracerShadowMiss.hlsl** (simplest, validate pattern)

```hlsl
// shadertype=hlsl

struct ShadowPayload
{
    bool isShadowed;
};

[shader("miss")]
void ShadowMissShader(inout ShadowPayload payload)
{
    payload.isShadowed = false;
}
```

- [ ] **Step 2: Create GPUPathTracerMiss.hlsl**

```hlsl
// shadertype=hlsl
#include "common/common.hlsl"

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

[shader("miss")]
void MissShader(inout PathTracerPayload payload)
{
    payload.missed = true;
}
```

- [ ] **Step 3: Create GPUPathTracerClosestHit.hlsl**

```hlsl
// shadertype=hlsl
#include "common/common.hlsl"

// Mega geometry buffers (global bindings, set index 1)
[[vk::binding(2, 1)]]
ByteAddressBuffer in_MegaVertexBuffer : register(t2);   // PTVertex = {float3 pos, float3 normal}

[[vk::binding(3, 1)]]
ByteAddressBuffer in_MegaIndexBuffer  : register(t3);   // uint32 indices

// MeshOffsetBuffer: StructuredBuffer<uint2> where x=vertexOffset, y=indexOffset
[[vk::binding(4, 1)]]
ByteAddressBuffer in_MeshOffsets      : register(t4);

// MaterialConstantBuffer array (DrawCallService::GetMaterialBuffer())
[[vk::binding(1, 1)]]
StructuredBuffer<MaterialCB> in_MaterialBuffer : register(t1);

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

// MaterialCB matches MaterialConstantBuffer in GPUDataStructure.h
struct MaterialCB
{
    float AlbedoR, AlbedoG, AlbedoB, Alpha;
    float Metallic, Roughness, AO, Thickness;
    uint  TextureIndices[7];
    uint  MaterialType;
    // Note: struct is 16-byte aligned, total ~64B; ensure match with C++ struct.
};

uint3 LoadTriangleIndices(uint baseIndex, uint primitiveIndex)
{
    uint byteOffset = (baseIndex + primitiveIndex * 3) * 4;
    return uint3(
        in_MegaIndexBuffer.Load(byteOffset + 0),
        in_MegaIndexBuffer.Load(byteOffset + 4),
        in_MegaIndexBuffer.Load(byteOffset + 8));
}

float3 LoadVertexNormal(uint baseVertex, uint vertexIndex)
{
    // PTVertex layout: {float3 pos (12B), float3 normal (12B)} = 24B per vertex
    uint byteOffset = (baseVertex + vertexIndex) * 24 + 12;  // skip pos
    float nx = asfloat(in_MegaVertexBuffer.Load(byteOffset + 0));
    float ny = asfloat(in_MegaVertexBuffer.Load(byteOffset + 4));
    float nz = asfloat(in_MegaVertexBuffer.Load(byteOffset + 8));
    return float3(nx, ny, nz);
}

[shader("closesthit")]
void ClosestHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint instanceID = InstanceID();  // = drawCallIndex after Task 4

    // Load mesh offsets for this instance
    uint2 offsets = uint2(
        in_MeshOffsets.Load(instanceID * 8 + 0),   // vertexOffset (bytes: instanceID*2 uints * 4B)
        in_MeshOffsets.Load(instanceID * 8 + 4));   // indexOffset

    // Interpolate normal
    uint3 tri = LoadTriangleIndices(offsets.y, PrimitiveIndex());
    float3 n0 = LoadVertexNormal(offsets.x, tri.x);
    float3 n1 = LoadVertexNormal(offsets.x, tri.y);
    float3 n2 = LoadVertexNormal(offsets.x, tri.z);

    float2 bary = attr.barycentrics;
    float3 localNormal = n0 * (1.0f - bary.x - bary.y) + n1 * bary.x + n2 * bary.y;
    payload.normal = normalize(mul((float3x3)ObjectToWorld3x4(), localNormal));
    payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Material lookup
    MaterialCB mat = in_MaterialBuffer[instanceID];
    payload.albedo    = float3(mat.AlbedoR, mat.AlbedoG, mat.AlbedoB);
    payload.metalness = mat.Metallic;
    payload.roughness = max(mat.Roughness, 0.04f);
    payload.missed    = false;
}
```

- [ ] **Step 4: Create GPUPathTracerRayGen.hlsl**

Read `Res/Shaders/HLSL/common/common.hlsl` first to confirm the `PerFrame_CB` field names (`v`, `p_jittered`, `sun_direction`, `sun_illuminance`, `viewportSize`, `camera_posWS`). Adjust struct/field accesses below if names differ.

```hlsl
// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(1, 0)]]
cbuffer FrameCountCB : register(b1) { uint g_FrameCount; }

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(0, 2)]]
RWTexture2D<float4> AccumBuffer : register(u0);

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

struct ShadowPayload { bool isShadowed; };

// ── PRNG (PCG32) ─────────────────────────────────────────────────────────────

uint PCG(inout uint state)
{
    uint oldState = state;
    state = oldState * 747796405u + 2891336453u;
    uint word = ((oldState >> ((oldState >> 28u) + 4u)) ^ oldState) * 277803737u;
    return (word >> 22u) ^ word;
}

float Rand(inout uint rng) { return float(PCG(rng)) / 4294967296.0f; }

float2 Rand2(inout uint rng)
{
    return float2(Rand(rng), Rand(rng));
}

uint InitRNG(uint2 pixel, uint frame)
{
    return pixel.x + pixel.y * 16384u + frame * 1073741827u;
}

// ── Halton sequence for camera jitter ────────────────────────────────────────

float Halton(uint index, uint base)
{
    float result = 0.0f, f = 1.0f;
    while (index > 0) { f /= float(base); result += f * float(index % base); index /= base; }
    return result;
}

// ── BRDFs ────────────────────────────────────────────────────────────────────

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (3.14159265f * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 CookTorranceGGX(float3 N, float3 V, float3 L, float3 albedo, float metalness, float roughness)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    float3 H  = normalize(V + L);

    float D  = DistributionGGX(N, H, roughness);
    float G  = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 specular = (D * G * F) / max(4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f), 0.001f);
    float3 diffuse  = (1.0f - F) * (1.0f - metalness) * albedo / 3.14159265f;

    return (diffuse + specular) * max(dot(N, L), 0.0f);
}

// ── GGX importance sampling ───────────────────────────────────────────────────

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265f * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Build TBN from N
    float3 up    = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// ── Sky model ────────────────────────────────────────────────────────────────

float3 SkyColor(float3 dir)
{
    return lerp(float3(0.1f, 0.15f, 0.2f), float3(0.5f, 0.7f, 1.0f), saturate(dir.y));
}

// ── Camera ray generation ─────────────────────────────────────────────────────

RayDesc GenerateCameraRay(uint2 pixel, float2 jitter, uint2 resolution)
{
    float2 uv = (float2(pixel) + 0.5f + jitter) / float2(resolution);
    uv.y = 1.0f - uv.y;
    float2 ndc = uv * 2.0f - 1.0f;

    // Unproject from NDC using inverse matrices stored in PerFrame_CB
    // p_inv maps from NDC to view space; v_inv from view to world
    float4 viewPos = mul(g_Frame.p_inv, float4(ndc.x, ndc.y, 1.0f, 1.0f));
    viewPos /= viewPos.w;
    float3 worldPos = mul(g_Frame.v_inv, float4(viewPos.xyz, 0.0f)).xyz;

    RayDesc ray;
    ray.Origin    = g_Frame.camera_posWS.xyz;
    ray.Direction = normalize(worldPos);
    ray.TMin      = 0.001f;
    ray.TMax      = 1e6f;
    return ray;
}

// ── Main ─────────────────────────────────────────────────────────────────────

[shader("raygeneration")]
void RayGenShader()
{
    uint2 pixel     = DispatchRaysIndex().xy;
    uint2 resolution = DispatchRaysDimensions().xy;

    uint rng = InitRNG(pixel, g_FrameCount);

    float2 jitter = float2(Halton(g_FrameCount, 2), Halton(g_FrameCount, 3)) - 0.5f;
    RayDesc ray = GenerateCameraRay(pixel, jitter, resolution);

    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 radiance   = float3(0.0f, 0.0f, 0.0f);

    for (int bounce = 0; bounce <= 40; ++bounce)
    {
        PathTracerPayload payload;
        payload.missed = false;

        TraceRay(SceneAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

        if (payload.missed)
        {
            radiance += throughput * SkyColor(ray.Direction);
            break;
        }

        float3 N  = payload.normal;
        float3 V  = -ray.Direction;
        float3 sunDir = normalize(g_Frame.sun_direction.xyz);

        // NEE: shadow ray toward sun
        ShadowPayload shadowPayload;
        shadowPayload.isShadowed = true;

        RayDesc shadowRay;
        shadowRay.Origin    = payload.hitPos + N * 0.001f;
        shadowRay.Direction = sunDir;
        shadowRay.TMin      = 0.001f;
        shadowRay.TMax      = 1e6f;

        TraceRay(SceneAS,
                 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF,
                 0,    // RayContributionToHitGroupIndex (unused — closest hit skipped)
                 1,
                 1,    // MissShaderIndex = 1 → ShadowMissShader
                 shadowRay, shadowPayload);

        if (!shadowPayload.isShadowed)
        {
            float3 brdf = CookTorranceGGX(N, V, sunDir, payload.albedo, payload.metalness, payload.roughness);
            radiance += throughput * brdf * g_Frame.sun_illuminance.xyz;
        }

        // Sample next bounce direction via GGX importance sampling
        float2 xi = Rand2(rng);
        float3 H  = ImportanceSampleGGX(xi, N, payload.roughness);
        float3 L  = reflect(-V, H);

        float NdotL = dot(N, L);
        if (NdotL <= 0.0f)
            break;

        float3 brdfSample = CookTorranceGGX(N, V, L, payload.albedo, payload.metalness, payload.roughness);
        float  NdotH = max(dot(N, H), 0.0f);
        float  VdotH = max(dot(V, H), 0.0f);
        float  pdf   = DistributionGGX(N, H, payload.roughness) * NdotH / max(4.0f * VdotH, 0.001f);
        throughput *= brdfSample * NdotL / max(pdf, 0.001f);

        // Russian roulette after bounce 3
        if (bounce >= 3)
        {
            float q = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05f, 0.95f);
            if (Rand(rng) > q)
                break;
            throughput /= q;
        }

        ray.Origin    = payload.hitPos + N * 0.001f;
        ray.Direction = L;
        ray.TMin      = 0.001f;
        ray.TMax      = 1e6f;
    }

    // Running average accumulation
    float w = 1.0f / float(g_FrameCount);
    float3 prev = AccumBuffer[pixel].rgb;
    AccumBuffer[pixel] = float4(lerp(prev, radiance, w), 1.0f);
}
```

- [ ] **Step 5: Create GPUPathTracerToneMap.hlsl**

```hlsl
// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(0, 1)]]
Texture2D<float4> in_AccumBuffer : register(t0);

[[vk::binding(0, 2)]]
RWTexture2D<float4> out_ToneMapOutput : register(u0);

float3 ACESFilmic(float3 x)
{
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 LinearToSRGB(float3 linear)
{
    return pow(saturate(linear), 1.0f / 2.2f);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixel = DTid.xy;
    float3 hdr = in_AccumBuffer[pixel].rgb;

    // Simple exposure: use PerFrame ISO/aperture/shutter or a fixed EV
    float exposure = g_Frame.aperture != 0.0f
        ? (g_Frame.ISO / (g_Frame.aperture * g_Frame.aperture * g_Frame.shutterTime * 100.0f))
        : 1.0f;

    float3 exposed    = hdr * exposure;
    float3 tonemapped = ACESFilmic(exposed);
    float3 srgb       = LinearToSRGB(tonemapped);

    out_ToneMapOutput[pixel] = float4(srgb, 1.0f);
}
```

---

## Task 9: GPUPathTracerPass::PrepareCommandList()

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`

- [ ] **Step 1: Add PrepareCommandList()**

Append to `GPUPathTracerPass.cpp`:

```cpp
bool GPUPathTracerPass::PrepareCommandList()
{
    if (!m_MegaVertexBuffer || m_MegaVertexBuffer->m_ObjectStatus != ObjectStatus::Activated)
        return false;

    auto l_graphicsService = g_Engine->getGraphicsService();
    auto l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
    auto l_materialBuf = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();
    auto l_tlas = l_graphicsService->GetTLASBuffer();
    auto l_renderingCfg = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();

    // ── Graphics CL: resource transitions ─────────────────────────────────
    l_graphicsService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Graphics, 0);
    l_graphicsService->TryToTransitState(m_AccumulationBuffer, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
    l_graphicsService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Graphics);

    // ── Compute CL: ray tracing dispatch ──────────────────────────────────
    l_graphicsService->CommandListBegin(m_RayTracingRenderPassComp, m_CommandListComp_Compute, 0);
    l_graphicsService->BindRenderPassComponent(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_perFrameCB,       0);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_FrameCountCB,     1);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_tlas,             2);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_materialBuf,      3);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaVertexBuffer, 4);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MegaIndexBuffer,  5);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_MeshOffsetBuffer, 6);
    l_graphicsService->BindGPUResource(m_RayTracingRenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_AccumulationBuffer, 7);

    const uint32_t W = l_renderingCfg.renderingServer->m_RenderWidth;
    const uint32_t H = l_renderingCfg.renderingServer->m_RenderHeight;
    l_graphicsService->DispatchRays(m_RayTracingRenderPassComp, m_CommandListComp_Compute, W, H, 1);
    l_graphicsService->CommandListEnd(m_RayTracingRenderPassComp, m_CommandListComp_Compute);

    // ── Tonemap CL: transition accum to read, dispatch tonemap ────────────
    l_graphicsService->CommandListBegin(m_ToneMapRenderPassComp, m_ToneMapCommandList, 0);
    l_graphicsService->TryToTransitState(m_AccumulationBuffer, m_ToneMapCommandList, Accessibility::ReadWrite, Accessibility::ReadOnly);
    l_graphicsService->TryToTransitState(m_ToneMapOutput,      m_ToneMapCommandList, Accessibility::ReadOnly,  Accessibility::ReadWrite);
    l_graphicsService->BindRenderPassComponent(m_ToneMapRenderPassComp, m_ToneMapCommandList);
    l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, l_perFrameCB,      0);
    l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, m_AccumulationBuffer, 1);
    l_graphicsService->BindGPUResource(m_ToneMapRenderPassComp, m_ToneMapCommandList, ShaderStage::Compute, m_ToneMapOutput,   2);
    l_graphicsService->Dispatch(m_ToneMapRenderPassComp, m_ToneMapCommandList, (W + 7) / 8, (H + 7) / 8, 1);
    l_graphicsService->CommandListEnd(m_ToneMapRenderPassComp, m_ToneMapCommandList);

    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}
```

- [ ] **Step 2: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: 0 errors. Fix any missing API names by reading the actual IGraphicsService API (`Source/Engine/Services/IGraphicsService.h`).

---

## Task 10: DefaultRenderingClient Integration

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp`

- [ ] **Step 1: Add include and member variables**

At the top of `DefaultRenderingClient.cpp`, after the existing includes, add:

```cpp
#include "GPUPathTracerPass.h"
```

In `DefaultRenderingClientImpl` (the private class), add after the existing booleans:

```cpp
bool m_GPUPathTracerActive = false;
std::function<void()> f_toggleGPUPathTracer;
```

- [ ] **Step 2: Register B key in Setup()**

In `DefaultRenderingClientImpl::Setup()`, after the existing key bindings (around line 113), add:

```cpp
f_toggleGPUPathTracer = [&]() {
    m_GPUPathTracerActive = !m_GPUPathTracerActive;
    if (m_GPUPathTracerActive)
        GPUPathTracerPass::Get().ResetAccumulation();
};
g_Engine->Get<HIDService>()->AddButtonStateCallback(
    ButtonState{ INNO_KEY_B, true },
    ButtonEvent{ EventLifeTime::OneShot, &f_toggleGPUPathTracer });
```

After the existing `FinalBlendPass::Get().Setup();` call, add:

```cpp
GPUPathTracerPass::Get().Setup();
```

- [ ] **Step 3: Add to Initialize()**

In `DefaultRenderingClientImpl::Initialize()`, after `FinalBlendPass::Get().Initialize();`:

```cpp
GPUPathTracerPass::Get().Initialize();
```

- [ ] **Step 4: Add to Update()**

In `DefaultRenderingClientImpl::Update()`, add:

```cpp
if (m_GPUPathTracerActive)
    GPUPathTracerPass::Get().Update();
```

- [ ] **Step 5: Add to PrepareCommands()**

In `DefaultRenderingClientImpl::PrepareCommands()`, wrap the canvas assignment and replace:

```cpp
m_Canvas = FinalBlendPass::Get().GetResult();
m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();
```

With:

```cpp
if (m_GPUPathTracerActive)
{
    GPUPathTracerPass::Get().PrepareCommandList();
    m_Canvas = GPUPathTracerPass::Get().GetResult();
    m_CanvasOwner = GPUPathTracerPass::Get().GetRenderPassComp();
}
else
{
    m_Canvas = FinalBlendPass::Get().GetResult();
    m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();
    // ... existing PrepareCommands passes ...
}
```

Note: the existing pass PrepareCommandList calls that follow should only execute when NOT in GPU path tracer mode. Wrap the remaining pass calls (`SunShadowCullingPass`, `OpaquePass`, etc.) in an `else` block or an `if (!m_GPUPathTracerActive)` guard.

- [ ] **Step 6: Add to ExecuteCommands()**

In `DefaultRenderingClientImpl::ExecuteCommands()`, find the final blend pass execution and surround:

```cpp
if (m_GPUPathTracerActive)
{
    if (GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
    {
        // Graphics CL: transition accumulation buffer to UAV
        auto l_graphicsCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Graphics);
        l_graphicsService->Execute(l_graphicsCL, GPUEngineType::Graphics);
        auto l_renderPass = GPUPathTracerPass::Get().GetRenderPassComp();
        l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
        l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

        // Compute CL: ray tracing dispatch
        auto l_computeCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Compute);
        l_graphicsService->Execute(l_computeCL, GPUEngineType::Compute);
        l_graphicsService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);

        // Tonemap CL
        l_graphicsService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Compute);
        auto l_toneMapCL = GPUPathTracerPass::Get().GetToneMapCommandList();
        l_graphicsService->Execute(l_toneMapCL, GPUEngineType::Compute);
    }
}
else
{
    // ... existing full pipeline execution ...
}
```

Also add `GetCommandListComp` and `GetToneMapCommandList` to `GPUPathTracerPass` public interface:
```cpp
// In GPUPathTracerPass.h:
CommandListComponent* GetCommandListComp(GPUEngineType type);
CommandListComponent* GetToneMapCommandList();

// In GPUPathTracerPass.cpp:
CommandListComponent* GPUPathTracerPass::GetCommandListComp(GPUEngineType type)
{
    return type == GPUEngineType::Graphics ? m_CommandListComp_Graphics : m_CommandListComp_Compute;
}
CommandListComponent* GPUPathTracerPass::GetToneMapCommandList()
{
    return m_ToneMapCommandList;
}
```

- [ ] **Step 7: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: 0 errors.

- [ ] **Step 8: Commit**

```bash
git add Source/DefaultClient/RenderingClient/GPUPathTracerPass.h \
        Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp \
        Source/DefaultClient/RenderingClient/DefaultRenderingClient.cpp
git commit -m "feat: add GPUPathTracerPass with B key toggle and DefaultRenderingClient integration"
```

---

## Task 11: Compile Shaders and GPU Validation

**Files:**
- New: 5 HLSL files in `Res/Shaders/HLSL/`

- [ ] **Step 1: Compile shaders**

```powershell
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"
```

Fix any HLSL compile errors (most common issues: undefined struct fields, wrong register bindings, missing includes). Refer to existing `RadianceCacheRayGen.hlsl` / `RadianceCacheMiss.hlsl` for the exact `PerFrame_CB` field names if `common.hlsl` field names differ from those used in the shaders above.

- [ ] **Step 2: Build**

```powershell
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
```

Expected: 0 errors.

- [ ] **Step 3: GPU validation test**

```powershell
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: 0. (The test runs with GPU path tracer OFF; existing pipeline must be unaffected.)

- [ ] **Step 4: Commit shaders**

```bash
git add Res/Shaders/HLSL/GPUPathTracerRayGen.hlsl \
        Res/Shaders/HLSL/GPUPathTracerClosestHit.hlsl \
        Res/Shaders/HLSL/GPUPathTracerMiss.hlsl \
        Res/Shaders/HLSL/GPUPathTracerShadowMiss.hlsl \
        Res/Shaders/HLSL/GPUPathTracerToneMap.hlsl
git commit -m "feat: add GPU path tracer HLSL shaders (ray gen, closest hit, miss, shadow miss, tonemap)"
```

---

## Task 12: Manual Validation

- [ ] **Step 1: Launch the engine normally (non-offscreen)**

```bash
cd C:/GitRepo/InnocenceEngine/Bin
RelWithDebInfo/DefaultClient.exe
```

Load `UnitTest.InnoScene` if it doesn't load automatically.

- [ ] **Step 2: Press B to activate GPU path tracer**

Expected: screen switches to a progressive path-traced view. After ~32 frames, lighting structure, shadow shapes, and bounce colour should converge toward the CPU reference (N key).

- [ ] **Step 3: Press B again**

Expected: normal radiance cache pipeline resumes. No artifacts or validation errors in the output log.

- [ ] **Step 4: Move camera, then press B**

Expected: accumulation resets to frame 1 immediately after toggle (path tracer mode starts clean).

- [ ] **Step 5: Final commit**

```bash
git add -u
git commit -m "feat: GPU progressive path tracer reference pass (B key toggle)"
```

---

## Spec Coverage Checklist

| Spec requirement | Implemented in |
|---|---|
| GPUPathTracerPass C++ class | Task 5–9 |
| B key toggles pass | Task 10 |
| Accumulation buffer (running average) | Task 6, RayGen shader |
| Camera movement detection + reset | Task 7 |
| 40-bounce iterative DXR (no recursion > 1) | RayGen shader |
| NEE sun lighting | RayGen shader |
| GGX BRDF + importance sampling | RayGen shader |
| Russian roulette after bounce 3 | RayGen shader |
| Shadow rays via RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RayGen shader |
| Shadow miss shader (isShadowed = false) | Task 8 step 1 |
| Sky gradient matching CPU tracer | Miss + RayGen SkyColor() |
| Vertex normal interpolation from mega buffer | ClosestHit shader |
| Material lookup from DrawCallService buffer | ClosestHit shader |
| TLAS InstanceID = drawCallIndex | Task 4 |
| ACES tonemap + exposure | ToneMap shader |
| Scene-reload callback rebuilds geometry | Task 7 RebuildGeometryBuffers() |
| GIResolvePass removed | Task 1 |
| B key freed (GIResolvePass deleted) | Task 1 |
| DXR backend: second miss shader | Task 2–3 |
| DXR backend: payload ≥ 44 bytes | Task 3 (48B) |
