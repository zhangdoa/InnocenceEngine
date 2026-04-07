# Per-Type Resource Services Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the GraphicsResourceService monolith with 8 per-type resource services, each with a NamedObjectPool and a DX12 backend implementation.

**Architecture:** Create `NamedObjectPool<T>` to unify pool allocation, name-indexed dedup, and live-object iteration into one container. Each resource type gets its own service (base + DX12 impl) following a uniform shape. FrameManagementService coordinates per-frame initialization by calling each service's drain function. All 60+ callers are migrated from `GraphicsResourceService` to the appropriate per-type service.

**Tech Stack:** C++17, DX12, engine memory system (`TObjectPool`), engine threading primitives (`ThreadSafeUnorderedMap`, `ThreadSafeVector`, `ThreadSafeQueue`)

---

## File Structure

### New files to create

| File | Responsibility |
|------|---------------|
| `Source/Engine/Common/NamedObjectPool.h` | Generic pool with name index and live-object tracking |
| `Source/Engine/Services/TextureResourceService.h` | Texture pool, deferred init queue, Find, ForEach |
| `Source/Engine/Services/Common/TextureResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12TextureResourceService.h` | DX12 texture init, SRV/UAV creation, upload, mipmap, readback |
| `Source/Engine/Services/DX12/DX12TextureResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/GPUBufferResourceService.h` | GPUBuffer pool, deferred init, Upload, WriteMappedMemory |
| `Source/Engine/Services/Common/GPUBufferResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12GPUBufferResourceService.h` | DX12 buffer init, SRV/UAV/CBV creation, upload |
| `Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/MeshResourceService.h` | Mesh pool, GPUMeshResource slots, deferred init |
| `Source/Engine/Services/Common/MeshResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12MeshResourceService.h` | DX12 mesh init, BLAS creation |
| `Source/Engine/Services/DX12/DX12MeshResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/MaterialResourceService.h` | Material pool, deferred init, Find |
| `Source/Engine/Services/Common/MaterialResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12MaterialResourceService.h` | DX12 material init (thin) |
| `Source/Engine/Services/DX12/DX12MaterialResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/RenderPassResourceService.h` | RenderPass pool, output merger, PSO, semaphore, fence lifecycle |
| `Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12RenderPassResourceService.h` | DX12 render pass init, root signature, PSO creation |
| `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/ShaderProgramResourceService.h` | ShaderProgram pool, sync init |
| `Source/Engine/Services/Common/ShaderProgramResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12ShaderProgramResourceService.h` | DX12 shader loading |
| `Source/Engine/Services/DX12/DX12ShaderProgramResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/SamplerResourceService.h` | Sampler pool, sync init |
| `Source/Engine/Services/Common/SamplerResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12SamplerResourceService.h` | DX12 sampler creation |
| `Source/Engine/Services/DX12/DX12SamplerResourceService.cpp` | DX12 implementation |
| `Source/Engine/Services/CommandListResourceService.h` | CommandList pool, sync init, ForEach |
| `Source/Engine/Services/Common/CommandListResourceServiceImpl.cpp` | Base implementation |
| `Source/Engine/Services/DX12/DX12CommandListResourceService.h` | DX12 command list creation |
| `Source/Engine/Services/DX12/DX12CommandListResourceService.cpp` | DX12 implementation |

### Files to delete

| File | Reason |
|------|--------|
| `Source/Engine/Services/GraphicsResourceService.h` | Replaced by per-type services |
| `Source/Engine/Services/Common/GraphicsResourceServiceImpl.cpp` | Replaced by per-type service impls |
| `Source/Engine/Services/DX12/DX12GraphicsResourceService.h` | Replaced by per-type DX12 impls |
| `Source/Engine/Services/DX12/DX12GraphicsResourceService.cpp` | Replaced by per-type DX12 impls |

### Files to modify

| File | Changes |
|------|---------|
| `Source/Engine/Engine.cpp` | Create 8 services instead of 1, register in singleton map |
| `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` | Call each service's InitializeComponents, use per-type services for swap chain resources |
| `Source/Engine/Services/FrameManagementService.h` | Replace `GraphicsResourceService*` with per-type service pointers (or use `g_Engine->Get<>()`) |
| `Source/Engine/Services/DX12/DX12FrameManagementService.cpp` | Update raytracing references |
| `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` | Update `AddSemaphore` call |
| `Source/Engine/Services/GraphicsHardwareService.h` | Remove `GraphicsResourceService` forward decl if unused |
| 31 render pass `.cpp` files in `Source/DefaultClient/RenderingClient/` | Replace `GraphicsResourceService` calls with per-type service calls |
| `Source/TestClient/TestRenderingClient.cpp` | Same migration |
| `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiRendererDX12.cpp` | Same migration |
| `Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp` | Same migration |
| `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` | Same migration |
| `Source/Engine/Services/SceneService.cpp` | Call per-type OnSceneUnloading |
| `Source/Engine/Services/DrawCallService.cpp` | Use GPUBufferResourceService + TextureResourceService |
| `Source/Engine/Services/PerFrameDataService.cpp` | Use GPUBufferResourceService |
| `Source/Engine/Services/LightDataService.cpp` | Use GPUBufferResourceService |
| `Source/Engine/Services/BillboardDrawCallService.cpp` | Use GPUBufferResourceService |
| `Source/Engine/Services/AnimationDrawCallService.cpp` | Use GPUBufferResourceService |
| `Source/Engine/Services/AnimationResourceService.cpp` | Use GPUBufferResourceService |
| `Source/Engine/RayTracer/RayTracer.cpp` | Use TextureResourceService |
| `Source/Editor/worldexplorer.cpp` | Use appropriate per-type service |
| CMakeLists or `.vcxproj` files | Add new source files, remove old ones |

---

## Task Decomposition

This refactor is decomposed into phases. Each phase produces a buildable, testable state. **The key constraint**: we cannot delete `GraphicsResourceService` until all callers are migrated and all functionality is moved. So we build the new services alongside the old one, migrate callers incrementally, then delete the old code.

### Phase 1: Foundation (NamedObjectPool)
### Phase 2: Create per-type services (one at a time, with DX12 impl)
### Phase 3: Wire services into Engine.cpp and FrameManagementService
### Phase 4: Migrate callers (bulk sed + manual fixup)
### Phase 5: Delete GraphicsResourceService

---

### Task 1: Create NamedObjectPool\<T\>

**Files:**
- Create: `Source/Engine/Common/NamedObjectPool.h`

- [ ] **Step 1: Create NamedObjectPool header**

```cpp
#pragma once
#include "ObjectPool.h"
#include "ThreadSafeUnorderedMap.h"
#include "ThreadSafeVector.h"
#include "LogService.h"

namespace Inno
{
	template <typename T>
	class NamedObjectPool
	{
	public:
		NamedObjectPool() = default;

		void Initialize(uint32_t capacity)
		{
			m_Pool = TObjectPool<T>::Create(capacity);
		}

		void Terminate()
		{
			m_NameIndex.clear();
			m_LiveObjects.clear();
			TObjectPool<T>::Destruct(m_Pool);
			m_Pool = nullptr;
		}

		T* Allocate(const char* name)
		{
			if (!name || name[0] == '\0')
			{
				Log(Error, "NamedObjectPool: name cannot be empty.");
				return nullptr;
			}

			auto l_existing = m_NameIndex.find(name);
			if (l_existing != m_NameIndex.end())
				return l_existing->second;

			auto l_ptr = m_Pool->Spawn();
			if (!l_ptr)
			{
				Log(Error, "NamedObjectPool: pool exhausted for name: ", name);
				return nullptr;
			}

			l_ptr->m_ObjectStatus = ObjectStatus::Created;
			l_ptr->m_InstanceName = ObjectName(name);

			m_NameIndex.emplace(name, l_ptr);
			m_LiveObjects.emplace_back(l_ptr);
			return l_ptr;
		}

		void Release(T* ptr)
		{
			if (!ptr) return;
			m_NameIndex.erase(std::string(ptr->m_InstanceName.c_str()));
			m_LiveObjects.eraseByValue(ptr);
			m_Pool->Destroy(ptr);
		}

		T* Find(const char* name)
		{
			auto l_result = m_NameIndex.find(name);
			return (l_result != m_NameIndex.end()) ? l_result->second : nullptr;
		}

		void ForEach(std::function<void(T*)> func)
		{
			m_LiveObjects.for_each(func);
		}

	private:
		TObjectPool<T>* m_Pool = nullptr;
		ThreadSafeUnorderedMap<std::string, T*> m_NameIndex;
		ThreadSafeVector<T*> m_LiveObjects;
	};
}
```

- [ ] **Step 2: Build**

Run: `powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"`
Expected: Clean build (header-only, no callers yet)

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Common/NamedObjectPool.h
git commit -m "feat: add NamedObjectPool<T> for unified pool+name+iteration"
```

---

### Task 2: Create CommandListResourceService + DX12 impl

This is the simplest service (no deferred init, no complex DX12 state). Good to establish the pattern.

**Files:**
- Create: `Source/Engine/Services/CommandListResourceService.h`
- Create: `Source/Engine/Services/Common/CommandListResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12CommandListResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12CommandListResourceService.cpp`

- [ ] **Step 1: Create CommandListResourceService.h**

```cpp
#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	class CommandListResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(CommandListResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		CommandListComponent* Add(const char* name);
		virtual bool Delete(CommandListComponent* ptr);

		void ForEach(std::function<void(CommandListComponent*)> func);

		void Initialize(CommandListComponent* commandList);

	protected:
		virtual bool InitializeImpl(CommandListComponent* commandList) { return false; }

		NamedObjectPool<CommandListComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
```

- [ ] **Step 2: Create CommandListResourceServiceImpl.cpp**

```cpp
#include "../CommandListResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool CommandListResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(256);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "CommandListResourceService Setup finished.");
	return true;
}

bool CommandListResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "CommandListResourceService has been terminated.");
	return true;
}

CommandListComponent* CommandListResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool CommandListResourceService::Delete(CommandListComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void CommandListResourceService::ForEach(std::function<void(CommandListComponent*)> func)
{
	m_Pool.ForEach(func);
}

void CommandListResourceService::Initialize(CommandListComponent* commandList)
{
	InitializeImpl(commandList);
}
```

- [ ] **Step 3: Create DX12CommandListResourceService.h**

```cpp
#pragma once
#include "../CommandListResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12CommandListResourceService : public CommandListResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12CommandListResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Delete(CommandListComponent* ptr) override;

	protected:
		bool InitializeImpl(CommandListComponent* commandList) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
```

- [ ] **Step 4: Create DX12CommandListResourceService.cpp**

Move `DX12GraphicsResourceService::InitializeImpl(CommandListComponent*)` (lines 1001-1059) and `DX12GraphicsResourceService::Delete(CommandListComponent*)` (lines 155-164) from `DX12GraphicsResourceService.cpp` into this file:

```cpp
#include "DX12CommandListResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool DX12CommandListResourceService::Delete(CommandListComponent* ptr)
{
	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(ptr->m_CommandList);
	if (l_dx12CommandList)
		l_dx12CommandList->Release();

	CommandListResourceService::Delete(ptr);
	return true;
}

bool DX12CommandListResourceService::InitializeImpl(CommandListComponent* commandList)
{
	if (!commandList)
	{
		Log(Error, "CommandList parameter is null");
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	ComPtr<ID3D12GraphicsCommandList7> l_commandList;
	HRESULT l_HResult;

	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Compute:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	case GPUEngineType::Copy:
		l_HResult = m_ctx->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
			m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, l_currentFrame).Get(), nullptr,
			IID_PPV_ARGS(&l_commandList));
		break;
	default:
		Log(Error, commandList->m_InstanceName, " Unknown GPU engine type for command list creation");
		return false;
	}

	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to create DX12 command list");
		return false;
	}

	l_HResult = l_commandList->Close();
	if (FAILED(l_HResult))
	{
		Log(Error, commandList->m_InstanceName, " Failed to close command list after creation");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(commandList, l_commandList, "CommandList");
#endif

	commandList->m_CommandList = reinterpret_cast<uint64_t>(l_commandList.Detach());
	commandList->m_ObjectStatus = ObjectStatus::Activated;

	Log(Verbose, commandList->m_InstanceName, " Command list created successfully");
	return true;
}
```

- [ ] **Step 5: Add new files to the CMake/vcxproj build**

Add the 4 new files to the appropriate project file so they compile.

- [ ] **Step 6: Build**

Run: `powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"`
Expected: Clean build (services exist but aren't wired into Engine yet)

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Services/CommandListResourceService.h \
        Source/Engine/Services/Common/CommandListResourceServiceImpl.cpp \
        Source/Engine/Services/DX12/DX12CommandListResourceService.h \
        Source/Engine/Services/DX12/DX12CommandListResourceService.cpp
git commit -m "feat: add CommandListResourceService with DX12 backend"
```

---

### Task 3: Create SamplerResourceService + DX12 impl

Same pattern as Task 2. No deferred init.

**Files:**
- Create: `Source/Engine/Services/SamplerResourceService.h`
- Create: `Source/Engine/Services/Common/SamplerResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12SamplerResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12SamplerResourceService.cpp`

- [ ] **Step 1: Create SamplerResourceService.h**

```cpp
#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Component/SamplerComponent.h"

namespace Inno
{
	class SamplerResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(SamplerResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		SamplerComponent* Add(const char* name);
		virtual bool Delete(SamplerComponent* ptr);

		void Initialize(SamplerComponent* sampler);

	protected:
		virtual bool InitializeImpl(SamplerComponent* sampler) { return false; }

		NamedObjectPool<SamplerComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
```

- [ ] **Step 2: Create SamplerResourceServiceImpl.cpp**

```cpp
#include "../SamplerResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool SamplerResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(256);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "SamplerResourceService Setup finished.");
	return true;
}

bool SamplerResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "SamplerResourceService has been terminated.");
	return true;
}

SamplerComponent* SamplerResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool SamplerResourceService::Delete(SamplerComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void SamplerResourceService::Initialize(SamplerComponent* sampler)
{
	InitializeImpl(sampler);
}
```

- [ ] **Step 3: Create DX12SamplerResourceService.h**

```cpp
#pragma once
#include "../SamplerResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12SamplerResourceService : public SamplerResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12SamplerResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

	protected:
		bool InitializeImpl(SamplerComponent* sampler) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
```

- [ ] **Step 4: Create DX12SamplerResourceService.cpp**

Move `DX12GraphicsResourceService::InitializeImpl(SamplerComponent*)` (lines 753-783) from `DX12GraphicsResourceService.cpp`:

```cpp
#include "DX12SamplerResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12SamplerResourceService::InitializeImpl(SamplerComponent* sampler)
{
	sampler->m_GPUResourceType = GPUResourceType::Sampler;

	D3D12_SAMPLER_DESC l_samplerDesc = {};
	l_samplerDesc.Filter = GetFilterMode(sampler->m_SamplerDesc.m_MinFilterMethod, sampler->m_SamplerDesc.m_MagFilterMethod);
	l_samplerDesc.AddressU = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodU);
	l_samplerDesc.AddressV = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodV);
	l_samplerDesc.AddressW = GetWrapMode(sampler->m_SamplerDesc.m_WrapMethodW);
	l_samplerDesc.MipLODBias = 0.0f;
	l_samplerDesc.MaxAnisotropy = sampler->m_SamplerDesc.m_MaxAnisotropy;
	l_samplerDesc.BorderColor[0] = sampler->m_SamplerDesc.m_BorderColor[0];
	l_samplerDesc.BorderColor[1] = sampler->m_SamplerDesc.m_BorderColor[1];
	l_samplerDesc.BorderColor[2] = sampler->m_SamplerDesc.m_BorderColor[2];
	l_samplerDesc.BorderColor[3] = sampler->m_SamplerDesc.m_BorderColor[3];
	l_samplerDesc.MinLOD = sampler->m_SamplerDesc.m_MinLOD;
	l_samplerDesc.MaxLOD = sampler->m_SamplerDesc.m_MaxLOD;

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	sampler->m_ReadHandles.resize(l_swapChainImageCount);
	for (auto& handle : sampler->m_ReadHandles)
	{
		handle = m_ctx->m_SamplerDescHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateSampler(&l_samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ handle.m_CPUHandle });
	}

	sampler->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}
```

- [ ] **Step 5: Add to build, build, commit**

```bash
git add Source/Engine/Services/SamplerResourceService.h \
        Source/Engine/Services/Common/SamplerResourceServiceImpl.cpp \
        Source/Engine/Services/DX12/DX12SamplerResourceService.h \
        Source/Engine/Services/DX12/DX12SamplerResourceService.cpp
git commit -m "feat: add SamplerResourceService with DX12 backend"
```

---

### Task 4: Create ShaderProgramResourceService + DX12 impl

No deferred init. DX12 impl is large (shader loading) but self-contained.

**Files:**
- Create: `Source/Engine/Services/ShaderProgramResourceService.h`
- Create: `Source/Engine/Services/Common/ShaderProgramResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12ShaderProgramResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12ShaderProgramResourceService.cpp`

- [ ] **Step 1: Create ShaderProgramResourceService.h**

Same shape as CommandListResourceService but for `ShaderProgramComponent`. Pool size 256.

- [ ] **Step 2: Create ShaderProgramResourceServiceImpl.cpp**

Same pattern. `Initialize(ShaderProgramComponent*)` calls `InitializeImpl`.

- [ ] **Step 3: Create DX12ShaderProgramResourceService.h**

```cpp
#pragma once
#include "../ShaderProgramResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12ShaderProgramResourceService : public ShaderProgramResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12ShaderProgramResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

	protected:
		bool InitializeImpl(ShaderProgramComponent* shaderProgram) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
```

- [ ] **Step 4: Create DX12ShaderProgramResourceService.cpp**

Move `DX12GraphicsResourceService::InitializeImpl(ShaderProgramComponent*)` (lines 605-747) — the entire shader loading function including both DXIL and non-DXIL paths. Also move the static `LoadShaderFile` helper functions that it depends on (these are currently file-local statics in DX12GraphicsResourceService.cpp — find them and move them into this file or a shared DX12 helper).

- [ ] **Step 5: Add to build, build, commit**

---

### Task 5: Create TextureResourceService + DX12 impl

Deferred init. DX12 impl includes SRV/UAV creation, upload, mipmap generation, Clear, Copy, GetIndex, ReadTextureBackToCPU.

**Files:**
- Create: `Source/Engine/Services/TextureResourceService.h`
- Create: `Source/Engine/Services/Common/TextureResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12TextureResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12TextureResourceService.cpp`

- [ ] **Step 1: Create TextureResourceService.h**

```cpp
#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/TextureComponent.h"
#include "../Common/EntityID.h"
#include "../Common/Math.h"

namespace Inno
{
	class TextureResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(TextureResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		TextureComponent* Add(const char* name);
		virtual bool Delete(TextureComponent* ptr);
		TextureComponent* Find(const char* name);

		void Initialize(TextureComponent* texture, void* textureData = nullptr, EntityID owner = INVALID_ENTITY);
		bool InitializeComponents();
		bool OnSceneUnloading();

		virtual bool Clear(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) { return false; }
		virtual bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) { return false; }
		virtual std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) { return std::nullopt; }
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) { return {}; }

	protected:
		virtual bool InitializeImpl(TextureComponent* texture, void* textureData) { return false; }

		NamedObjectPool<TextureComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	private:
		struct TextureInitTask
		{
			TextureInitTask(TextureComponent* component, void* textureData, EntityID owner = INVALID_ENTITY)
				: m_Component(component), m_TextureData(textureData), m_Owner(owner) {}

			TextureComponent* m_Component;
			void* m_TextureData;
			EntityID m_Owner;
		};

		ThreadSafeQueue<TextureInitTask> m_DeferredQueue;
	};
}
```

- [ ] **Step 2: Create TextureResourceServiceImpl.cpp**

Pool size from `RenderingCapability::maxTextures`. `InitializeComponents()` drains `m_DeferredQueue` same as current code in `GraphicsResourceServiceImpl.cpp` lines 483-506. `OnSceneUnloading()` drains scene-bound textures from queue.

- [ ] **Step 3: Create DX12TextureResourceService.h**

Override `InitializeImpl`, `Delete`, `Clear`, `Copy`, `GenerateMipmap`, `GetIndex`, `ReadTextureBackToCPU`. Own the mipmap generator (m_2DMipmapRootSignature/PSO, m_3DMipmapRootSignature/PSO) and texture buffer maps (m_TextureBuffers_Upload, m_TextureBuffers_Default). Also own `CreateMipmapGenerator()` / `ReleaseMipmapGenerator()`.

- [ ] **Step 4: Create DX12TextureResourceService.cpp**

Move from DX12GraphicsResourceService.cpp:
- `InitializeImpl(TextureComponent*, void*)` (lines 403-599)
- `Delete(TextureComponent*)` (lines 95-116)
- `CreateSRV(TextureComponent*, uint32_t)` (lines 1574-1599)
- `CreateUAV(TextureComponent*, uint32_t)` (lines 1601-1636)
- `Clear(CommandListComponent*, TextureComponent*)` (lines 1135-1173)
- `Copy(CommandListComponent*, TextureComponent*, TextureComponent*)` (lines 1116-1133)
- `GenerateMipmap(TextureComponent*, CommandListComponent*)` (lines 1420-1568)
- `GetIndex(TextureComponent*, Accessibility)` (lines 1179-1202)
- `ReadTextureBackToCPU(RenderPassComponent*, TextureComponent*)` (lines 1209-1414)
- `CreateMipmapGenerator()` (lines 2298-2369)
- `ReleaseMipmapGenerator()` (lines 2417-2425)

- [ ] **Step 5: Add to build, build, commit**

---

### Task 6: Create GPUBufferResourceService + DX12 impl

Deferred init. Owns Upload, WriteMappedMemory, UploadToGPU, Clear(GPUBuffer). Also owns raytracing buffer creation (TLAS, scratch, instance buffer).

**Files:**
- Create: `Source/Engine/Services/GPUBufferResourceService.h`
- Create: `Source/Engine/Services/Common/GPUBufferResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12GPUBufferResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12GPUBufferResourceService.cpp`

- [ ] **Step 1: Create GPUBufferResourceService.h**

Pool size from `RenderingCapability::maxBuffers`. Has deferred queue. Owns `Upload<T>` template, `WriteMappedMemory`. Owns raytracing buffer pointers (m_TLASBufferComponent, m_ScratchBufferComponent, m_RaytracingInstanceBufferComponent) and the TLAS-ready flag. Has `ForEach` for FrameManagementService iteration.

- [ ] **Step 2: Create GPUBufferResourceServiceImpl.cpp**

Move from GraphicsResourceServiceImpl.cpp:
- `Initialize(GPUBufferComponent*)` → deferred queue push
- `InitializeComponents()` → drain GPU buffer queue
- `WriteMappedMemory()` (lines 234-253)
- `Upload<T>` template stays in header
- `GetTLASBuffer()` (line 255-258)
- `OnSceneUnloading()` → no scene-bound GPU buffers to drain, but keep for consistency

- [ ] **Step 3: Create DX12GPUBufferResourceService.h**

Override `InitializeImpl`, `Delete`, `UploadToGPU`, `Clear`. Own `CreateRaytracingResources()` / `ReleaseRaytracingResources()`. Own SRV/UAV/CBV creation helpers, raytracing instance descs.

- [ ] **Step 4: Create DX12GPUBufferResourceService.cpp**

Move from DX12GraphicsResourceService.cpp:
- `InitializeImpl(GPUBufferComponent*)` (lines 789-938)
- `Delete(GPUBufferComponent*)` (lines 123-144)
- `UploadToGPU(CommandListComponent*, GPUBufferComponent*)` (lines 1072-1081)
- `UploadToGPU(CommandListComponent*, DX12MappedMemory*, DX12DeviceMemory*, GPUBufferComponent*)` (lines 1083-1093)
- `Clear(CommandListComponent*, GPUBufferComponent*)` (lines 1095-1114)
- `CreateSRV(GPUBufferComponent*)` (lines 1638-1660)
- `CreateUAV(GPUBufferComponent*)` (lines 1662-1698)
- `CreateCBV(GPUBufferComponent*)` (lines 1700-1721)
- `CreateRaytracingResources()` (lines 2371-2406)
- `ReleaseRaytracingResources()` (lines 2408-2415)
- `InitializeImpl(EntityID)` (lines 944-995) — entity RT instance registration
- `OnSceneLoadingStart()` (lines 2278-2292) — clear RT descs

- [ ] **Step 5: Add to build, build, commit**

---

### Task 7: Create MeshResourceService + DX12 impl

Deferred init. Owns GPUMeshResource slot management.

**Files:**
- Create: `Source/Engine/Services/MeshResourceService.h`
- Create: `Source/Engine/Services/Common/MeshResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12MeshResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12MeshResourceService.cpp`

- [ ] **Step 1: Create MeshResourceService.h**

Pool size from `RenderingCapability::maxMeshes`. Has deferred queue with MeshInitTask. Owns GPUMeshResource vector, free slot list, mesh resource LUT. Has `GetMeshResource`, `FindMeshResourceByName`.

- [ ] **Step 2: Create MeshResourceServiceImpl.cpp**

Move from GraphicsResourceServiceImpl.cpp:
- `Initialize(MeshComponent*, vertices, indices, owner)` (lines 363-393)
- `InitializeComponents()` mesh drain loop (lines 448-481)
- `AllocateMeshResource()` (lines 260-289)
- `ReleaseMeshResource()` (lines 291-307)
- `ReleaseAllMeshResources()` (lines 309-320)
- `GetMeshResource()` (lines 322-344)
- `FindMeshResourceByName()` (lines 346-352)
- `OnSceneUnloading()` → release scene-bound mesh resources, drain scene-bound mesh queue

- [ ] **Step 3: Create DX12MeshResourceService.h and .cpp**

Move from DX12GraphicsResourceService.cpp:
- `InitializeImpl(MeshAssetHandle, vertices, indices)` (lines 201-397)
- `ReleaseMeshGPUResourceImpl(MeshAssetHandle)` (lines 80-93)
- `Delete(MeshComponent*)` (lines 72-78)
- Own `m_DX12MeshResources` map

- [ ] **Step 4: Add to build, build, commit**

---

### Task 8: Create MaterialResourceService + DX12 impl

Deferred init. Thin DX12 impl.

**Files:**
- Create: `Source/Engine/Services/MaterialResourceService.h`
- Create: `Source/Engine/Services/Common/MaterialResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12MaterialResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12MaterialResourceService.cpp`

- [ ] **Step 1: Create MaterialResourceService.h**

Pool size from `RenderingCapability::maxMaterials`. Has deferred queue. Has `Find`.

- [ ] **Step 2: Create MaterialResourceServiceImpl.cpp**

Move from GraphicsResourceServiceImpl.cpp:
- `Initialize(MaterialComponent*, owner)` (lines 404-411)
- `InitializeComponents()` material drain loop (lines 508-531)
- `FindMaterialByName()` (lines 228-232)
- `OnSceneUnloading()` → drain scene-bound materials

Base `InitializeImpl(MaterialComponent*)` returns true (current behavior).

- [ ] **Step 3: Create DX12MaterialResourceService.h and .cpp**

DX12 Delete is a no-op (returns true). Keep the base class Delete which calls pool Release.

- [ ] **Step 4: Add to build, build, commit**

---

### Task 9: Create RenderPassResourceService + DX12 impl

The most complex service. Owns output merger targets, PSO pool, semaphore pool, fence events. Depends on TextureResourceService, ShaderProgramResourceService, SamplerResourceService.

**Files:**
- Create: `Source/Engine/Services/RenderPassResourceService.h`
- Create: `Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp`
- Create: `Source/Engine/Services/DX12/DX12RenderPassResourceService.h`
- Create: `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`

- [ ] **Step 1: Create RenderPassResourceService.h**

```cpp
#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/RenderPassComponent.h"
#include "../Common/Math.h"

namespace Inno
{
	class TextureResourceService;

	class RenderPassResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(RenderPassResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		RenderPassComponent* Add(const char* name);
		virtual bool Delete(RenderPassComponent* ptr);

		void ForEach(std::function<void(RenderPassComponent*)> func);

		void Initialize(RenderPassComponent* renderPass);
		bool InitializeComponents();

		bool InitializeRenderPass(RenderPassComponent* renderPass);
		bool CreateOutputMergerTargets(RenderPassComponent* renderPass);
		bool InitializeOutputMergerTargets(RenderPassComponent* renderPass);
		bool DeleteRenderTargets(RenderPassComponent* renderPass);

		virtual IPipelineStateObject* AddPipelineStateObject() = 0;
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;
		virtual bool Delete(IPipelineStateObject* rhs) = 0;
		virtual bool Delete(ISemaphore* rhs) = 0;
		virtual bool Delete(IOutputMergerTarget* rhs) = 0;

		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) { return Vec4(); }

	protected:
		virtual bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) { return false; }
		virtual bool CreatePipelineStateObject(RenderPassComponent* renderPass) { return false; }
		virtual bool CreateFenceEvents(RenderPassComponent* renderPass) { return false; }

		NamedObjectPool<RenderPassComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	private:
		ThreadSafeQueue<RenderPassComponent*> m_DeferredQueue;
	};
}
```

- [ ] **Step 2: Create RenderPassResourceServiceImpl.cpp**

Move from GraphicsResourceServiceImpl.cpp:
- `Initialize(RenderPassComponent*)` → deferred queue push (lines 432-439)
- `InitializeComponents()` → drain render pass queue (lines 548-561)
- `InitializeRenderPass()` (lines 672-696)
- `CreateOutputMergerTargets()` (lines 581-617) — uses `g_Engine->Get<TextureResourceService>()->Add()` for render targets
- `InitializeOutputMergerTargets()` (lines 619-670) — uses `g_Engine->Get<TextureResourceService>()` for texture init
- `DeleteRenderTargets()` (lines 698-713)

- [ ] **Step 3: Create DX12RenderPassResourceService.h**

Own PSO pool, semaphore pool, output merger target pool. Override all the DX12-specific methods. Move root signature creation, graphics/compute/raytracing PSO creation, fence event creation, OnOutputMergerTargetsCreated (RTV/DSV creation).

- [ ] **Step 4: Create DX12RenderPassResourceService.cpp**

Move from DX12GraphicsResourceService.cpp:
- Pool management: `AddPipelineStateObject`, `AddSemaphore`, `Add(IOutputMergerTarget)`, `Delete(IPipelineStateObject)`, `Delete(ISemaphore)`, `Delete(IOutputMergerTarget)`
- `CreateRootSignature()` (lines 1727-1885)
- `GetDescriptorRange()` (lines 1887-1947)
- `CreatePipelineStateObject()` (lines 1953-1994)
- `CreateGraphicsPipelineStateObject()` (lines 1996-2044)
- `CreateRaytracingPipelineStateObject()` (lines 2046-2181)
- `CreateFenceEvents()` (lines 2183-2217)
- `OnOutputMergerTargetsCreated()` (lines 2219-2276)
- `ReadRenderTargetSample()` (lines 1204-1207)
- Helper statics: `LoadGraphicsShaders`, `LoadComputeShaders`, `LoadRaytracingShaders`, `GenerateDepthStencilStateDesc`, `GenerateBlendStateDesc`, `GenerateRasterizerStateDesc`, `GenerateViewportStateDesc`, `CreateInputLayout` — all from DX12Helper_Pipeline.h or file-local statics

- [ ] **Step 5: Add to build, build, commit**

---

### Task 10: Wire all services into Engine.cpp

**Files:**
- Modify: `Source/Engine/Engine.cpp`

- [ ] **Step 1: Create all 8 DX12 services in Engine.cpp**

Replace the current `DX12GraphicsResourceService` creation with 8 per-type services. Each gets `SetDX12Context(l_ctx)`. Register each in the singleton map:

```cpp
// Replace:
// auto* l_rsService = new DX12GraphicsResourceService();
// l_rsService->SetDX12Context(l_ctx);

auto* l_cmdListService = new DX12CommandListResourceService();
l_cmdListService->SetDX12Context(l_ctx);
auto* l_samplerService = new DX12SamplerResourceService();
l_samplerService->SetDX12Context(l_ctx);
auto* l_shaderService = new DX12ShaderProgramResourceService();
l_shaderService->SetDX12Context(l_ctx);
auto* l_textureService = new DX12TextureResourceService();
l_textureService->SetDX12Context(l_ctx);
auto* l_gpuBufferService = new DX12GPUBufferResourceService();
l_gpuBufferService->SetDX12Context(l_ctx);
auto* l_meshService = new DX12MeshResourceService();
l_meshService->SetDX12Context(l_ctx);
auto* l_materialService = new DX12MaterialResourceService();
l_materialService->SetDX12Context(l_ctx);
auto* l_renderPassService = new DX12RenderPassResourceService();
l_renderPassService->SetDX12Context(l_ctx);

singletons_[std::type_index(typeid(CommandListResourceService))] = l_cmdListService;
singletons_[std::type_index(typeid(SamplerResourceService))] = l_samplerService;
singletons_[std::type_index(typeid(ShaderProgramResourceService))] = l_shaderService;
singletons_[std::type_index(typeid(TextureResourceService))] = l_textureService;
singletons_[std::type_index(typeid(GPUBufferResourceService))] = l_gpuBufferService;
singletons_[std::type_index(typeid(MeshResourceService))] = l_meshService;
singletons_[std::type_index(typeid(MaterialResourceService))] = l_materialService;
singletons_[std::type_index(typeid(RenderPassResourceService))] = l_renderPassService;
```

- [ ] **Step 2: Add Setup calls for all 8 services**

In the setup block, call `Setup()` for each service. Order: CommandList, Sampler, ShaderProgram, Texture, GPUBuffer, Mesh, Material, RenderPass (render pass last since it may depend on texture/shader/sampler services during initialization).

- [ ] **Step 3: Add Terminate calls**

Reverse order of setup.

- [ ] **Step 4: Keep GraphicsResourceService alive temporarily**

During migration, both old and new services coexist. The old GraphicsResourceService singleton remains until all callers are migrated. Once migration is complete (Task 11), remove it.

- [ ] **Step 5: Build and test**

Run build + GPU validation test.

- [ ] **Step 6: Commit**

---

### Task 11: Update FrameManagementService to use per-type services

**Files:**
- Modify: `Source/Engine/Services/FrameManagementService.h`
- Modify: `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp`
- Modify: `Source/Engine/Services/DX12/DX12FrameManagementService.cpp`

- [ ] **Step 1: Replace GraphicsResourceService* with per-type service access**

In `FrameManagementServiceImpl.cpp`, replace all `m_ResourceService->` calls:
- `m_ResourceService->AddCommandListComponent(...)` → `g_Engine->Get<CommandListResourceService>()->Add(...)`
- `m_ResourceService->Initialize(l_commandList)` → `g_Engine->Get<CommandListResourceService>()->Initialize(l_commandList)`
- `m_ResourceService->AddRenderPassComponent(...)` → `g_Engine->Get<RenderPassResourceService>()->Add(...)`
- `m_ResourceService->AddShaderProgramComponent(...)` → `g_Engine->Get<ShaderProgramResourceService>()->Add(...)`
- `m_ResourceService->AddSamplerComponent(...)` → `g_Engine->Get<SamplerResourceService>()->Add(...)`
- `m_ResourceService->Initialize(m_SwapChainShaderProgramComp)` → `g_Engine->Get<ShaderProgramResourceService>()->Initialize(...)`
- `m_ResourceService->Initialize(m_SwapChainSamplerComp)` → `g_Engine->Get<SamplerResourceService>()->Initialize(...)`
- `m_ResourceService->Initialize(l_swapChainRP)` → `g_Engine->Get<RenderPassResourceService>()->Initialize(...)`
- `m_ResourceService->InitializeComponents()` → call each service's `InitializeComponents()`:
  ```cpp
  g_Engine->Get<MeshResourceService>()->InitializeComponents();
  g_Engine->Get<TextureResourceService>()->InitializeComponents();
  g_Engine->Get<MaterialResourceService>()->InitializeComponents();
  g_Engine->Get<GPUBufferResourceService>()->InitializeComponents();
  g_Engine->Get<RenderPassResourceService>()->InitializeComponents();
  ```
- `m_ResourceService->GetGPUBufferPointers()` → `g_Engine->Get<GPUBufferResourceService>()->ForEach(...)`
- `m_ResourceService->UploadToGPU(cmdList, buffer)` → `g_Engine->Get<GPUBufferResourceService>()->UploadToGPU(cmdList, buffer)`
- `m_ResourceService->GetRenderPassPointers()` → `g_Engine->Get<RenderPassResourceService>()->ForEach(...)`
- `m_ResourceService->DeleteRenderTargets(rp)` → `g_Engine->Get<RenderPassResourceService>()->DeleteRenderTargets(rp)`
- `m_ResourceService->CreateOutputMergerTargets(rp)` → `g_Engine->Get<RenderPassResourceService>()->CreateOutputMergerTargets(rp)`
- `m_ResourceService->InitializeOutputMergerTargets(rp)` → `g_Engine->Get<RenderPassResourceService>()->InitializeOutputMergerTargets(rp)`
- `m_ResourceService->OnOutputMergerTargetsCreated(rp)` → `g_Engine->Get<RenderPassResourceService>()->OnOutputMergerTargetsCreated(rp)` (make public or friend)
- `m_ResourceService->AddPipelineStateObject()` → `g_Engine->Get<RenderPassResourceService>()->AddPipelineStateObject()`
- `m_ResourceService->CreatePipelineStateObject(rp)` → `g_Engine->Get<RenderPassResourceService>()->CreatePipelineStateObject(rp)` (make public or restructure)
- Delete calls → route to appropriate per-type service

- [ ] **Step 2: Update DX12FrameManagementService.cpp**

Replace `IsTLASReady()` and raytracing buffer access with `g_Engine->Get<GPUBufferResourceService>()`.

- [ ] **Step 3: Remove `m_ResourceService` pointer from FrameManagementService.h**

Or keep it temporarily if callers still need it. Remove `SetResourceService()` setter.

- [ ] **Step 4: Build and test**

- [ ] **Step 5: Commit**

---

### Task 12: Migrate render pass callers (bulk)

**Files:**
- Modify: All 31+ render pass `.cpp` files in `Source/DefaultClient/RenderingClient/`
- Modify: `Source/TestClient/TestRenderingClient.cpp`
- Modify: `Source/Engine/ThirdParty/ImGuiWrapper/ImGuiRendererDX12.cpp`

- [ ] **Step 1: Mechanical sed replacement across all render pass files**

Every render pass currently does:
```cpp
auto l_rsService = g_Engine->Get<GraphicsResourceService>();
l_rsService->AddShaderProgramComponent("...");
l_rsService->AddRenderPassComponent("...");
l_rsService->AddTextureComponent("...");
// etc.
```

Replace with per-type service calls. The pattern is mechanical:
- `l_rsService->AddShaderProgramComponent(` → `g_Engine->Get<ShaderProgramResourceService>()->Add(`
- `l_rsService->AddRenderPassComponent(` → `g_Engine->Get<RenderPassResourceService>()->Add(`
- `l_rsService->AddTextureComponent(` → `g_Engine->Get<TextureResourceService>()->Add(`
- `l_rsService->AddSamplerComponent(` → `g_Engine->Get<SamplerResourceService>()->Add(`
- `l_rsService->AddGPUBufferComponent(` → `g_Engine->Get<GPUBufferResourceService>()->Add(`
- `l_rsService->AddCommandListComponent(` → `g_Engine->Get<CommandListResourceService>()->Add(`
- `l_rsService->Initialize(m_ShaderProgramComp)` → `g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp)`
- `l_rsService->Initialize(m_RenderPassComp)` → `g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp)`
- `l_rsService->Initialize(m_Result)` (TextureComponent*) → `g_Engine->Get<TextureResourceService>()->Initialize(m_Result)`
- `l_rsService->Initialize(m_SamplerComp)` → `g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp)`
- `l_rsService->Initialize(m_GPUBuffer*)` → `g_Engine->Get<GPUBufferResourceService>()->Initialize(m_GPUBuffer*)`
- `l_rsService->Initialize(m_CommandListComp_*)` → `g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_*)`
- `l_rsService->Delete(m_RenderPassComp)` → `g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp)`
- `l_rsService->Delete(m_ShaderProgramComp)` → `g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp)`
- `l_rsService->Delete(m_Result)` (TextureComponent*) → `g_Engine->Get<TextureResourceService>()->Delete(m_Result)`
- `l_rsService->Delete(m_CommandListComp_*)` → `g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_*)`
- `l_rsService->Upload(buffer, data)` → `g_Engine->Get<GPUBufferResourceService>()->Upload(buffer, data)`
- `l_rsService->GetTLASBuffer()` → `g_Engine->Get<GPUBufferResourceService>()->GetTLASBuffer()`
- `l_rsService->Clear(cmdList, texture)` → `g_Engine->Get<TextureResourceService>()->Clear(cmdList, texture)`
- `l_rsService->ReadTextureBackToCPU(...)` → `g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(...)`

- [ ] **Step 2: Update includes**

Replace `#include "../../Engine/Services/GraphicsResourceService.h"` with the specific per-type service headers each file needs.

- [ ] **Step 3: Remove unused `l_rsService` declarations**

Many files had a single `auto l_rsService = g_Engine->Get<GraphicsResourceService>();` that is no longer needed. Remove it.

- [ ] **Step 4: Build**

- [ ] **Step 5: Commit**

---

### Task 13: Migrate engine service callers

**Files:**
- Modify: `Source/Engine/Services/DrawCallService.cpp` — use GPUBufferResourceService + TextureResourceService
- Modify: `Source/Engine/Services/PerFrameDataService.cpp` — use GPUBufferResourceService
- Modify: `Source/Engine/Services/LightDataService.cpp` — use GPUBufferResourceService
- Modify: `Source/Engine/Services/BillboardDrawCallService.cpp` — use GPUBufferResourceService
- Modify: `Source/Engine/Services/AnimationDrawCallService.cpp` — use GPUBufferResourceService
- Modify: `Source/Engine/Services/AnimationResourceService.cpp` — use GPUBufferResourceService
- Modify: `Source/Engine/Services/SceneService.cpp` — call each service's OnSceneUnloading
- Modify: `Source/Engine/ThirdParty/AssimpWrapper/AssimpTextureProcessor.cpp` — use TextureResourceService
- Modify: `Source/Engine/ThirdParty/JSONWrapper/JSONSerializer_Components.cpp` — use per-type services
- Modify: `Source/Engine/RayTracer/RayTracer.cpp` — use TextureResourceService
- Modify: `Source/Editor/worldexplorer.cpp` — use appropriate per-type service
- Modify: `Source/Engine/Services/DX12/DX12GraphicsHardwareService.cpp` — update AddSemaphore call

- [ ] **Step 1: Migrate each file**

Same mechanical replacement pattern as Task 12. SceneService is special — its `OnSceneUnloading` call needs to fan out to each per-type service that has scene-bound resources:
```cpp
g_Engine->Get<MeshResourceService>()->OnSceneUnloading();
g_Engine->Get<TextureResourceService>()->OnSceneUnloading();
g_Engine->Get<MaterialResourceService>()->OnSceneUnloading();
```

For `DX12GraphicsHardwareService::CreateSyncPrimitives()`, the `m_ResourceService->AddSemaphore()` call becomes `g_Engine->Get<RenderPassResourceService>()->AddSemaphore()`.

- [ ] **Step 2: Update includes**

- [ ] **Step 3: Build and test**

- [ ] **Step 4: Commit**

---

### Task 14: Delete GraphicsResourceService

**Files:**
- Delete: `Source/Engine/Services/GraphicsResourceService.h`
- Delete: `Source/Engine/Services/Common/GraphicsResourceServiceImpl.cpp`
- Delete: `Source/Engine/Services/DX12/DX12GraphicsResourceService.h`
- Delete: `Source/Engine/Services/DX12/DX12GraphicsResourceService.cpp`
- Modify: `Source/Engine/Engine.cpp` — remove GraphicsResourceService creation/singleton registration
- Modify: `Source/Engine/Services/FrameManagementService.h` — remove GraphicsResourceService forward decl and pointer
- Modify: `Source/Engine/Services/GraphicsHardwareService.h` — remove GraphicsResourceService forward decl if present
- Modify: CMakeLists/vcxproj — remove old files

- [ ] **Step 1: Remove the 4 files from the build system**

- [ ] **Step 2: Delete the files**

- [ ] **Step 3: Remove all remaining references**

Grep for `GraphicsResourceService` across the entire Source/ directory. Fix any remaining references.

- [ ] **Step 4: Remove the `Accessibility` static member definitions**

The `Accessibility::Immutable/ReadOnly/WriteOnly/ReadWrite/CopySource/CopyDestination` static definitions currently live in `GraphicsResourceServiceImpl.cpp` (lines 17-23). Move them to an appropriate location — either a dedicated `Accessibility.cpp` or into one of the per-type service impl files (e.g. the first one that gets compiled).

- [ ] **Step 5: Build and full GPU validation test**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: Build clean, exit code 0.

- [ ] **Step 6: Commit**

```bash
git commit -m "refactor: remove GraphicsResourceService monolith, all callers use per-type services"
```

---

## Notes

- **Build system**: The project uses Visual Studio `.vcxproj` files. New source files must be added to the appropriate project file. Check `Source/Engine/Services/DX12/DX12GraphicsService.vcxproj` (or similar) for the pattern.
- **VK backend**: The Vulkan backend (`Source/Engine/Services/VK/`) also inherits from GraphicsResourceService. This plan focuses on DX12 only. The VK backend will need similar treatment but is out of scope — leave VK files as-is or stub them.
- **Thread safety**: NamedObjectPool uses the same ThreadSafe containers as the current code. No change in threading model.
- **Forbidden patterns**: No direct STL includes, no raw malloc/new[], no std::cout. Use engine wrappers per CLAUDE.md.
