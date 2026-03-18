# RenderingContextService Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the 29K-line `RenderingContextService` into 6 focused `ISystem` services, each owning one GPU data domain.

**Architecture:** Each new service is a standalone `ISystem` with Setup/Initialize/Update/Terminate. It reads component data directly via `ComponentManager`, packs into CPU vectors, and uploads to its own GPU buffers. No cross-service reads in Update. Render passes call the specific owning service instead of the old dispatch enum. The old service is deleted once all 6 are extracted.

**Tech Stack:** C++17, DirectX 12 via `IRenderingServer`, engine `ISystem` lifecycle, `INNO_CLASS_CONCRETE_NON_COPYABLE` macro, `g_Engine->Get<T>()` for service access.

---

## Reference

**Spec:** `docs/superpowers/specs/2026-03-18-rendering-context-service-split-design.md`

**Build command:**
```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```

**GPU gate (must pass after every task):**
```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```
Expected: `0`

**Code standards:** `Documents\code-standards.md` — read before any code change.
**Commit policy:** `Documents\commit-message-policy.md` — read before any commit.

**Engine registration pattern** — every new service requires **three** changes in `Engine.cpp`:

1. **Pre-registration** (~line 366): `Get<RenderingContextService>()` is called bare to register the type before Setup. Add `Get<NewService>();` on the line before it.
2. **Setup/Init/Term macros**: `SystemSetup`, `SystemInit`, `SystemTerm` — add BEFORE RCS in Setup/Init, AFTER RCS in Term (reverse order).
3. **Update call** (~line 508): `Get<RenderingContextService>()->Update()` is called directly (not via macro). Add `Get<NewService>()->Update();` on the line BEFORE it.

```cpp
// Pre-registration block (~line 366):
Get<NewService>();          // ← add this
Get<RenderingContextService>();

// Setup phase (~line 542):
SystemSetup(NewService);    // ← add BEFORE RCS
SystemSetup(RenderingContextService);

// Init phase (~line 612):
SystemInit(NewService);     // ← add BEFORE RCS
SystemInit(RenderingContextService);

// Update callback (~line 508):
Get<NewService>()->Update(); // ← add BEFORE RCS
Get<RenderingContextService>()->Update();

// Term phase (~line 707):
SystemTerm(RenderingContextService);
SystemTerm(NewService);     // ← add AFTER RCS (reverse order)
```

`SystemSetup`, `SystemInit`, `SystemTerm` are macros defined at the top of `Engine.cpp`.

**ISystem required includes:**
```cpp
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"   // GPUBufferComponent
#include "../Common/GPUDataStructure.h"            // PerFrameConstantBuffer, GPUModelData, etc.
```

---

## File Map

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `Source/Engine/Services/PerFrameDataService.h` | Camera/jitter/sun per-frame CB + ping-pong buffers |
| Create | `Source/Engine/Services/PerFrameDataService.cpp` | |
| Create | `Source/Engine/Services/LightDataService.h` | Point/sphere light CBs + CSM + GI buffers |
| Create | `Source/Engine/Services/LightDataService.cpp` | |
| Create | `Source/Engine/Services/DrawCallService.h` | Model/transform/material packing + GPU buffers |
| Create | `Source/Engine/Services/DrawCallService.cpp` | |
| Create | `Source/Engine/Services/AnimationDrawCallService.h` | Animation draw call assembly + buffer |
| Create | `Source/Engine/Services/AnimationDrawCallService.cpp` | |
| Create | `Source/Engine/Services/BillboardDrawCallService.h` | Editor icon billboard draw calls + buffer |
| Create | `Source/Engine/Services/BillboardDrawCallService.cpp` | |
| Create | `Source/Engine/Services/DebugDrawCallService.h` | Frame-scoped debug geometry list |
| Create | `Source/Engine/Services/DebugDrawCallService.cpp` | |
| Modify | `Source/Engine/Engine.cpp` | Register 6 new services; deregister old |
| Modify | `Source/Engine/Services/RenderingContextService.h` | Remove migrated members/enums progressively |
| Modify | `Source/Engine/Services/RenderingContextService.cpp` | Remove migrated code progressively |
| Modify | ~40 render pass / tool / engine files | Replace `GetGPUBufferComponent(GPUBufferUsageType::X)` with typed getters |
| Delete | `Source/Engine/Services/RenderingContextService.h` | Task 7 |
| Delete | `Source/Engine/Services/RenderingContextService.cpp` | Task 7 |

---

## Task 1: Extract PerFrameDataService

**Callers that will be updated (use `GPUBufferUsageType::PerFrame` or `::PerFramePrev` or `GetPerFrameConstantBuffer()`):**
- `DefaultClient/RenderingClient/`: BillboardPass, FinalBlendPass, BSDFTestPass, LightCullingPass, LuminanceHistogramPass, GIResolvePass, LightPass, LuminanceAveragePass, SkyPass, RadianceCacheFilterHorizontalPass, SunShadowBlurEvenPass, **MotionBlurPass**, TAAPass, SSAOPass, SunShadowBlurOddPass, VXGIGeometryProcessPass, VolumetricPass, SurfelGITestPass, VXGILightPass, RadianceCacheIntegrationPass, TiledFrustumGenerationPass, TransparentBlendPass, RadianceCacheFilterVerticalPass, RadianceCacheRaytracingPass, TransparentGeometryProcessPass, RadianceCacheReprojectionPass, VXGIVisualizationPass, VXGIScreenSpaceFeedbackPass, VXGIRenderer, SunShadowGeometryProcessPass, OpaquePass, OpaqueCullingPass, SunShadowCullingPass
- `Engine/RenderingServer/DX12/DX12RenderingServer_CommandListAPI.cpp` (model count read)

**Migration pattern:**
```cpp
// Before:
#include "../../Engine/Services/RenderingContextService.h"
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFramePrev)
g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer()

// After:
#include "../../Engine/Services/PerFrameDataService.h"
g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer()
g_Engine->Get<PerFrameDataService>()->GetPreviousFrameBuffer()
g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer()
```

For `DX12RenderingServer_CommandListAPI.cpp` (~line 480), which reads model count via `RenderingContextService`:
```cpp
// Before:
auto l_renderingContextService = g_Engine->Get<RenderingContextService>();
// uses l_renderingContextService->GetGPUModelData()

// After:
#include "../../Services/DrawCallService.h"
auto l_modelCount = (uint32_t)g_Engine->Get<DrawCallService>()->GetGPUModelData().size();
```
Note: `DrawCallService` doesn't exist yet in Task 1. Defer the `DX12RenderingServer_CommandListAPI.cpp` model count update to Task 3 — keep the old `RenderingContextService` include there until then.

**modelCount stub for Tasks 1–2:** `UpdatePerFrameConstantBuffer()` in the original code sets `l_perFrameCB.modelCount = (uint32_t)m_gpuModelDataVector.size()`. When extracted to `PerFrameDataService`, `m_gpuModelDataVector` does not exist. Set `modelCount = 0` as a temporary stub in the extracted code. This produces no GPU validation errors (shaders guard on it), but culling passes will skip all meshes until Task 3 reconnects it. The GPU gate test (`draw_instanced`) is not affected because it uses an empty root signature with no culling. Task 3 will fix this.

**Files:**
- Create: `Source/Engine/Services/PerFrameDataService.h`
- Create: `Source/Engine/Services/PerFrameDataService.cpp`
- Modify: `Source/Engine/Engine.cpp`
- Modify: `Source/Engine/Services/RenderingContextService.h` (remove PerFrame buffers + enum values)
- Modify: `Source/Engine/Services/RenderingContextService.cpp` (remove UpdatePerFrameConstantBuffer + PerFrame buffer setup/init/term, but keep the CSM CB vector build — that moves to LightDataService in Task 2)
- Modify: all caller files listed above

- [ ] **Step 1.1: Create `PerFrameDataService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct PerFrameDataServiceImpl;
    class PerFrameDataService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(PerFrameDataService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
        GPUBufferComponent* GetCurrentFrameBuffer();
        GPUBufferComponent* GetPreviousFrameBuffer();

    private:
        PerFrameDataServiceImpl* m_Impl;
    };
}
```

- [ ] **Step 1.2: Create `PerFrameDataService.cpp`**

Copy `RadicalInverse`, `GetCurrentFramePerFrameBuffer`, `GetPreviousFramePerFrameBuffer`, and `UpdatePerFrameConstantBuffer` from `RenderingContextService.cpp`. Remove the CSM CB vector build (lines 322–338 in the original — the `m_CSMCBVector.clear()` and the for-loop that populates it). That block moves to `LightDataService` in Task 2.

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<PerFrameConstantBuffer> m_perFrameCBs`
- `GPUBufferComponent* m_PerFrameCBufferGPUBufferComp`
- `GPUBufferComponent* m_PerFrameCBufferPrevGPUBufferComp`

Setup: allocate both GPU buffer components (`"PerFrameCBuffer/"`, `"PerFrameCBufferPrev/"`).
Initialize: set element count/size, call `l_renderingServer->Initialize()` for both. Resize `m_perFrameCBs` to swap chain count.
Update: call `UpdatePerFrameConstantBuffer()`, then `l_renderingServer->Upload(GetCurrentFramePerFrameBuffer(), &m_perFrameCBs[currentFrame])`.
Terminate: `l_renderingServer->Delete()` both buffers.

Required includes in the .cpp:
```cpp
#include "PerFrameDataService.h"
#include "../Common/LogService.h"
#include "CameraSystem.h"            // for ICameraSystem / GetActiveCamera
#include "ComponentManager.h"        // for Get<LightComponent>(0)
#include "RenderingConfigurationService.h"
#include "../Engine.h"
```

- [ ] **Step 1.3: Register `PerFrameDataService` in `Engine.cpp`**

Add include at top of Engine.cpp:
```cpp
#include "Services/PerFrameDataService.h"
```

Pre-registration block (~line 366) — add BEFORE `Get<RenderingContextService>()`:
```cpp
Get<PerFrameDataService>();
```

Non-headless Setup block — BEFORE `SystemSetup(RenderingContextService)`:
```cpp
SystemSetup(PerFrameDataService);
```

Non-headless Init block — BEFORE `SystemInit(RenderingContextService)`:
```cpp
SystemInit(PerFrameDataService);
```

Update callback block (~line 508) — BEFORE `Get<RenderingContextService>()->Update()`:
```cpp
Get<PerFrameDataService>()->Update();
```

Terminate block — AFTER `SystemTerm(RenderingContextService)`:
```cpp
SystemTerm(PerFrameDataService);
```

- [ ] **Step 1.4: Update all callers**

For each file in the caller list above:
1. Add `#include "../../Engine/Services/PerFrameDataService.h"` (adjust relative path to match the file's location)
2. Replace `g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame)` → `g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer()`
3. Replace `g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFramePrev)` → `g_Engine->Get<PerFrameDataService>()->GetPreviousFrameBuffer()`
4. Replace `g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer()` → `g_Engine->Get<PerFrameDataService>()->GetPerFrameConstantBuffer()`
5. Remove the `RenderingContextService` include ONLY if that file no longer uses any other `RenderingContextService` API (many files use multiple buffer types — check before removing)

- [ ] **Step 1.5: Remove extracted code from `RenderingContextService`**

From `RenderingContextService.h`:
- Remove `GPUBufferUsageType::PerFrame` and `GPUBufferUsageType::PerFramePrev` from the enum
- Remove `GetPerFrameConstantBuffer()` declaration

From `RenderingContextService.cpp`:
- Remove `m_PerFrameCBufferGPUBufferComp`, `m_PerFrameCBufferPrevGPUBufferComp` from the impl struct
- Remove `m_perFrameCBs` from the impl struct
- Remove `GetCurrentFramePerFrameBuffer()`, `GetPreviousFramePerFrameBuffer()` methods
- Remove `UpdatePerFrameConstantBuffer()` (keep the CSM vector build — it stays until Task 2)
- Remove the call to `UpdatePerFrameConstantBuffer()` from `Update()`
- Remove PerFrame buffer setup/init/term lines
- Remove `GetPerFrameConstantBuffer()` public method
- Update `GetGPUBufferComponent()` switch: remove `PerFrame` and `PerFramePrev` cases

- [ ] **Step 1.6: Build**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1
```
Expected: 0 errors, 0 warnings about undefined symbols. Fix any errors before continuing.

- [ ] **Step 1.7: GPU gate**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```
Expected: `0`. If not 0, stop and debug before continuing.

- [ ] **Step 1.8: Commit**

```bash
git add Source/Engine/Services/PerFrameDataService.h \
        Source/Engine/Services/PerFrameDataService.cpp \
        Source/Engine/Engine.cpp \
        Source/Engine/Services/RenderingContextService.h \
        Source/Engine/Services/RenderingContextService.cpp \
        Source/DefaultClient/RenderingClient/
git commit -m "refactor: extract PerFrameDataService from RenderingContextService"
```

Note: `DX12RenderingServer_CommandListAPI.cpp` is NOT staged here — its model count update is deferred to Task 3.

---

## Task 2: Extract LightDataService

**Callers that will be updated:**
- `LightPass.cpp`: `GPUBufferUsageType::PointLight`, `::SphereLight`, `::CSM`
- `LightCullingPass.cpp`: `GPUBufferUsageType::PointLight`
- `VolumetricPass.cpp`: `GPUBufferUsageType::PointLight`, `::CSM`
- `GIResolvePass.cpp`: `GPUBufferUsageType::CSM`, `::GI`
- `SunShadowGeometryProcessPass.cpp`: `GPUBufferUsageType::CSM` (also uses GPUModelData/Transform — those migrate in Task 3)
- `SurfelGenerator.cpp`: `GPUBufferUsageType::GI`
- `ProbeGenerator.cpp`: `GPUBufferUsageType::GI`
- `BrickGenerator.cpp`: `GPUBufferUsageType::GI`
- `SurfelGITestPass.cpp`: `GPUBufferUsageType::GI`

**Migration pattern:**
```cpp
// Before:
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PointLight)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::SphereLight)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::CSM)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI)

// After:
g_Engine->Get<LightDataService>()->GetPointLightBuffer()
g_Engine->Get<LightDataService>()->GetSphereLightBuffer()
g_Engine->Get<LightDataService>()->GetCSMBuffer()
g_Engine->Get<LightDataService>()->GetGIBuffer()
```

**Files:**
- Create: `Source/Engine/Services/LightDataService.h`
- Create: `Source/Engine/Services/LightDataService.cpp`
- Modify: `Source/Engine/Engine.cpp`
- Modify: `Source/Engine/Services/RenderingContextService.h`
- Modify: `Source/Engine/Services/RenderingContextService.cpp`
- Modify: all caller files listed above

- [ ] **Step 2.1: Create `LightDataService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"

namespace Inno
{
    struct LightDataServiceImpl;
    class LightDataService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(LightDataService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        GPUBufferComponent* GetPointLightBuffer();
        GPUBufferComponent* GetSphereLightBuffer();
        GPUBufferComponent* GetCSMBuffer();
        GPUBufferComponent* GetGIBuffer();

    private:
        LightDataServiceImpl* m_Impl;
    };
}
```

- [ ] **Step 2.2: Create `LightDataService.cpp`**

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<PointLightConstantBuffer> m_pointLightCBVector`
- `std::vector<SphereLightConstantBuffer> m_sphereLightCBVector`
- `std::vector<CSMConstantBuffer> m_CSMCBVector`
- `GPUBufferComponent* m_PointLightGPUBufferComp`
- `GPUBufferComponent* m_SphereLightGPUBufferComp`
- `GPUBufferComponent* m_CSMGPUBufferComp`
- `GPUBufferComponent* m_GICBufferGPUBufferComp`

Copy `UpdateLightData()` from `RenderingContextService.cpp` (point/sphere light packing).

Add CSM packing here — this is the CSM vector build that was in `UpdatePerFrameConstantBuffer()` in the original code (and was NOT yet removed in Task 1 cleanup). Move it to a new method `UpdateCSMData()`:
```cpp
bool LightDataServiceImpl::UpdateCSMData()
{
    m_CSMCBVector.clear();
    auto l_sun = g_Engine->Get<ComponentManager>()->Get<LightComponent>(0);
    if (l_sun == nullptr)
        return false;

    auto& l_LitRegion = l_sun->m_LitRegion_WorldSpace;
    auto& l_ViewMatrices = l_sun->m_ViewMatrices;
    auto& l_ProjectionMatrices = l_sun->m_ProjectionMatrices;

    if (l_LitRegion.size() > 0 && l_ViewMatrices.size() > 0 && l_ProjectionMatrices.size() > 0)
    {
        for (size_t j = 0; j < l_LitRegion.size(); j++)
        {
            CSMConstantBuffer l_CSMCB;
            l_CSMCB.p = l_ProjectionMatrices[j];
            l_CSMCB.v = l_ViewMatrices[j];
            l_CSMCB.AABBMax = l_LitRegion[j].m_boundMax;
            l_CSMCB.AABBMin = l_LitRegion[j].m_boundMin;
            m_CSMCBVector.emplace_back(l_CSMCB);
        }
    }
    return true;
}
```

Update(): call `UpdateLightData()`, `UpdateCSMData()`, then upload all four buffers (guard each upload with `.size() > 0` check). GICBuffer is a stub — do not upload (size is always 1 but no data is written).

Required includes in the .cpp:
```cpp
#include "LightDataService.h"
#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Engine.h"
```

- [ ] **Step 2.3: Register `LightDataService` in `Engine.cpp`**

Same pattern as Task 1.3 — add `#include "Services/LightDataService.h"`, then add `Get<LightDataService>();` (pre-registration), `SystemSetup/Init/Term(LightDataService)`, and `Get<LightDataService>()->Update();` before the RenderingContextService counterparts in each block.

- [ ] **Step 2.4: Update all callers** (same pattern as Task 1.4)

- [ ] **Step 2.5: Remove extracted code from `RenderingContextService`**

Remove: `m_pointLightCBVector`, `m_sphereLightCBVector`, `m_CSMCBVector`, four GPU buffer component members, `UpdateLightData()`, CSM vector build (now finally removed from `UpdatePerFrameConstantBuffer`'s remnant), upload calls for these buffers in `UploadGPUBuffers()`, setup/init/term lines for these buffers, and the corresponding `GetGPUBufferComponent()` switch cases and enum values.

- [ ] **Step 2.6: Build → GPU gate → Commit**

```bash
git commit -m "refactor: extract LightDataService from RenderingContextService"
```

---

## Task 3: Extract DrawCallService

**Callers that will be updated:**
- `OpaquePass.cpp`: `GetGPUModelData()`, `GPUBufferUsageType::GPUModelData`, `::Transform`, `::TransformPrev`, `::Material`
- `OpaqueCullingPass.cpp`: `GetGPUModelData()`, `::GPUModelData`, `::Transform`
- `SunShadowCullingPass.cpp`: same pattern
- `SunShadowGeometryProcessPass.cpp`: same pattern
- `AnimationPass.cpp`: `::GPUModelData`, `::Material`
- `BSDFTestPass.cpp`: `::GPUModelData`, `::Material`
- `TransparentGeometryProcessPass.cpp`: `::GPUModelData`, `::Material`
- `VolumetricPass.cpp`: `::GPUModelData`, `::Material`, `::Transform`
- `VXGIGeometryProcessPass.cpp`: `::GPUModelData`, `::Material`
- `SurfelGenerator.cpp`: `::Transform`, `::Material`
- `ProbeGenerator.cpp`: `::Transform`, `::Material`
- `BrickGenerator.cpp`: `::Transform`
- `DX12RenderingServer_CommandListAPI.cpp`: model count (update deferred from Task 1 if needed)

**Migration pattern:**
```cpp
// Before:
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GPUModelData)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Transform)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::TransformPrev)
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Material)
g_Engine->Get<RenderingContextService>()->GetGPUModelData()

// After:
g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer()
g_Engine->Get<DrawCallService>()->GetCurrentFrameTransformBuffer()
g_Engine->Get<DrawCallService>()->GetPreviousFrameTransformBuffer()
g_Engine->Get<DrawCallService>()->GetMaterialBuffer()
g_Engine->Get<DrawCallService>()->GetGPUModelData()
```

**Files:**
- Create: `Source/Engine/Services/DrawCallService.h`
- Create: `Source/Engine/Services/DrawCallService.cpp`
- Modify: `Source/Engine/Engine.cpp`, `RenderingContextService.h/.cpp`, all callers

- [ ] **Step 3.1: Create `DrawCallService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct DrawCallServiceImpl;
    class DrawCallService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(DrawCallService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const std::vector<GPUModelData>& GetGPUModelData();
        GPUBufferComponent* GetGPUModelDataBuffer();
        GPUBufferComponent* GetCurrentFrameTransformBuffer();
        GPUBufferComponent* GetPreviousFrameTransformBuffer();
        GPUBufferComponent* GetMaterialBuffer();

    private:
        DrawCallServiceImpl* m_Impl;
    };
}
```

- [ ] **Step 3.2: Create `DrawCallService.cpp`**

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<GPUModelData> m_gpuModelDataVector`
- `std::vector<TransformConstantBuffer> m_transformBufferVector`
- `std::vector<MaterialConstantBuffer> m_materialCBVector`
- `GPUBufferComponent* m_GPUModelDataBufferComp`
- `GPUBufferComponent* m_TransformBufferComp`
- `GPUBufferComponent* m_TransformPrevBufferComp`
- `GPUBufferComponent* m_MaterialGPUBufferComp`

Copy `UpdateDrawCalls()` from `RenderingContextService.cpp` (lines 383–522), removing the commented-out animation block (that belongs to `AnimationDrawCallService`).

Copy `GetCurrentFrameTransformBuffer()` and `GetPreviousFrameTransformBuffer()` helpers.

Update(): call `UpdateDrawCalls()`, then upload `m_gpuModelDataVector`, `m_transformBufferVector`, `m_materialCBVector` (guarded by `.size() > 0`).

**Terminate note:** The original `RenderingContextService::Terminate()` has a bug — it never calls `Delete()` on `m_TransformBufferComp` and `m_TransformPrevBufferComp`. Fix this in `DrawCallService::Terminate()`:
```cpp
l_renderingServer->Delete(m_TransformBufferComp);
l_renderingServer->Delete(m_TransformPrevBufferComp);
```
These two lines must be present even though they were absent in the original.

Required includes:
```cpp
#include "DrawCallService.h"
#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "RenderingConfigurationService.h"
#include "../Component/ModelComponent.h"
#include "../Component/DrawCallComponent.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/TextureComponent.h"
#include "../Engine.h"
```

- [ ] **Step 3.3: Register `DrawCallService` in `Engine.cpp`**

Same pattern as Task 1.3 — add `#include "Services/DrawCallService.h"`, then add `Get<DrawCallService>();` (pre-registration), `SystemSetup/Init/Term(DrawCallService)`, and `Get<DrawCallService>()->Update();` before the RenderingContextService counterparts.

Also update `DX12RenderingServer_CommandListAPI.cpp` here (deferred from Task 1): replace `g_Engine->Get<RenderingContextService>()` model count read with `g_Engine->Get<DrawCallService>()->GetGPUModelData().size()`.

**Reconnect modelCount in `PerFrameDataService`:** Now that `DrawCallService` exists, update `PerFrameDataServiceImpl::UpdatePerFrameConstantBuffer()` to replace the `modelCount = 0` stub with:
```cpp
l_perFrameCB.modelCount = static_cast<uint32_t>(g_Engine->Get<DrawCallService>()->GetGPUModelData().size());
```
Add `#include "DrawCallService.h"` to `PerFrameDataService.cpp`.

- [ ] **Step 3.4–3.6: Update callers → Remove from RCS → Build → GPU gate → Commit**

```bash
git commit -m "refactor: extract DrawCallService from RenderingContextService"
```

---

## Task 4: Extract AnimationDrawCallService

**Callers that will be updated:**
- `AnimationPass.cpp`: `GPUBufferUsageType::Animation`, `GetAnimationDrawCallInfo()`

**Migration pattern:**
```cpp
// Before:
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Animation)
g_Engine->Get<RenderingContextService>()->GetAnimationDrawCallInfo()

// After:
g_Engine->Get<AnimationDrawCallService>()->GetAnimationBuffer()
g_Engine->Get<AnimationDrawCallService>()->GetAnimationDrawCallInfo()
```

**Files:**
- Create: `Source/Engine/Services/AnimationDrawCallService.h`
- Create: `Source/Engine/Services/AnimationDrawCallService.cpp`
- Modify: `Engine.cpp`, `RenderingContextService.h/.cpp`, `AnimationPass.cpp`

- [ ] **Step 4.1: Create `AnimationDrawCallService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"
#include "AnimationService.h"    // AnimationInstance

namespace Inno
{
    struct AnimationDrawCallInfo
    {
        AnimationInstance animationInstance;
        uint32_t modelDataIndex;
        uint32_t animationConstantBufferIndex;
    };

    struct AnimationDrawCallServiceImpl;
    class AnimationDrawCallService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationDrawCallService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();
        GPUBufferComponent* GetAnimationBuffer();

    private:
        AnimationDrawCallServiceImpl* m_Impl;
    };
}
```

Note: `AnimationDrawCallInfo` struct moves here from `RenderingContextService.h`. Update `RenderingContextService.h` to include `AnimationDrawCallService.h` temporarily (or update callers directly).

- [ ] **Step 4.2: Create `AnimationDrawCallService.cpp`**

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<AnimationDrawCallInfo> m_animationDrawCallInfoVector`
- `std::vector<AnimationConstantBuffer> m_animationCBVector`
- `GPUBufferComponent* m_animationGPUBufferComp`

`UpdateDrawCalls()`: implement the animation draw call assembly (currently commented-out block in `RenderingContextService.cpp` lines 495–516). This path assembles `AnimationDrawCallInfo` from `AnimationService` instances.

Update(): call update method, upload animation CB if non-empty.

Required includes:
```cpp
#include "AnimationDrawCallService.h"
#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "AnimationService.h"
#include "../Engine.h"
```

- [ ] **Step 4.3: Register `AnimationDrawCallService` in `Engine.cpp`**

Same pattern as Task 1.3 — add `#include "Services/AnimationDrawCallService.h"`, then add `Get<AnimationDrawCallService>();` (pre-registration), `SystemSetup/Init/Term(AnimationDrawCallService)`, and `Get<AnimationDrawCallService>()->Update();` before the RenderingContextService counterparts.

- [ ] **Step 4.4–4.6: Update callers → Remove from RCS → Build → GPU gate → Commit**

Also remove `AnimationDrawCallInfo` struct from `RenderingContextService.h` (it now lives in `AnimationDrawCallService.h`). Update any files that used it.

```bash
git commit -m "refactor: extract AnimationDrawCallService from RenderingContextService"
```

---

## Task 5: Extract BillboardDrawCallService

**Callers that will be updated:**
- `BillboardPass.cpp`: `GPUBufferUsageType::Billboard`, `GetBillboardPassDrawCallInfo()`

**Migration pattern:**
```cpp
// Before:
g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Billboard)
g_Engine->Get<RenderingContextService>()->GetBillboardPassDrawCallInfo()

// After:
g_Engine->Get<BillboardDrawCallService>()->GetBillboardBuffer()
g_Engine->Get<BillboardDrawCallService>()->GetBillboardPassDrawCallInfo()
```

**Files:**
- Create: `Source/Engine/Services/BillboardDrawCallService.h`
- Create: `Source/Engine/Services/BillboardDrawCallService.cpp`
- Modify: `Engine.cpp`, `RenderingContextService.h/.cpp`, `BillboardPass.cpp`

- [ ] **Step 5.1: Create `BillboardDrawCallService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct BillboardDrawCallServiceImpl;
    class BillboardDrawCallService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(BillboardDrawCallService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
        GPUBufferComponent* GetBillboardBuffer();

    private:
        BillboardDrawCallServiceImpl* m_Impl;
    };
}
```

- [ ] **Step 5.2: Create `BillboardDrawCallService.cpp`**

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<BillboardPassDrawCallInfo> m_billboardPassDrawCallInfoVector`
- `std::vector<TransformConstantBuffer> m_directionalLightPerObjectCB`
- `std::vector<TransformConstantBuffer> m_pointLightPerObjectCB`
- `std::vector<TransformConstantBuffer> m_sphereLightPerObjectCB`
- `std::vector<TransformConstantBuffer> m_billboardPassPerObjectCB`
- `GPUBufferComponent* m_billboardGPUBufferComp`
- `std::function<void()> f_sceneLoadingFinishedCallback`

Copy `UpdateBillboardPassData()` from `RenderingContextService.cpp` (lines 524–588).

The scene-loading callback that sets icon textures (`f_sceneLoadingFinishedCallback` in Setup) moves here verbatim.

Setup: allocate billboard GPU buffer, register scene loading finished callback to populate icon textures from `TemplateAssetService`.
Update(): call `UpdateBillboardPassData()`, upload `m_billboardPassPerObjectCB`.
Terminate: delete billboard GPU buffer.

Required includes:
```cpp
#include "BillboardDrawCallService.h"
#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "SceneService.h"
#include "TemplateAssetService.h"
#include "../Engine.h"
```

- [ ] **Step 5.3: Register `BillboardDrawCallService` in `Engine.cpp`**

Same pattern as Task 1.3 — add `#include "Services/BillboardDrawCallService.h"`, then add `Get<BillboardDrawCallService>();` (pre-registration), `SystemSetup/Init/Term(BillboardDrawCallService)`, and `Get<BillboardDrawCallService>()->Update();` before the RenderingContextService counterparts.

- [ ] **Step 5.4–5.6: Update callers → Remove from RCS → Build → GPU gate → Commit**

```bash
git commit -m "refactor: extract BillboardDrawCallService from RenderingContextService"
```

---

## Task 6: Extract DebugDrawCallService

**Callers that will be updated:**
- `DebugPass.cpp`: `GetDebugPassDrawCallInfo()`

**Migration pattern:**
```cpp
// Before:
g_Engine->Get<RenderingContextService>()->GetDebugPassDrawCallInfo()

// After:
g_Engine->Get<DebugDrawCallService>()->GetDebugPassDrawCallInfo()
```

**Files:**
- Create: `Source/Engine/Services/DebugDrawCallService.h`
- Create: `Source/Engine/Services/DebugDrawCallService.cpp`
- Modify: `Engine.cpp`, `RenderingContextService.h/.cpp`, `DebugPass.cpp`

- [ ] **Step 6.1: Create `DebugDrawCallService.h`**

```cpp
#pragma once
#include "../Interface/ISystem.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct DebugDrawCallServiceImpl;
    class DebugDrawCallService : public ISystem
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(DebugDrawCallService);

        bool Setup(ISystemConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;

        ObjectStatus GetStatus() override;

        const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
        void Submit(const DebugPassDrawCallInfo& info);

    private:
        DebugDrawCallServiceImpl* m_Impl;
    };
}
```

- [ ] **Step 6.2: Create `DebugDrawCallService.cpp`**

The impl struct owns:
- `ObjectStatus m_ObjectStatus`
- `std::vector<DebugPassDrawCallInfo> m_debugPassDrawCallInfoVector`

Update(): clear `m_debugPassDrawCallInfoVector` at the start (frame-scoped contract).
Submit(): append to `m_debugPassDrawCallInfoVector`.

The original `UpdateDebuggerPassData()` is a `// @TODO: Implementation` stub — do not copy it. This service starts as a clean frame-scoped submit/clear mechanism.

Required includes:
```cpp
#include "DebugDrawCallService.h"
#include "../Common/LogService.h"
#include "../Engine.h"
```

- [ ] **Step 6.3: Register `DebugDrawCallService` in `Engine.cpp`**

Same pattern as Task 1.3 — add `#include "Services/DebugDrawCallService.h"`, then add `Get<DebugDrawCallService>();` (pre-registration), `SystemSetup/Init/Term(DebugDrawCallService)`, and `Get<DebugDrawCallService>()->Update();` before the RenderingContextService counterparts.

- [ ] **Step 6.4–6.6: Update callers → Remove from RCS → Build → GPU gate → Commit**

```bash
git commit -m "refactor: extract DebugDrawCallService from RenderingContextService"
```

---

## Task 7: Delete RenderingContextService

At this point `RenderingContextService` should own nothing. Verify this before deleting.

**Files:**
- Delete: `Source/Engine/Services/RenderingContextService.h`
- Delete: `Source/Engine/Services/RenderingContextService.cpp`
- Modify: `Source/Engine/Engine.cpp` (remove all three SystemSetup/Init/Term lines and the include)
- Modify: any remaining files that still `#include "RenderingContextService.h"` without using it

- [ ] **Step 7.1: Verify RenderingContextService is empty**

Read `RenderingContextService.h` and `RenderingContextService.cpp`. The impl struct should have no data members other than `m_ObjectStatus`. The `GetGPUBufferComponent()` switch should have no cases. The `GPUBufferUsageType` enum should have no values left.

If anything remains, it was missed in a previous task — fix it before continuing.

- [ ] **Step 7.2: Remove remaining includes**

Run the grep to find every file still referencing `RenderingContextService`:
```
grep -r "RenderingContextService" Source/ --include="*.cpp" --include="*.h" -l
```
For every file returned (excluding `RenderingContextService.cpp` itself which is about to be deleted): open it, verify it contains no active calls to `RenderingContextService` methods, then remove the `#include "RenderingContextService.h"` line. Do not skip any file from the grep results — any missed include will cause a build failure in Step 7.5.

Known include-only files (no active method calls, safe to remove): `VKRenderingServer_VulkanObject.cpp`, `VKRenderingServer_GraphicsDevice.cpp`, `VKRenderingServer_EngineComponent.cpp`, `VKRenderingServer.cpp`, `VKHelper_Pipeline.cpp`, `WinWindowSystem.cpp`, `WinDXWindowSurface.cpp`, `WinVKWindowSurface.cpp`, `STBWrapper.cpp`, `JSONWrapper.cpp`, `ImGuiRendererDX12.cpp`, `ImGuiRendererGL.cpp`, `ImGuiWrapper.cpp`, `RayTracer.cpp`, `GIDataLoader.cpp`, `BRDFLUTPass.cpp`, `BRDFLUTMSPass.cpp`, `PostTAAPass.cpp`, `PreTAAPass.cpp`, `VXGIConvertPass.cpp`, `VXGIMultiBouncePass.cpp`, `VXGIRayTracingPass.cpp`, `DefaultRenderingClient.cpp`, `AssetService.cpp`, `CameraSystem.cpp`, `HIDService.cpp`, `LightSystem.cpp`. Verify each one is truly method-call-free before removing.

- [ ] **Step 7.3: Remove all `RenderingContextService` references from `Engine.cpp`**

Remove each of the following (6 locations):
```cpp
#include "Services/RenderingContextService.h"          // top of file

Get<RenderingContextService>();                        // pre-registration block (~line 366)

SystemSetup(RenderingContextService);                  // Setup block

SystemInit(RenderingContextService);                   // Init block

Get<RenderingContextService>()->Update();              // Update callback (~line 508)

SystemTerm(RenderingContextService);                   // Terminate block
```

- [ ] **Step 7.4: Delete the files**

```bash
git rm Source/Engine/Services/RenderingContextService.h
git rm Source/Engine/Services/RenderingContextService.cpp
```

- [ ] **Step 7.5: Build**

Expected: 0 errors. Any remaining reference to `RenderingContextService` or `GPUBufferUsageType` is a missed caller — fix it.

- [ ] **Step 7.6: GPU gate**

Expected: `0`.

- [ ] **Step 7.7: Commit**

```bash
git commit -m "refactor: delete RenderingContextService — fully replaced by 6 focused services"
```
