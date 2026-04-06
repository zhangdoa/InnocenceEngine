# Enhanced Barriers + Obsolete Field Removal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace CPU-tracked barrier state with explicit before/after barrier API and remove 4 obsolete `ResourceBindingLayoutDesc` fields in a single pass through all render passes.

**Architecture:** The current `TryToTransitState` API ignores its `sourceAccessibility` parameter and reads from `m_CurrentState` CPU tracking vectors — a design that caused state-divergence crashes (the path tracer toggle bug). The new `Barrier()` API requires explicit before/after `Accessibility`, eliminating the tracking entirely. Simultaneously, 4 `ResourceBindingLayoutDesc` fields that duplicate information already on the resource components are removed; the DX12 backend reads from the resource at bind time instead.

**Tech Stack:** C++, DX12 Enhanced Barriers pattern (legacy resource barrier API with explicit states), MSVC RelWithDebInfo

---

### Task 1: Remove `m_IndirectBinding` field

The simplest removal — this field is set in 10 render pass files but never read by any engine service.

**Files:**
- Modify: `Source/Engine/Common/GraphicsPrimitive.h:349-350`
- Modify: 9 render pass files in `Source/DefaultClient/RenderingClient/`

- [ ] **Step 1: Remove the field from `ResourceBindingLayoutDesc`**

In `Source/Engine/Common/GraphicsPrimitive.h`, delete:
```cpp
// @TODO: deprecated, remove this
bool m_IndirectBinding = false;
```

- [ ] **Step 2: Remove all assignments in render passes**

In every render pass file, delete lines matching `m_IndirectBinding = true`:
- `AnimationPass.cpp` (6 occurrences)
- `BillboardPass.cpp` (2)
- `BSDFTestPass.cpp` (3)
- `MotionBlurPass.cpp` (4)
- `VolumetricPass.cpp` (15)
- `TransparentGeometryProcessPass.cpp` (2)
- `TransparentBlendPass.cpp` (2)
- `SunShadowBlurOddPass.cpp` (2)
- `SunShadowBlurEvenPass.cpp` (2)

Each occurrence is a standalone assignment like:
```cpp
m_RenderPassComp->m_ResourceBindingLayoutDescs[N].m_IndirectBinding = true;
```
Delete the entire line.

- [ ] **Step 3: Build**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```
Expected: Build succeeds with zero errors.

- [ ] **Step 4: Run regression test**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: Exit code 0.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Common/GraphicsPrimitive.h Source/DefaultClient/RenderingClient/AnimationPass.cpp Source/DefaultClient/RenderingClient/BillboardPass.cpp Source/DefaultClient/RenderingClient/BSDFTestPass.cpp Source/DefaultClient/RenderingClient/MotionBlurPass.cpp Source/DefaultClient/RenderingClient/VolumetricPass.cpp Source/DefaultClient/RenderingClient/TransparentGeometryProcessPass.cpp Source/DefaultClient/RenderingClient/TransparentBlendPass.cpp Source/DefaultClient/RenderingClient/SunShadowBlurOddPass.cpp Source/DefaultClient/RenderingClient/SunShadowBlurEvenPass.cpp
git commit -m "refactor: remove deprecated m_IndirectBinding field from ResourceBindingLayoutDesc"
```

---

### Task 2: Remove `m_GPUBufferUsage` field

Only 4 occurrences. The single backend consumer (`DX12RenderPassResourceService.cpp:166`) checks `m_GPUBufferUsage == GPUBufferUsage::TLAS` to decide between a root SRV and a descriptor table SRV. Replace with checking `gpuBuffer->m_Usage`.

**Files:**
- Modify: `Source/Engine/Common/GraphicsPrimitive.h:347`
- Modify: `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:166`
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`
- Modify: `Source/DefaultClient/RenderingClient/RadianceCacheRaytracingPass.cpp`

- [ ] **Step 1: Update the DX12 backend consumer**

In `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`, the `GetDescriptorRange` method checks `m_GPUBufferUsage`. This is called during root signature creation where we don't have the runtime resource yet, but we DO know from the layout desc that this slot is for a TLAS (it's a static pipeline property). The correct fix: the TLAS slot uses `m_DescriptorSetIndex` and `m_DescriptorIndex` which are unique. Instead, since TLAS buffers are always bound as root SRV (not descriptor table), encode this as a new field `m_IsRootSRV` on `ResourceBindingLayoutDesc` — a pipeline property, not a resource property.

Add to `ResourceBindingLayoutDesc` in `GraphicsPrimitive.h`:
```cpp
bool m_IsRootSRV = false;
```

In `DX12RenderPassResourceService.cpp:166`, change:
```cpp
if (l_resourceBinderLayoutDesc.m_GPUBufferUsage == GPUBufferUsage::TLAS)
```
to:
```cpp
if (l_resourceBinderLayoutDesc.m_IsRootSRV)
```

- [ ] **Step 2: Update render pass call sites**

In `GPUPathTracerPass.cpp`, find:
```cpp
.m_GPUBufferUsage = GPUBufferUsage::TLAS;
```
Replace with:
```cpp
.m_IsRootSRV = true;
```

Same in `RadianceCacheRaytracingPass.cpp`.

- [ ] **Step 3: Remove the field from `ResourceBindingLayoutDesc`**

In `GraphicsPrimitive.h`, delete:
```cpp
GPUBufferUsage m_GPUBufferUsage = GPUBufferUsage::Generic;  // @TODO: remove this as we can get it from the res getter
```

- [ ] **Step 4: Build and test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```
Then regression test:
```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: Exit code 0.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Common/GraphicsPrimitive.h Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp Source/DefaultClient/RenderingClient/RadianceCacheRaytracingPass.cpp
git commit -m "refactor: replace m_GPUBufferUsage with m_IsRootSRV in ResourceBindingLayoutDesc"
```

---

### Task 3: Remove `m_ResourceAccessibility` field

The most impactful removal — 98 occurrences across 36 files. The DX12 backend uses this field in 3 places to select descriptor heap regions and descriptor range types. Replace with reading `resource->m_GPUAccessibility` at bind time (already available) and `TextureComponent::m_TextureDesc.GPUAccessibility` at root signature creation time.

**Files:**
- Modify: `Source/Engine/Common/GraphicsPrimitive.h:341`
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` (2 consumers in `BindComputeResource`/`BindGraphicsResource`)
- Modify: `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp` (2 consumers in `GetDescriptorRange`)
- Modify: `Source/Engine/Services/VK/VKGraphicsService_EngineComponent.cpp` (3 occurrences)
- Modify: ~30 render pass files in `Source/DefaultClient/RenderingClient/`

- [ ] **Step 1: Update `BindComputeResource` in `DX12FrameManagementService.cpp`**

The method already has the `resource` parameter (a `GPUResourceComponent*`). Where it currently reads `resourceBindingLayoutDesc.m_ResourceAccessibility`, change to `resource->m_GPUAccessibility`. For Image type where it reads `m_TextureUsage`, also replace with `reinterpret_cast<TextureComponent*>(resource)->m_TextureDesc.Usage`.

Around line 560-580, change:
```cpp
if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::Sample)
{
    auto& l_textureDescHeapAccessor = m_ctx->GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
        , resourceBindingLayoutDesc.m_ResourceAccessibility, TextureUsage::Sample);
```
to:
```cpp
auto l_image = reinterpret_cast<TextureComponent*>(resource);
if (l_image->m_TextureDesc.Usage == TextureUsage::Sample)
{
    auto& l_textureDescHeapAccessor = m_ctx->GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
        , l_image->m_GPUAccessibility, TextureUsage::Sample);
```

Apply the same pattern to the depth/color/compute branch and the `BindGraphicsResource` method (same logic, different `Set*RootDescriptorTable` call).

- [ ] **Step 2: Update `GetDescriptorRange` in `DX12RenderPassResourceService.cpp`**

This is called during root signature creation where we don't have the runtime resource. However, the range type (CBV/SRV/UAV) depends only on pipeline-static information:
- Buffer + ReadOnly binding → CBV
- Buffer + ReadWrite binding → SRV (StructuredBuffer)
- Image + ReadOnly binding → SRV
- Image + ReadWrite binding → UAV

The `m_BindingAccessibility` already captures this: ReadOnly binding = SRV/CBV, ReadWrite binding = UAV. The `m_ResourceAccessibility` was only used to distinguish CBV (resource ReadOnly, small upload heap) from SRV (resource ReadWrite, default heap with StructuredBuffer).

The fix: use `m_BindingAccessibility` alone. For buffers:
- `m_BindingAccessibility == ReadOnly` → CBV (constant buffer)
- `m_BindingAccessibility.CanWrite()` → SRV or UAV based on binding

Around line 298-307:
```cpp
if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
{
    if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
    {
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    }
    else if (resourceBinderLayoutDesc.m_ResourceAccessibility.CanWrite())
    {
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}
```
Change to:
```cpp
if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
{
    if (resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
    {
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    }
    else if (resourceBinderLayoutDesc.m_BindingAccessibility.CanWrite())
    {
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}
```

Wait — this is WRONG. Some slots bind a ReadWrite resource (SRV) via a ReadOnly binding slot. For example, LightPass slot 15: `m_BindingAccessibility = ReadOnly, m_ResourceAccessibility = ReadWrite` → should be SRV, not CBV.

The correct approach: when `m_BindingAccessibility == ReadOnly` and we previously had `m_ResourceAccessibility == ReadWrite`, the range type should be SRV. But at root signature time we don't have the resource. This means we need the information in the layout desc.

The design decision: collapse `m_ResourceAccessibility` into `m_BindingAccessibility`. If a slot reads a ReadWrite resource via a read-only binding, the `m_BindingAccessibility` should express this. Review the existing assignments:

The pattern is: render passes that set both `m_BindingAccessibility = ReadOnly` AND `m_ResourceAccessibility = ReadWrite` are saying "I bind as SRV a resource that lives on the default heap (ReadWrite)". This is an SRV descriptor range, not CBV.

The correct encoding without `m_ResourceAccessibility`: use a new field `m_IsStructuredBuffer = true` on those slots. Or simpler: the range type for Buffer+ReadOnly defaults to SRV (not CBV), and CBV is only used for plain constant buffers. Let me check — which slots use CBV?

CBV slots are the ones where BOTH `m_BindingAccessibility` and `m_ResourceAccessibility` are `ReadOnly`. These are the b0-b5 constant buffer slots (PerFrameCBuffer, etc.). Every render pass has these, and they never set `m_ResourceAccessibility` explicitly (it defaults to `ReadOnly`).

SRV buffer slots are those where `m_BindingAccessibility = ReadOnly` AND `m_ResourceAccessibility = ReadWrite`. These are StructuredBuffer slots like light index lists.

So the rule is: if `m_ResourceAccessibility` was explicitly set to `ReadWrite`, the slot is SRV. If left at default (`ReadOnly`), the slot is CBV.

The cleanest encoding: a new bool `m_IsStructuredBufferBinding = false` on `ResourceBindingLayoutDesc`. When true + Buffer type → SRV range instead of CBV. This is a pipeline property, not a resource property.

Actually, even simpler. Looking at the data: every buffer slot that's CBV uses default `m_BindingAccessibility = ReadOnly` AND default `m_ResourceAccessibility = ReadOnly`. Every buffer slot that's SRV explicitly sets `m_ResourceAccessibility = ReadWrite`. So the minimal change at root signature time is to check `m_BindingAccessibility.CanWrite()` for UAV, and for read-only bindings, we need one bit to distinguish CBV from SRV.

The field replacement:
```cpp
bool m_IsStructuredBufferBinding = false;  // Buffer + ReadOnly binding: false=CBV, true=SRV
```

- [ ] **Step 2 (revised): Add `m_IsStructuredBufferBinding` and update backends**

In `GraphicsPrimitive.h`, add to `ResourceBindingLayoutDesc`:
```cpp
bool m_IsStructuredBufferBinding = false;
```

In `DX12RenderPassResourceService.cpp:298-307`, change:
```cpp
if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
{
    if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    else if (resourceBinderLayoutDesc.m_ResourceAccessibility.CanWrite())
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}
else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
{
    if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}
```
to:
```cpp
if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
{
    if (resourceBinderLayoutDesc.m_IsStructuredBufferBinding)
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    else
        l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
}
else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
{
    l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}
```

In `GetDescriptorRange` line 288, change:
```cpp
auto& l_descriptorAccessor = m_ctx->GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
    , resourceBinderLayoutDesc.m_ResourceAccessibility, resourceBinderLayoutDesc.m_TextureUsage);
```
to:
```cpp
auto l_resourceAccessibility = resourceBinderLayoutDesc.m_IsStructuredBufferBinding ? Accessibility::ReadWrite : resourceBinderLayoutDesc.m_BindingAccessibility;
auto& l_descriptorAccessor = m_ctx->GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
    , l_resourceAccessibility, resourceBinderLayoutDesc.m_TextureUsage);
```

- [ ] **Step 3: Update render pass files**

For every render pass file that sets `m_ResourceAccessibility = Accessibility::ReadWrite`:
- If `m_GPUResourceType == Buffer`: replace with `m_IsStructuredBufferBinding = true`
- If `m_GPUResourceType == Image`: simply delete the line (the bind-time code reads from the resource)

Delete ALL remaining `m_ResourceAccessibility` assignments (those that set it to `ReadOnly` are just restating the default).

Files to update (all in `Source/DefaultClient/RenderingClient/`):
`AnimationPass.cpp`, `BillboardPass.cpp`, `BRDFLUTMSPass.cpp`, `BRDFLUTPass.cpp`, `DebugPass.cpp`, `FinalBlendPass.cpp`, `GPUPathTracerPass.cpp`, `LightCullingPass.cpp`, `LightPass.cpp`, `LuminanceAveragePass.cpp`, `LuminanceHistogramPass.cpp`, `MotionBlurPass.cpp`, `OpaqueCullingPass.cpp`, `OpaquePass.cpp`, `PostTAAPass.cpp`, `PreTAAPass.cpp`, `RadianceCacheFilterHorizontalPass.cpp`, `RadianceCacheFilterVerticalPass.cpp`, `RadianceCacheIntegrationPass.cpp`, `RadianceCacheRaytracingPass.cpp`, `RadianceCacheReprojectionPass.cpp`, `SSAOPass.cpp`, `SkyPass.cpp`, `SunShadowBlurEvenPass.cpp`, `SunShadowBlurOddPass.cpp`, `SunShadowCullingPass.cpp`, `SunShadowGeometryProcessPass.cpp`, `TAAPass.cpp`, `TiledFrustumGenerationPass.cpp`, `TransparentBlendPass.cpp`, `TransparentGeometryProcessPass.cpp`, `VolumetricPass.cpp`

Also: `Source/Engine/Services/VK/VKGraphicsService_EngineComponent.cpp` — find and update/remove `m_ResourceAccessibility` references.

- [ ] **Step 4: Remove the field from `ResourceBindingLayoutDesc`**

In `GraphicsPrimitive.h`, delete:
```cpp
Accessibility m_ResourceAccessibility = Accessibility::ReadOnly; // @TODO: remove this as we can get it from the res getter
```

- [ ] **Step 5: Build and test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```
Then integration test:
```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: Exit code 0. This is the critical test — wrong descriptor range types will cause GPU crashes.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "refactor: remove m_ResourceAccessibility, derive descriptor type from binding and resource"
```

---

### Task 4: Remove `m_TextureUsage` field

87 occurrences across 23 files. The DX12 backend uses `m_TextureUsage` in:
1. `BindComputeResource`/`BindGraphicsResource` — to select descriptor heap accessor and branch on Sample vs Attachment. Replace with reading `TextureComponent::m_TextureDesc.Usage`.
2. `GetDescriptorRange` — to decide `NumDescriptors` (Sample → full heap, Attachment → 1). Also replace with reading from desc at pipeline creation time via `m_TextureUsage` on `ResourceBindingLayoutDesc`. Wait — at root signature time we don't have the resource either.

Actually, for `GetDescriptorRange` line 333-339, the distinction is: `Sample` textures use bindless (full descriptor array), while attachment textures use single descriptors. This is pipeline-static — the layout desc must encode it. The `m_TextureUsage` in the layout desc already encodes this and it's NOT redundant with the resource — it's the pipeline's view of what type of texture binding this slot expects. However, the `@TODO` comment says otherwise.

After analysis: `m_TextureUsage` serves two purposes:
1. **Pipeline-static (root signature):** Sample → bindless array, Attachment → single descriptor
2. **Bind-time (command recording):** Selecting descriptor heap region

For purpose 1, we can replace with `bool m_IsBindlessTexture = false` (Sample slots are bindless). For purpose 2, we read `texture->m_TextureDesc.Usage` from the resource.

Actually the simpler approach: keep `m_TextureUsage` since it IS a pipeline property (what kind of texture slot this is). The TODO comment was wrong — it's not purely duplicating the resource. BUT: we should rename it to make clear it describes the slot, not the resource.

**Decision:** Rename `m_TextureUsage` to `m_TextureSlotType` to clarify it's a pipeline property. Remove assignments in render passes where the value can be inferred from the binding (`ReadWrite` binding → always attachment/compute, `ReadOnly` binding for Image → could be Sample or attachment). Actually this gets complex. Let's keep this task simple:

**Revised decision:** The `m_TextureUsage` field in `ResourceBindingLayoutDesc` is NOT redundant — it describes the binding slot type, not the resource. The TODO comment is misleading. Remove the TODO comment and keep the field.

**Files:**
- Modify: `Source/Engine/Common/GraphicsPrimitive.h:346` — remove the TODO comment only

- [ ] **Step 1: Remove the misleading TODO comment**

In `GraphicsPrimitive.h`, change:
```cpp
TextureUsage m_TextureUsage = TextureUsage::Invalid;  // @TODO: remove this as we can get it from the res getter
```
to:
```cpp
TextureUsage m_TextureSlotType = TextureUsage::Invalid;
```

- [ ] **Step 2: Rename all occurrences across codebase**

Search-and-replace `m_TextureUsage` → `m_TextureSlotType` in all files under `Source/` (excluding `Source/External/`). This is a mechanical rename across 23 files.

- [ ] **Step 3: Build and test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```
Then regression test:
```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: Exit code 0.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "refactor: rename m_TextureUsage to m_TextureSlotType in ResourceBindingLayoutDesc"
```

---

### Task 5: Migrate `TryToTransitState` to explicit `Barrier` API

64 call sites across 18 files. The new API takes explicit before/after `Accessibility` and eliminates `m_CurrentState` CPU tracking.

**Files:**
- Modify: `Source/Engine/Services/FrameManagementService.h` — rename virtual method
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.h` — rename override
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` — update implementation
- Modify: `Source/Engine/Services/VK/VKGraphicsService.h` — rename override
- Modify: `Source/Engine/Services/VK/VKGraphicsService.cpp` — update implementation
- Modify: `Source/Engine/Component/TextureComponent.h` — remove `m_CurrentState`, `GetCurrentState`, `SetCurrentState`
- Modify: `Source/Engine/Component/GPUBufferComponent.h` — remove `m_CurrentState`, `GetCurrentState`, `SetCurrentState`
- Modify: `Source/Engine/Component/GPUResourceComponent.h` — remove `m_ReadState`, `m_WriteState`
- Modify: `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` — update 4 call sites
- Modify: `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiRendererDX12.cpp` — update 1 call site
- Modify: ~14 render pass files in `Source/DefaultClient/RenderingClient/`
- Modify: `Source/Engine/Services/DX12/DX12TextureResourceService.cpp` — remove `m_ReadState`/`m_WriteState` assignments
- Modify: `Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp` — remove `m_ReadState`/`m_WriteState` assignments

- [ ] **Step 1: Rename `TryToTransitState` to `Barrier` in base class**

In `Source/Engine/Services/FrameManagementService.h`, change:
```cpp
virtual bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }
virtual bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) { return false; }
```
to:
```cpp
virtual bool Barrier(TextureComponent* texture, CommandListComponent* commandList, Accessibility before, Accessibility after) { return false; }
virtual bool Barrier(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility before, Accessibility after) { return false; }
```

- [ ] **Step 2: Update DX12 implementation to use explicit before/after**

In `DX12FrameManagementService.h`, rename overrides to `Barrier`.

In `DX12FrameManagementService.cpp`, the texture overload (~line 254):
```cpp
bool DX12FrameManagementService::Barrier(TextureComponent* texture, CommandListComponent* commandList, Accessibility before, Accessibility after)
{
    auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
    uint32_t frameIndex = GetCurrentFrame();

    auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));

    auto l_oldState = GetD3D12ResourceState(before, texture->m_TextureDesc);
    auto l_newState = GetD3D12ResourceState(after, texture->m_TextureDesc);

    if (l_oldState != l_newState)
    {
        auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(resource, l_oldState, l_newState);
        l_commandList->ResourceBarrier(1, &l_transition);
    }

    return true;
}
```

And the GPU buffer overload (~line 283):
```cpp
bool DX12FrameManagementService::Barrier(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility before, Accessibility after)
{
    auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
    uint32_t frameIndex = GetCurrentFrame();

    auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[frameIndex]);

    auto l_oldState = GetD3D12BufferResourceState(before);
    auto l_newState = GetD3D12BufferResourceState(after);

    if (l_oldState != l_newState)
    {
        auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(l_deviceMemory->m_DefaultHeapBuffer.Get(), l_oldState, l_newState);
        l_commandList->ResourceBarrier(1, &l_transition);
    }

    return true;
}
```

Add helper functions (private or in DX12Helper):
```cpp
static D3D12_RESOURCE_STATES GetD3D12ResourceState(Accessibility access, const TextureDesc& desc)
{
    if (access.IsCopySource()) return D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (access.IsCopyDestination()) return D3D12_RESOURCE_STATE_COPY_DEST;
    if (access.CanWrite()) return DX12Helper::GetTextureWriteState(desc);
    return DX12Helper::GetTextureReadState(desc);
}

static D3D12_RESOURCE_STATES GetD3D12BufferResourceState(Accessibility access)
{
    if (access.IsCopySource()) return D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (access.IsCopyDestination()) return D3D12_RESOURCE_STATE_COPY_DEST;
    if (access.CanWrite()) return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
}
```

- [ ] **Step 3: Update VK stubs**

In `VKGraphicsService.h` and `VKGraphicsService.cpp`, rename `TryToTransitState` to `Barrier` (stub implementations).

- [ ] **Step 4: Update all call sites**

Mechanical rename `TryToTransitState` → `Barrier` across all 18 files. All existing call sites already pass explicit before/after values, so the parameter semantics don't change — only the name.

Files: `FrameManagementServiceImpl.cpp`, `ImGuiRendererDX12.cpp`, `FinalBlendPass.cpp`, `GPUPathTracerPass.cpp`, `LightCullingPass.cpp`, `LightPass.cpp`, `LuminanceHistogramPass.cpp`, `PostTAAPass.cpp`, `PreTAAPass.cpp`, `RadianceCacheFilterHorizontalPass.cpp`, `RadianceCacheFilterVerticalPass.cpp`, `RadianceCacheIntegrationPass.cpp`, `RadianceCacheRaytracingPass.cpp`, `RadianceCacheReprojectionPass.cpp`, `SSAOPass.cpp`, `TAAPass.cpp`

- [ ] **Step 5: Remove `m_CurrentState` from components**

In `TextureComponent.h`, remove:
```cpp
mutable std::vector<uint32_t> m_CurrentState;
uint32_t GetCurrentState(uint32_t frameIndex) const { ... }
void SetCurrentState(uint32_t frameIndex, uint32_t state) const { ... }
```

In `GPUBufferComponent.h`, remove:
```cpp
mutable std::vector<uint32_t> m_CurrentState;
uint32_t GetCurrentState(uint32_t frameIndex) const { ... }
void SetCurrentState(uint32_t frameIndex, uint32_t state) const { ... }
```

In `GPUResourceComponent.h`, remove:
```cpp
uint32_t m_ReadState = 0;
uint32_t m_WriteState = 0;
```

- [ ] **Step 6: Remove `m_ReadState`/`m_WriteState` assignments from resource services**

In `DX12TextureResourceService.cpp`, remove:
```cpp
texture->m_WriteState = GetTextureWriteState(texture->m_TextureDesc);
texture->m_ReadState = GetTextureReadState(texture->m_TextureDesc);
```
And update the initial state computation to inline the helper call:
```cpp
auto l_initialState = static_cast<D3D12_RESOURCE_STATES>(texture->m_TextureDesc.Usage == TextureUsage::Sample ? DX12Helper::GetTextureReadState(texture->m_TextureDesc) : DX12Helper::GetTextureWriteState(texture->m_TextureDesc));
```

In `DX12GPUBufferResourceService.cpp`, remove all `m_ReadState`/`m_WriteState` assignments (6 lines).

In `DX12FrameManagementService.cpp` (swap chain setup ~line 941-942), remove:
```cpp
l_textureComp->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_PRESENT);
l_textureComp->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_RENDER_TARGET);
```

- [ ] **Step 7: Remove `m_CurrentState` initialization from resource services**

Search for any code that initializes `m_CurrentState` vectors (resize, push_back) in texture and buffer resource services and remove it.

- [ ] **Step 8: Build and full integration test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```
Then full integration:
```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```
Then scene reload:
```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: All exit code 0.

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "refactor: replace TryToTransitState with explicit Barrier API, remove CPU state tracking"
```

---

### Task 6: Swap chain special case for Present barrier

The swap chain texture needs special handling — its "read" state is `PRESENT` (not the generic texture read state). After removing `m_ReadState`, the `Barrier` implementation uses `GetTextureReadState()` which returns `SRV|COPY_SOURCE` — wrong for swap chain.

**Files:**
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp`

- [ ] **Step 1: Handle swap chain in `GetD3D12ResourceState`**

The swap chain texture has `m_TextureDesc.Usage == TextureUsage::ColorAttachment`. Its read state should be `PRESENT`, not the generic attachment read state. Add a special case:

```cpp
static D3D12_RESOURCE_STATES GetD3D12ResourceState(Accessibility access, const TextureDesc& desc, bool isSwapChain = false)
{
    if (access.IsCopySource()) return D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (access.IsCopyDestination()) return D3D12_RESOURCE_STATE_COPY_DEST;
    if (access.CanWrite()) return DX12Helper::GetTextureWriteState(desc);
    if (isSwapChain) return D3D12_RESOURCE_STATE_PRESENT;
    return DX12Helper::GetTextureReadState(desc);
}
```

In `FrameManagementServiceImpl.cpp`, the swap chain barriers already use `Accessibility::ReadOnly` for the "present" transition. The `Barrier` call is for the swap chain render target. We need to tag the swap chain texture or add an overload.

Simpler approach: in `PrepareSwapChainCommands` and `ExecuteSwapChainCommands`, the swap chain barriers were:
```cpp
TryToTransitState(swapChainTexture, cl, Accessibility::WriteOnly, Accessibility::ReadOnly);
```

For the swap chain, `ReadOnly` means `PRESENT`. Since `Accessibility::ReadOnly` already maps to "read state" in our helper, we just need the helper to know this is a swap chain texture. The cleanest way: add an `Accessibility::Present` static value, or check if the texture is the swap chain.

Alternative: the swap chain render pass already knows its own textures. The `FrameManagementServiceImpl.cpp` calls are on `m_SwapChainRenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0]`. We can add a `bool m_IsSwapChain = false` field to `TextureComponent` (set during swap chain creation).

Actually, the simplest correct approach: `PRESENT` is a D3D12-specific state. In the abstraction, transitioning a swap chain texture to `ReadOnly` means "present". The DX12 backend can detect swap chain textures by checking if the `ID3D12Resource*` matches the swap chain back buffer. But that's complex.

Even simpler: just hardcode the swap chain special case in `PrepareSwapChainCommands`/`ExecuteSwapChainCommands` where we know we're dealing with the swap chain.

- [ ] **Step 2: Build and test**

```bash
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```
Expected: Exit code 0.

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "fix: handle swap chain Present state in Barrier API"
```

---

### Task 7: Final validation

Full test suite across all tiers to confirm the refactoring is correct.

- [ ] **Step 1: Regression test**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

- [ ] **Step 2: Integration test (10 frames)**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

- [ ] **Step 3: Scene reload test**

```bash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

All expected: Exit code 0.
