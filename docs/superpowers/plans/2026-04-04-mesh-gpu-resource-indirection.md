# Mesh GPU Resource Indirection — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Decouple MeshComponent (ECS value type) from GPU resource ownership by introducing an indirection handle and a lifespan-tagged GPU mesh resource table owned by the graphics service.

**Architecture:** MeshComponent becomes a pure value type holding a `GPUMeshResourceHandle` (uint32 index) plus an AABB. The graphics service owns a `GPUMeshResourceTable` that maps handles to GPU resources (D3D12 buffers, BLAS, mapped memory, buffer views). Each resource entry is tagged with `ObjectLifespan` so scene unloading bulk-releases `Scene`-lifespan resources without per-entity Delete calls or ref counting.

**Tech Stack:** C++17, D3D12, engine ECS (`EntityRegistry`), engine containers (`ThreadSafeUnorderedMap`, `ThreadSafeVector`)

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `Source/Engine/Common/GPUMeshResource.h` | **Create** | `GPUMeshResourceHandle` value type + `GPUMeshResource` struct (buffer views, AABB, mapped memory, lifespan) |
| `Source/Engine/Component/MeshComponent.h` | **Modify** | Replace GPU fields with `GPUMeshResourceHandle`; keep only handle + AABB + status + name |
| `Source/Engine/Services/IGraphicsService.h` | **Modify** | Add `GPUMeshResourceTable`, change `MeshInitTask` to carry handle, remove mesh from `GPUHandlePools`, update `Initialize`/`Delete` signatures |
| `Source/Engine/Services/Common/IGraphicsService.cpp` | **Modify** | Rewrite mesh init task processing to populate resource table; rewrite `OnSceneUnloading` mesh path to bulk-release by lifespan; update `AddMeshComponent`/`FindMeshByName` |
| `Source/Engine/Services/DX12/DX12GraphicsService.h` | **Modify** | Remove 6 mesh side-maps; add `GPUMeshResourceTable` accessor |
| `Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp` | **Modify** | `InitializeImpl` writes to resource table entry instead of side-maps + component fields |
| `Source/Engine/Services/DX12/DX12GraphicsService_ComponentPool.cpp` | **Modify** | `Delete(MeshComponent*)` → `ReleaseMeshGPUResource(GPUMeshResourceHandle)` operating on resource table |
| `Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp` | **Modify** | `DrawIndexedInstanced` resolves handle → resource table entry for buffer views |
| `Source/Engine/Services/DrawCallService.cpp` | **Modify** | `BuildDrawCalls` resolves handle → resource table entry for GPU addresses and strides |
| `Source/Engine/Services/TemplateAssetService.cpp` | **Modify** | Template mesh creation stores handle; `GetMeshComponent` returns component with handle |
| `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` | **Modify** | Mesh Load: template path copies handle; custom path unchanged (Initialize assigns handle) |
| `Source/DefaultClient/LogicClient/World.inl` | **Modify** | `attachMeshAndMaterial` copies handle + AABB instead of buffer views + mapped memory |
| `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp` | **Modify** | Resolve handle to get mapped memory and buffer views |
| `Source/DefaultClient/RenderingClient/BSDFTestPass.cpp` | **Modify** | Minor — `GetMeshComponent` still returns `MeshComponent*`, no change needed beyond field access |
| `Source/DefaultClient/RenderingClient/SurfelGITestPass.cpp` | **Modify** | Verify — currently just gets pointer, passes to `DrawIndexedInstanced` |

---

## Task 1: Define GPUMeshResource and GPUMeshResourceHandle

**Files:**
- Create: `Source/Engine/Common/GPUMeshResource.h`
- Modify: `Source/Engine/Component/MeshComponent.h`

- [ ] **Step 1: Create `GPUMeshResource.h` with handle and resource struct**

```cpp
// Source/Engine/Common/GPUMeshResource.h
#pragma once
#include "GraphicsPrimitive.h"
#include "Object.h"
#include "MathHelper.h"

namespace Inno
{
	struct GPUMeshResourceHandle
	{
		uint32_t m_Index = UINT32_MAX;
		bool IsValid() const { return m_Index != UINT32_MAX; }
	};

	inline constexpr GPUMeshResourceHandle INVALID_GPU_MESH_HANDLE = {};

	struct GPUMeshResource
	{
		ObjectLifespan m_Lifespan = ObjectLifespan::Invalid;
		ObjectStatus m_Status = ObjectStatus::Invalid;
		ObjectName m_Name = "";

		GPUBufferView m_VertexBufferView;
		GPUBufferView m_IndexBufferView;

		void* m_MappedMemory_VB = nullptr;
		void* m_MappedMemory_IB = nullptr;

		AABB m_AABB;

		uint32_t GetIndexCount() const
		{
			if (m_IndexBufferView.m_StrideInBytes == 0)
				return 0;
			return m_IndexBufferView.m_SizeInBytes / m_IndexBufferView.m_StrideInBytes;
		}
	};
}
```

- [ ] **Step 2: Slim down `MeshComponent.h`**

Replace the current MeshComponent with:

```cpp
// Source/Engine/Component/MeshComponent.h
#pragma once
#include "../Common/GPUMeshResource.h"
#include "../Common/Object.h"

namespace Inno
{
	struct MeshComponent
	{
		static uint32_t GetTypeID() { return 6; };
		static const char* GetTypeName() { return "MeshComponent"; };

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		ObjectName   m_InstanceName = "";

		GPUMeshResourceHandle m_GPUResource;
	};
}
```

Note: `AABB` moves into `GPUMeshResource`. `GetIndexCount()` moves there too. `GPUBufferView` stays defined in `GraphicsPrimitive.h` (unchanged). The `m_MappedMemory_*` and `m_VertexBufferView`/`m_IndexBufferView` fields are removed from MeshComponent — they now live in the resource table.

- [ ] **Step 3: Build to find all compilation errors from removed fields**

Run:
```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Expected: Many compilation errors from files that access removed MeshComponent fields. This gives a complete list of sites to update. Do NOT fix them yet — just record the file:line pairs.

- [ ] **Step 4: Commit the type definitions (compile-broken is OK at this point)**

```bash
git add Source/Engine/Common/GPUMeshResource.h Source/Engine/Component/MeshComponent.h
git commit -m "refactor: define GPUMeshResource and slim MeshComponent to handle-only

Introduce GPUMeshResourceHandle (uint32 index into a resource table) and
GPUMeshResource (holds buffer views, mapped memory, AABB, lifespan tag).
MeshComponent becomes a value type: handle + status + name.

This deliberately breaks compilation — subsequent tasks wire up the
resource table and update all consumers.

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 2: Add GPUMeshResourceTable to IGraphicsService

**Files:**
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

- [ ] **Step 1: Add resource table and allocation/lookup methods to `IGraphicsService.h`**

In `IGraphicsService.h`, add inside the `protected:` section (near the existing `GPUHandlePools`):

```cpp
// GPU Mesh Resource Table — owns all mesh GPU resources, indexed by handle
std::vector<GPUMeshResource> m_MeshResources;
std::vector<uint32_t> m_FreeMeshResourceSlots;
ThreadSafeUnorderedMap<std::string, GPUMeshResourceHandle> m_MeshResourceLUT;

GPUMeshResourceHandle AllocateMeshResource(const char* name, ObjectLifespan lifespan);
void ReleaseMeshResource(GPUMeshResourceHandle handle);
void ReleaseAllMeshResources(ObjectLifespan lifespan);
```

Add a public accessor:

```cpp
public:
	GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle);
	const GPUMeshResource* GetMeshResource(GPUMeshResourceHandle handle) const;
	GPUMeshResourceHandle FindMeshResourceByName(const char* name);
```

Include `GPUMeshResource.h` at the top.

Remove from `GPUHandlePools`:
- `TObjectPool<MeshComponent>* Meshes` (and its pool init/terminate)
- `ThreadSafeUnorderedMap<std::string, MeshComponent*> MeshLUT`
- `ThreadSafeVector<MeshComponent*> MeshPointers`

- [ ] **Step 2: Update `MeshInitTask` to carry lifespan instead of raw component pointer for resource creation**

```cpp
struct MeshInitTask
{
	MeshInitTask(MeshComponent* component, std::vector<Vertex>&& vertices,
	             std::vector<Index>&& indices, EntityID owner = INVALID_ENTITY,
	             ObjectLifespan lifespan = ObjectLifespan::Invalid)
		: m_Component(component), m_Vertices(std::move(vertices)),
		  m_Indices(std::move(indices)), m_Owner(owner), m_Lifespan(lifespan) {}

	MeshComponent* m_Component;
	std::vector<Vertex> m_Vertices;
	std::vector<Index> m_Indices;
	EntityID m_Owner;
	ObjectLifespan m_Lifespan;
};
```

- [ ] **Step 3: Implement resource table methods in `IGraphicsService.cpp`**

```cpp
GPUMeshResourceHandle IGraphicsService::AllocateMeshResource(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_MeshResourceLUT.find(name);
	if (l_existing != m_MeshResourceLUT.end())
		return l_existing->second;

	uint32_t l_index;
	if (!m_FreeMeshResourceSlots.empty())
	{
		l_index = m_FreeMeshResourceSlots.back();
		m_FreeMeshResourceSlots.pop_back();
		m_MeshResources[l_index] = GPUMeshResource{};
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MeshResources.size());
		m_MeshResources.push_back(GPUMeshResource{});
	}

	auto& l_resource = m_MeshResources[l_index];
	l_resource.m_Lifespan = lifespan;
	l_resource.m_Name = name;

	GPUMeshResourceHandle l_handle;
	l_handle.m_Index = l_index;
	m_MeshResourceLUT.emplace(name, l_handle);

	return l_handle;
}

void IGraphicsService::ReleaseMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return;

	// Delegate D3D12 resource release to backend
	ReleaseMeshGPUResourceImpl(handle);

	m_MeshResourceLUT.erase(std::string(l_resource.m_Name.c_str()));
	l_resource = GPUMeshResource{};
	m_FreeMeshResourceSlots.push_back(handle.m_Index);
}

void IGraphicsService::ReleaseAllMeshResources(ObjectLifespan lifespan)
{
	for (uint32_t i = 0; i < m_MeshResources.size(); i++)
	{
		if (m_MeshResources[i].m_Lifespan == lifespan
			&& m_MeshResources[i].m_Status != ObjectStatus::Invalid)
		{
			GPUMeshResourceHandle l_handle;
			l_handle.m_Index = i;
			ReleaseMeshResource(l_handle);
		}
	}
}

GPUMeshResource* IGraphicsService::GetMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;
	return &m_MeshResources[handle.m_Index];
}

const GPUMeshResource* IGraphicsService::GetMeshResource(GPUMeshResourceHandle handle) const
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;
	return &m_MeshResources[handle.m_Index];
}

GPUMeshResourceHandle IGraphicsService::FindMeshResourceByName(const char* name)
{
	auto l_result = m_MeshResourceLUT.find(name);
	return (l_result != m_MeshResourceLUT.end()) ? l_result->second : INVALID_GPU_MESH_HANDLE;
}
```

Add the pure virtual for the backend to implement:

```cpp
// In IGraphicsService.h, protected:
virtual void ReleaseMeshGPUResourceImpl(GPUMeshResourceHandle handle) = 0;
```

- [ ] **Step 4: Update `Initialize(MeshComponent*, ...)` to allocate a resource handle**

In `IGraphicsService.cpp`, update the `Initialize` method:

```cpp
void IGraphicsService::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices,
                                   std::vector<Index>& indices, EntityID owner)
{
	if (mesh->m_GPUResource.IsValid())
		return;

	ObjectLifespan l_lifespan = ObjectLifespan::Scene;
	if (owner != INVALID_ENTITY)
	{
		auto l_registry = g_Engine->Get<EntityRegistry>();
		l_lifespan = l_registry->GetLifespan(owner);
	}

	auto l_handle = AllocateMeshResource(mesh->m_InstanceName.c_str(), l_lifespan);
	mesh->m_GPUResource = l_handle;

	auto* l_resource = GetMeshResource(l_handle);
	l_resource->m_AABB = Math::GenerateAABB(vertices.data(), vertices.size());

	m_uninitializedMeshes.push(MeshInitTask(mesh, std::move(vertices), std::move(indices), owner, l_lifespan));
}
```

- [ ] **Step 5: Rewrite `InitializeComponents()` mesh processing block**

In the mesh task processing loop (currently lines 854-881), update to write to the resource table:

```cpp
// Mesh init tasks
while (m_uninitializedMeshes.size() > 0)
{
	MeshInitTask l_task(nullptr, std::vector<Vertex>(), std::vector<Index>());
	if (!m_uninitializedMeshes.tryPop(l_task))
		break;

	MeshComponent* l_meshComp = l_task.m_Component;
	if (l_task.m_Owner != INVALID_ENTITY)
	{
		MeshComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MeshComponent>(l_task.m_Owner);
		if (l_current)
			l_meshComp = l_current;
		else
		{
			Log(Warning, "MeshInitTask: entity ", l_task.m_Owner,
			    " no longer has MeshComponent, using stored pointer");
		}
	}

	auto* l_resource = GetMeshResource(l_meshComp->m_GPUResource);
	if (!l_resource)
	{
		Log(Error, "MeshInitTask: invalid GPU resource handle for ", l_meshComp->m_InstanceName);
		continue;
	}

	if (InitializeImpl(l_meshComp->m_GPUResource, l_task.m_Vertices, l_task.m_Indices))
	{
		l_resource->m_Status = ObjectStatus::Activated;
		l_meshComp->m_ObjectStatus = ObjectStatus::Activated;
	}
	else
	{
		m_uninitializedMeshes.push(std::move(l_task));
	}
}
```

- [ ] **Step 6: Rewrite `OnSceneUnloading()` mesh path**

Replace the per-entity `Delete(l_mesh)` loop (lines 111-119) with bulk release:

```cpp
bool IGraphicsService::OnSceneUnloading()
{
	ReleaseAllMeshResources(ObjectLifespan::Scene);

	// Reset mesh components on scene entities (their handles now point to released resources)
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_sceneEntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
	for (auto l_entityID : l_sceneEntityIDs)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entityID);
		if (l_mesh)
			l_mesh->m_ObjectStatus = ObjectStatus::Invalid;

		auto* l_material = l_registry->Get<MaterialComponent>(l_entityID);
		if (l_material && l_material->m_ObjectStatus == ObjectStatus::Activated)
		{
			l_material->m_ObjectStatus = ObjectStatus::Invalid;
			Delete(l_material);
		}
	}

	// ... rest of task queue purging unchanged ...
```

- [ ] **Step 7: Update `AddMeshComponent` and `FindMeshByName`**

`AddMeshComponent` currently uses `AllocateGPUHandle` with the pool. It needs to still return a `MeshComponent*` for pool-allocated mesh components (used by render passes). For now, keep the pool path for non-ECS mesh components but remove mesh entries from `GPUHandlePools`:

```cpp
MeshComponent* IGraphicsService::AddMeshComponent(const char* name)
{
	// Pool path remains for render-pass mesh components (e.g., full-screen quad)
	// These are NOT in the resource table — they're direct pool allocations
	return AllocateGPUHandle(m_GPUHandlePools.Meshes,
	                         m_GPUHandlePools.MeshLUT,
	                         m_GPUHandlePools.MeshPointers, name);
}
```

Wait — on reflection, `AddMeshComponent` is used by render passes that create their own meshes (e.g., full-screen quads) via the pool. These meshes get `InitializeImpl` called directly with their pool pointer as key. They don't go through ECS at all.

**Decision: Keep the pool for render-pass mesh components for now.** The resource table handles ECS mesh components (template + scene). A future cleanup task can migrate pool meshes to the resource table. Remove the `MeshLUT` and `MeshPointers` from `GPUHandlePools` since `FindMeshByName` should use the resource table for ECS meshes.

Actually, let's keep `AddMeshComponent` unchanged for this task. The pool still serves render-pass meshes. The resource table is the new path for ECS meshes.

- [ ] **Step 8: Change `InitializeImpl` signature for mesh**

In `IGraphicsService.h`:

```cpp
virtual bool InitializeImpl(GPUMeshResourceHandle handle,
                            std::vector<Vertex>& vertices,
                            std::vector<Index>& indices) = 0;
```

Keep the old `InitializeImpl(MeshComponent*, ...)` signature temporarily as a deleted overload to catch accidental usage at compile time:

```cpp
bool InitializeImpl(MeshComponent*, std::vector<Vertex>&, std::vector<Index>&) = delete;
```

- [ ] **Step 9: Commit**

```bash
git add Source/Engine/Services/IGraphicsService.h Source/Engine/Services/Common/IGraphicsService.cpp
git commit -m "refactor: add GPUMeshResourceTable with lifespan-based lifetime management

Mesh GPU resources are now allocated into a resource table indexed by
GPUMeshResourceHandle. Each entry is tagged with ObjectLifespan.
OnSceneUnloading bulk-releases Scene-lifespan resources instead of
per-entity Delete calls.

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 3: Update DX12 Backend — InitializeImpl and Delete

**Files:**
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService.h`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_ComponentPool.cpp`

- [ ] **Step 1: Change DX12 side-maps from component-pointer keys to handle keys**

In `DX12GraphicsService.h`, replace the 6 mesh maps:

```cpp
// Before:
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshVertexBuffers_Upload;
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshVertexBuffers_Default;
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshIndexBuffers_Upload;
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshIndexBuffers_Default;
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshBLAS;
std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_MeshScratchBuffers;

// After:
struct DX12MeshGPUResources
{
    ComPtr<ID3D12Resource> m_VertexBuffer_Upload;
    ComPtr<ID3D12Resource> m_VertexBuffer_Default;
    ComPtr<ID3D12Resource> m_IndexBuffer_Upload;
    ComPtr<ID3D12Resource> m_IndexBuffer_Default;
    ComPtr<ID3D12Resource> m_BLAS;
    ComPtr<ID3D12Resource> m_ScratchBuffer;
};
std::unordered_map<uint32_t, DX12MeshGPUResources> m_DX12MeshResources;
```

Update the method signatures:

```cpp
bool InitializeImpl(GPUMeshResourceHandle handle,
                    std::vector<Vertex>& vertices,
                    std::vector<Index>& indices) override;
void ReleaseMeshGPUResourceImpl(GPUMeshResourceHandle handle) override;

// Keep the old Delete(MeshComponent*) for pool-managed meshes used by render passes
bool Delete(MeshComponent* mesh) override;
```

Also add:

```cpp
bool UploadToGPU(CommandListComponent* commandList, GPUMeshResourceHandle handle);
```

- [ ] **Step 2: Rewrite `InitializeImpl` for mesh in `DX12GraphicsService_EngineComponent_Protected.cpp`**

Change the function to write to the resource table + DX12 resource map:

```cpp
bool DX12GraphicsService::InitializeImpl(GPUMeshResourceHandle handle,
                                          std::vector<Vertex>& vertices,
                                          std::vector<Index>& indices)
{
	auto* l_resource = GetMeshResource(handle);
	if (!l_resource)
		return false;

	auto l_handleIndex = handle.m_Index;
	DX12MeshGPUResources l_dx12Resources = {};

	// Vertex buffer
	auto l_verticesDataSize = uint32_t(sizeof(Vertex) * vertices.size());
	auto l_verticesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_verticesDataSize);

	l_dx12Resources.m_VertexBuffer_Default = CreateDefaultHeapBuffer(&l_verticesResourceDesc);
	if (!l_dx12Resources.m_VertexBuffer_Default)
	{
		Log(Error, l_resource->m_Name, " can't create vertex buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_VertexBuffer_Default, "DefaultHeap_VB");
#endif

	l_dx12Resources.m_VertexBuffer_Upload = CreateUploadHeapBuffer(&l_verticesResourceDesc);
	if (!l_dx12Resources.m_VertexBuffer_Upload)
	{
		Log(Error, l_resource->m_Name, " can't create vertex buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_VertexBuffer_Upload, "UploadHeap_VB");
#endif

	l_resource->m_VertexBufferView.m_BufferLocation = l_dx12Resources.m_VertexBuffer_Default->GetGPUVirtualAddress();
	l_resource->m_VertexBufferView.m_SizeInBytes = l_verticesDataSize;
	l_resource->m_VertexBufferView.m_StrideInBytes = sizeof(Vertex);

	// Index buffer
	auto l_indicesDataSize = uint32_t(sizeof(Index) * indices.size());
	auto l_indicesResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(l_indicesDataSize);

	l_dx12Resources.m_IndexBuffer_Default = CreateDefaultHeapBuffer(&l_indicesResourceDesc);
	if (!l_dx12Resources.m_IndexBuffer_Default)
	{
		Log(Error, l_resource->m_Name, " can't create index buffer on Default Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_IndexBuffer_Default, "DefaultHeap_IB");
#endif

	l_dx12Resources.m_IndexBuffer_Upload = CreateUploadHeapBuffer(&l_indicesResourceDesc);
	if (!l_dx12Resources.m_IndexBuffer_Upload)
	{
		Log(Error, l_resource->m_Name, " can't create index buffer on Upload Heap!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_IndexBuffer_Upload, "UploadHeap_IB");
#endif

	l_resource->m_IndexBufferView.m_BufferLocation = l_dx12Resources.m_IndexBuffer_Default->GetGPUVirtualAddress();
	l_resource->m_IndexBufferView.m_SizeInBytes = l_indicesDataSize;
	l_resource->m_IndexBufferView.m_StrideInBytes = sizeof(Index);

	// Flip y texture coordinate
	for (auto& i : vertices)
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;

	// Map and copy
	CD3DX12_RANGE l_readRange(0, 0);
	l_dx12Resources.m_VertexBuffer_Upload->Map(0, &l_readRange, &l_resource->m_MappedMemory_VB);
	l_dx12Resources.m_IndexBuffer_Upload->Map(0, &l_readRange, &l_resource->m_MappedMemory_IB);

	std::memcpy(l_resource->m_MappedMemory_VB, vertices.data(), vertices.size() * sizeof(Vertex));
	std::memcpy(l_resource->m_MappedMemory_IB, indices.data(), indices.size() * sizeof(Index));

	// Upload command list
	CommandListComponent l_commandList = {};
	l_commandList.m_Type = GPUEngineType::Graphics;
	auto l_dx12CommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT,
		GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), L"MeshInitCommandList");
	l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_VertexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_IndexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

	l_dx12CommandList->CopyResource(l_dx12Resources.m_VertexBuffer_Default.Get(),
	                                l_dx12Resources.m_VertexBuffer_Upload.Get());
	l_dx12CommandList->CopyResource(l_dx12Resources.m_IndexBuffer_Default.Get(),
	                                l_dx12Resources.m_IndexBuffer_Upload.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_VertexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_IndexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	// BLAS
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = l_dx12Resources.m_VertexBuffer_Default->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = l_dx12Resources.m_IndexBuffer_Default->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(indices.size());
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geometryDesc;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
	{
		Log(Error, l_resource->m_Name, " Failed to get prebuild info for BLAS!");
		return false;
	}

	auto blasResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
		prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	l_dx12Resources.m_BLAS = CreateDefaultHeapBuffer(&blasResourceDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	if (!l_dx12Resources.m_BLAS)
	{
		Log(Error, l_resource->m_Name, " Failed to create BLAS buffer!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_BLAS, "BLAS");
#endif

	auto scratchResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
		prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	l_dx12Resources.m_ScratchBuffer = CreateDefaultHeapBuffer(&scratchResourceDesc);
	if (!l_dx12Resources.m_ScratchBuffer)
	{
		Log(Error, l_resource->m_Name, " Failed to create scratch buffer for BLAS!");
		return false;
	}
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(l_resource->m_Name, l_dx12Resources.m_ScratchBuffer, "ScratchBuffer_BLAS");
#endif

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_IndexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_VertexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = l_dx12Resources.m_ScratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = l_dx12Resources.m_BLAS->GetGPUVirtualAddress();

	l_dx12CommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_dx12Resources.m_BLAS.Get());

	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_IndexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		l_dx12Resources.m_VertexBuffer_Default.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	Close(&l_commandList, GPUEngineType::Graphics);
	Execute(&l_commandList, GPUEngineType::Graphics);
	SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
	auto l_semaphoreValue = GetSemaphoreValue(GPUEngineType::Graphics);
	WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);

	m_DX12MeshResources[l_handleIndex] = std::move(l_dx12Resources);

	Log(Verbose, l_resource->m_Name, " mesh GPU resources initialized.");

	return true;
}
```

- [ ] **Step 3: Implement `ReleaseMeshGPUResourceImpl` in `DX12GraphicsService_ComponentPool.cpp`**

```cpp
void DX12GraphicsService::ReleaseMeshGPUResourceImpl(GPUMeshResourceHandle handle)
{
	auto it = m_DX12MeshResources.find(handle.m_Index);
	if (it != m_DX12MeshResources.end())
	{
		// ComPtr Reset releases the D3D12 resources via COM ref counting
		it->second.m_VertexBuffer_Upload.Reset();
		it->second.m_VertexBuffer_Default.Reset();
		it->second.m_IndexBuffer_Upload.Reset();
		it->second.m_IndexBuffer_Default.Reset();
		it->second.m_BLAS.Reset();
		it->second.m_ScratchBuffer.Reset();
		m_DX12MeshResources.erase(it);
	}
}
```

- [ ] **Step 4: Update `UploadToGPU` for mesh to use resource handle**

```cpp
bool DX12GraphicsService::UploadToGPU(CommandListComponent* commandList, GPUMeshResourceHandle handle)
{
	auto it = m_DX12MeshResources.find(handle.m_Index);
	if (it == m_DX12MeshResources.end())
		return false;

	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	l_DX12CommandList->CopyResource(it->second.m_VertexBuffer_Default.Get(),
	                                it->second.m_VertexBuffer_Upload.Get());
	l_DX12CommandList->CopyResource(it->second.m_IndexBuffer_Default.Get(),
	                                it->second.m_IndexBuffer_Upload.Get());

	return true;
}
```

Note: The old `UploadToGPU(CommandListComponent*, MeshComponent*)` is no longer called from `InitializeImpl` since the new `InitializeImpl` inlines the upload. Keep the old overload for now but it can be removed in a cleanup pass.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Services/DX12/DX12GraphicsService.h \
        Source/Engine/Services/DX12/DX12GraphicsService_EngineComponent_Protected.cpp \
        Source/Engine/Services/DX12/DX12GraphicsService_ComponentPool.cpp
git commit -m "refactor: DX12 mesh init writes to resource table, consolidate 6 side-maps

Replace 6 separate unordered_maps keyed by component pointer with a
single DX12MeshGPUResources struct keyed by resource handle index.
InitializeImpl now takes GPUMeshResourceHandle. ReleaseMeshGPUResourceImpl
releases D3D12 resources via ComPtr::Reset.

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 4: Update DrawIndexedInstanced to Resolve Handle

**Files:**
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp`
- Modify: `Source/Engine/Services/IGraphicsService.h` (signature)

- [ ] **Step 1: Update `DrawIndexedInstanced` to resolve the resource handle**

In `DX12GraphicsService_CommandListAPI.cpp` (currently line 507-534):

```cpp
bool DX12GraphicsService::DrawIndexedInstanced(RenderPassComponent* renderPass,
                                                CommandListComponent* commandList,
                                                MeshComponent* mesh,
                                                size_t instanceCount)
{
	auto* l_resource = GetMeshResource(mesh->m_GPUResource);
	if (!l_resource || l_resource->m_Status != ObjectStatus::Activated)
		return false;

	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = l_resource->m_VertexBufferView.m_BufferLocation;
	vbv.StrideInBytes = l_resource->m_VertexBufferView.m_StrideInBytes;
	vbv.SizeInBytes = l_resource->m_VertexBufferView.m_SizeInBytes;

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = l_resource->m_IndexBufferView.m_BufferLocation;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = l_resource->m_IndexBufferView.m_SizeInBytes;

	l_DX12CommandList->IASetVertexBuffers(0, 1, &vbv);
	l_DX12CommandList->IASetIndexBuffer(&ibv);
	l_DX12CommandList->DrawIndexedInstanced(
		l_resource->GetIndexCount(), static_cast<uint32_t>(instanceCount), 0, 0, 0);

	return true;
}
```

- [ ] **Step 2: Commit**

```bash
git add Source/Engine/Services/DX12/DX12GraphicsService_CommandListAPI.cpp
git commit -m "refactor: DrawIndexedInstanced resolves mesh handle via resource table

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 5: Update DrawCallService to Resolve Handle

**Files:**
- Modify: `Source/Engine/Services/DrawCallService.cpp`

- [ ] **Step 1: Update `BuildDrawCalls` to resolve resource handle**

Replace direct MeshComponent field reads (currently lines 147-170) with handle resolution:

```cpp
const MeshComponent& l_mesh = l_Meshes[i];

if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
	continue;

auto* l_resource = l_graphicsService->GetMeshResource(l_mesh.m_GPUResource);
if (!l_resource || l_resource->m_Status != ObjectStatus::Activated)
	continue;

// ... material/visibility checks unchanged ...

GPUModelData l_gpuModelData = {};

l_gpuModelData.m_VertexBufferAddress = l_resource->m_VertexBufferView.m_BufferLocation;
l_gpuModelData.m_IndexBufferAddress = l_resource->m_IndexBufferView.m_BufferLocation;

if (l_resource->m_VertexBufferView.m_StrideInBytes == 0)
{
	Log(Error, "Vertex stride is zero - cannot calculate vertex count");
	l_gpuModelData.m_VertexCount = 0;
}
else
{
	l_gpuModelData.m_VertexCount = l_resource->m_VertexBufferView.m_SizeInBytes
		/ l_resource->m_VertexBufferView.m_StrideInBytes;
}
l_gpuModelData.m_IndexCount = l_resource->GetIndexCount();
l_gpuModelData.m_VertexStride = l_resource->m_VertexBufferView.m_StrideInBytes;
l_gpuModelData.m_IndexStride = l_resource->m_IndexBufferView.m_StrideInBytes;

// ... material index, UUID, visibility unchanged ...

const AABB& l_localAabb = l_vis ? l_vis->m_AABB : l_resource->m_AABB;
```

- [ ] **Step 2: Commit**

```bash
git add Source/Engine/Services/DrawCallService.cpp
git commit -m "refactor: DrawCallService resolves mesh GPU resource handle for draw data

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 6: Update Scene Loading and Template Assets

**Files:**
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp`
- Modify: `Source/Engine/Services/TemplateAssetService.cpp`
- Modify: `Source/DefaultClient/LogicClient/World.inl`

- [ ] **Step 1: Update `JSONSerializer_Components.cpp` — template mesh copy path**

The template copy (line 135) now just copies the handle:

```cpp
bool JSONWrapper::Load(const char* fileName, MeshComponent& component, EntityID owner)
{
	json j;
	if (!Load(fileName, j))
		return false;

	MeshShape l_meshShape = MeshShape(j["MeshShape"]);
	if (l_meshShape != MeshShape::Customized)
	{
		auto* l_templateMesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(l_meshShape);
		component.m_GPUResource = l_templateMesh->m_GPUResource;
		component.m_ObjectStatus = l_templateMesh->m_ObjectStatus;
		return true;
	}

	// ... custom mesh path unchanged (reads binary, calls Initialize) ...
}
```

- [ ] **Step 2: Update `World.inl` — `attachMeshAndMaterial`**

```cpp
void WorldSystem::attachMeshAndMaterial(EntityID Entity, MeshShape Shape)
{
	auto l_templateMesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(Shape);
	auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();

	if (!l_templateMesh || !l_defaultMaterial)
	{
		Log(Error, "TemplateAssetService returned null mesh or material for entity ", Entity);
		return;
	}

	auto l_Registry = g_Engine->Get<EntityRegistry>();

	auto& l_mesh = l_Registry->Emplace<MeshComponent>(Entity);
	l_mesh.m_GPUResource = l_templateMesh->m_GPUResource;
	l_mesh.m_ObjectStatus = l_templateMesh->m_ObjectStatus;

	auto& l_material = l_Registry->Emplace<MaterialComponent>(Entity);
	l_material.m_materialAttributes = l_defaultMaterial->m_materialAttributes;
	l_material.m_ShaderModel = l_defaultMaterial->m_ShaderModel;
	l_material.m_ObjectStatus = ObjectStatus::Activated;
}
```

Also update `processPendingMeshSetups` validation:

```cpp
void WorldSystem::processPendingMeshSetups()
{
	if (m_PendingMeshSetups.empty())
		return;

	auto l_sphereMesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Sphere);
	if (!l_sphereMesh || !l_sphereMesh->m_GPUResource.IsValid())
		return;

	// Check the resource is actually initialized
	auto* l_resource = g_Engine->getGraphicsService()->GetMeshResource(l_sphereMesh->m_GPUResource);
	if (!l_resource || l_resource->m_Status != ObjectStatus::Activated)
		return;

	for (auto& l_pair : m_PendingMeshSetups)
		attachMeshAndMaterial(l_pair.first, l_pair.second);

	m_PendingMeshSetups.clear();
}
```

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp \
        Source/DefaultClient/LogicClient/World.inl
git commit -m "refactor: scene loading and World.inl copy mesh handle instead of GPU fields

Template mesh copy now transfers GPUMeshResourceHandle — a uint32 index.
No GPU resource ownership transferred. Scene entities reference persistent
template resources via the handle.

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 7: Update Rendering Clients

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp`
- Verify: `Source/DefaultClient/RenderingClient/BSDFTestPass.cpp` (likely no change — uses `DrawIndexedInstanced`)
- Verify: `Source/DefaultClient/RenderingClient/SurfelGITestPass.cpp` (likely no change)

- [ ] **Step 1: Update `GPUPathTracerPass.cpp` — resolve handle for mapped memory access**

The path tracer reads `m_MappedMemory_VB`, `m_MappedMemory_IB`, and buffer view fields directly. Update to resolve the handle:

```cpp
const MeshComponent& l_mesh = l_meshes[i];

if (l_mesh.m_ObjectStatus != ObjectStatus::Activated)
	continue;

auto* l_resource = l_graphicsService->GetMeshResource(l_mesh.m_GPUResource);
if (!l_resource || l_resource->m_Status != ObjectStatus::Activated)
	continue;

if (!l_resource->m_MappedMemory_VB || !l_resource->m_MappedMemory_IB)
	continue;

const uint32_t l_vertexStride = l_resource->m_VertexBufferView.m_StrideInBytes;
const uint32_t l_indexStride  = l_resource->m_IndexBufferView.m_StrideInBytes;

if (l_vertexStride == 0 || l_indexStride == 0)
	continue;

const uint32_t l_vertexCount = l_resource->m_VertexBufferView.m_SizeInBytes / l_vertexStride;
const uint32_t l_indexCount  = l_resource->GetIndexCount();

// ... rest unchanged, but use l_resource->m_MappedMemory_VB and l_resource->m_MappedMemory_IB
const uint8_t* l_vbPtr = static_cast<const uint8_t*>(l_resource->m_MappedMemory_VB);
// ...
const uint8_t* l_ibPtr = static_cast<const uint8_t*>(l_resource->m_MappedMemory_IB);
```

- [ ] **Step 2: Verify `BSDFTestPass.cpp` — no field access changes needed**

`BSDFTestPass.cpp` just calls `GetMeshComponent()` and passes the pointer to `DrawIndexedInstanced()`. Since `DrawIndexedInstanced` now resolves the handle internally, no changes needed.

- [ ] **Step 3: Verify `SurfelGITestPass.cpp` — no field access changes needed**

Gets a mesh pointer and passes it around. No direct field access beyond what `DrawIndexedInstanced` handles.

- [ ] **Step 4: Commit**

```bash
git add Source/DefaultClient/RenderingClient/GPUPathTracerPass.cpp
git commit -m "refactor: GPUPathTracerPass resolves mesh handle for vertex/index data access

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 8: Update Headless and Other Graphics Service Backends

**Files:**
- Modify: `Source/Engine/Services/Headless/HeadlessGraphicsService.h`
- Modify: `Source/Engine/Services/Headless/HeadlessGraphicsService.cpp`
- Modify: `Source/Engine/Services/VK/VKGraphicsService.h` (stub)
- Modify: `Source/Engine/Services/MT/MTGraphicsService.h` (stub)

- [ ] **Step 1: Add `ReleaseMeshGPUResourceImpl` stub to Headless backend**

```cpp
// HeadlessGraphicsService.h
void ReleaseMeshGPUResourceImpl(GPUMeshResourceHandle handle) override;

// HeadlessGraphicsService.cpp
void HeadlessGraphicsService::ReleaseMeshGPUResourceImpl(GPUMeshResourceHandle handle)
{
	// Headless — no GPU resources to release
}
```

Also add `InitializeImpl(GPUMeshResourceHandle, ...)` override that returns true (headless).

- [ ] **Step 2: Add stubs to VK and MT backends**

Same pattern — empty implementations that return true / do nothing.

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Services/Headless/ Source/Engine/Services/VK/ Source/Engine/Services/MT/
git commit -m "refactor: add ReleaseMeshGPUResourceImpl stubs to Headless/VK/MT backends

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 9: Build, Fix Compilation Errors, Test

- [ ] **Step 1: Full build**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
```

Fix any remaining compilation errors from missed field accesses. Common patterns to grep for:

```bash
grep -rn "m_VertexBufferView\|m_IndexBufferView\|m_MappedMemory_VB\|m_MappedMemory_IB\|GetIndexCount" \
  Source/Engine Source/DefaultClient Source/Tool --include="*.cpp" --include="*.h" \
  | grep -v "GPUMeshResource\|External\|GPUBufferView"
```

Any remaining hits on `MeshComponent` fields that were removed need updating.

- [ ] **Step 2: Run GPU validation test**

```
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code 0

- [ ] **Step 3: Run interactive test**

```
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 1' -Wait -PassThru -NoNewWindow).ExitCode"
```

Verify: window renders, no D3D12 validation errors, no crash on scene load/unload.

- [ ] **Step 4: Final commit for any build fixes**

```bash
git add -A
git commit -m "fix: resolve remaining compilation issues from mesh resource indirection refactor

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```

---

## Task 10: Clean Up Legacy Mesh Pool Infrastructure

After the refactor is working, remove dead code.

**Files:**
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`
- Modify: `Source/Engine/Services/DX12/DX12GraphicsService_ComponentPool.cpp`

- [ ] **Step 1: Audit whether `AddMeshComponent` pool path is still used**

Search for `AddMeshComponent` callers:

```bash
grep -rn "AddMeshComponent" Source/ --include="*.cpp" --include="*.h" | grep -v External
```

If no callers remain (render passes may have been using it), remove:
- `TObjectPool<MeshComponent>* Meshes` from `GPUHandlePools`
- `MeshLUT` and `MeshPointers` from `GPUHandlePools`
- Pool init/terminate for Meshes
- `Delete(MeshComponent*)` from DX12 backend (replaced by `ReleaseMeshGPUResourceImpl`)

If callers remain, leave the pool for now and document the cleanup in a follow-up task.

- [ ] **Step 2: Remove old `UploadToGPU(CommandListComponent*, MeshComponent*)` if unused**

- [ ] **Step 3: Remove `SetObjectName` overload that takes `MeshComponent*` if it exists**

Check whether `SetObjectName` has an overload for `MeshComponent*` or uses the component's name directly. Update to use `ObjectName` from the resource.

- [ ] **Step 4: Commit cleanup**

```bash
git add -A
git commit -m "refactor: remove legacy mesh pool infrastructure superseded by resource table

Code-AI-Generated-By: Claude (Anthropic AI Assistant)"
```
