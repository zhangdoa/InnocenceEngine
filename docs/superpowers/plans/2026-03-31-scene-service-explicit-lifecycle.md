# SceneService Explicit Lifecycle Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the priority-sorted callback mechanism in SceneService with explicit ordered method calls for engine services and a simple (no-sort) registration list for client services.

**Architecture:** Engine services (`IGraphicsService`, `TransformService`, `PhysicsSimulationService`) gain `OnSceneUnloading()` methods called directly by `SceneService::LoadSync` in a hardcoded, well-reasoned order. `BillboardDrawCallService` gains `OnSceneLoaded()`. DefaultClient and editor services register via a simple `std::vector<std::function<void()>*>` with no priority or sorting. `PhysXWrapper`'s scene cleanup is absorbed into `PhysicsSimulationService::OnSceneUnloading()`.

**Tech Stack:** C++17, InnocenceEngine service layer

---

## Execution Order in New LoadSync

**Unloading phase (before asset load):**
1. `g_Engine->getGraphicsService()->OnSceneUnloading()` — free GPU resources, drain init queues
2. `g_Engine->Get<EntityRegistry>()->CleanUp(ObjectLifespan::Scene)` — destroy components
3. `g_Engine->Get<TransformService>()->OnSceneUnloading()` — clear hierarchy nodes
4. `g_Engine->Get<PhysicsSimulationService>()->OnSceneUnloading()` — clear BVH + PhysX actors
5. Iterate `m_sceneUnloadingCallbacks` — client callbacks (e.g. GIResolvePass::DeleteGPUBuffers)

**Load:**
6. `AssetService::LoadScene(fileName)`

**Loaded phase (after asset load):**
7. `g_Engine->Get<BillboardDrawCallService>()->OnSceneLoaded()` — refresh icon textures
8. Iterate `m_sceneLoadedCallbacks` — client callbacks (GIDataLoader, GIResolvePass, VXGIRenderer, WorldSystem, WorldExplorer)
9. `m_needUpdate = true`

---

## Task 1: Add `OnSceneUnloading()` to `IGraphicsService`

**Files:**
- Modify: `Source/Engine/Services/IGraphicsService.h`
- Modify: `Source/Engine/Services/Common/IGraphicsService.cpp`

- [ ] **Step 1: Add `OnSceneUnloading()` virtual method to IGraphicsService.h**

In `IGraphicsService.h`, in the `protected:` section near `OnSceneLoadingStart()` (line 207), add:

```cpp
virtual bool OnSceneUnloading();
```

This replaces the `m_SceneLoadingStartedCallback` lambda. Also remove the `m_SceneLoadingStartedCallback` member field (line 233):

```cpp
// Remove this line:
std::function<void()> m_SceneLoadingStartedCallback;
```

- [ ] **Step 2: Implement `IGraphicsService::OnSceneUnloading()` in IGraphicsService.cpp**

In `Source/Engine/Services/Common/IGraphicsService.cpp`, replace the `m_SceneLoadingStartedCallback` lambda assignment and the `AddSceneLoadingStartedCallback` registration call (lines 103–144) with a standalone method implementation placed after `IGraphicsService::Setup()`:

```cpp
bool IGraphicsService::OnSceneUnloading()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_sceneEntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
	for (auto l_entityID : l_sceneEntityIDs)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entityID);
		if (l_mesh && l_mesh->m_ObjectStatus == ObjectStatus::Activated)
		{
			Delete(l_mesh);
			l_mesh->m_ObjectStatus = ObjectStatus::Invalid;
		}

		auto* l_material = l_registry->Get<MaterialComponent>(l_entityID);
		if (l_material && l_material->m_ObjectStatus == ObjectStatus::Activated)
		{
			Delete(l_material);
			l_material->m_ObjectStatus = ObjectStatus::Invalid;
		}
	}

	MeshInitTask l_meshStale(nullptr, {}, {});
	while (m_uninitializedMeshes.tryPop(l_meshStale)) {}

	MaterialComponent* l_matStale = nullptr;
	while (m_uninitializedMaterials.tryPop(l_matStale)) {}

	EntityID l_entityStale = INVALID_ENTITY;
	while (m_uninitializedEntities.tryPop(l_entityStale)) {}

	OnSceneLoadingStart();
	return true;
}
```

In `IGraphicsService::Setup()`, remove the entire lambda assignment and `AddSceneLoadingStartedCallback` call:

```cpp
// Remove these lines from Setup():
m_SceneLoadingStartedCallback = [this]() { ... };
g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&m_SceneLoadingStartedCallback, -1);
```

Also remove the `#include "../../Services/SceneService.h"` from this file if it is no longer used after removal (check first — only remove if unused).

- [ ] **Step 3: Verify build compiles**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: zero errors (may have warnings about unused `SceneService` include removal).

---

## Task 2: Add `OnSceneUnloading()` to `TransformService`

**Files:**
- Modify: `Source/Engine/Services/TransformService.h`
- Modify: `Source/Engine/Services/TransformService.cpp`

- [ ] **Step 1: Add method declaration to TransformService.h**

Add `void OnSceneUnloading();` to the public interface. Remove the `m_SceneLoadingCallback` private field:

```cpp
// In public section, after Terminate():
void OnSceneUnloading();

// Remove from private:
std::function<void()> m_SceneLoadingCallback;
```

- [ ] **Step 2: Implement `OnSceneUnloading()` and remove callback registration**

In `TransformService.cpp`:

Replace the `Initialize()` body lambda + registration:
```cpp
// Remove from Initialize():
m_SceneLoadingCallback = [this]()
{
    std::fill(m_Nodes.begin(), m_Nodes.end(), HierarchyNode{});
    m_TraversalOrder.clear();
    m_HierarchyDirty = true;
};
g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&m_SceneLoadingCallback, 0);
```

Add a new free method implementation before `Terminate()`:
```cpp
void TransformService::OnSceneUnloading()
{
	std::fill(m_Nodes.begin(), m_Nodes.end(), HierarchyNode{});
	m_TraversalOrder.clear();
	m_HierarchyDirty = true;
}
```

Remove the `#include "SceneService.h"` from `TransformService.cpp` if it is no longer used after removal.

- [ ] **Step 3: Verify build compiles**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: zero errors.

---

## Task 3: Add `OnSceneUnloading()` to `PhysicsSimulationService` (consolidating PhysXWrapper)

**Files:**
- Modify: `Source/Engine/Services/PhysicsSimulationService.h`
- Modify: `Source/Engine/Services/PhysicsSimulationService.cpp`
- Modify: `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.h`
- Modify: `Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp`

- [ ] **Step 1: Add `OnSceneUnloading()` to `PhysXWrapper`**

In `PhysXWrapper.h`, add:
```cpp
bool OnSceneUnloading();
```

In `PhysXWrapper.cpp`, replace the `f_sceneLoadingStartedCallback` lambda assignment and `AddSceneLoadingStartedCallback` call with a standalone method:

```cpp
bool PhysXWrapper::OnSceneUnloading()
{
	m_needSimulate = false;

	Log(Verbose, "Removing all PhysX Actors...");

	for (auto i : PhysXActors)
	{
		gScene->removeActor(*i.m_PxRigidActor);
	}

	PhysXActors.clear();

	Log(Success, "All PhysX Actors have been removed.");
	return true;
}
```

Remove `f_sceneLoadingStartedCallback` member field from `PhysXWrapper`'s impl struct and remove the `AddSceneLoadingStartedCallback` registration call.

Remove `#include "../../Services/SceneService.h"` from `PhysXWrapper.cpp` if unused after removal.

- [ ] **Step 2: Add `OnSceneUnloading()` to `PhysicsSimulationService.h`**

```cpp
void OnSceneUnloading();
```

- [ ] **Step 3: Implement `PhysicsSimulationService::OnSceneUnloading()`**

In `PhysicsSimulationService.cpp`, remove the `f_SceneLoadingStartedCallback` lambda + registration from `PhysicsSimulationServiceImpl::Setup()`:

```cpp
// Remove:
f_SceneLoadingStartedCallback = [&]() { ... };
g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 1);
```

Also remove the `f_SceneLoadingStartedCallback` field from `PhysicsSimulationServiceImpl`.

Add the forwarding method to `PhysicsSimulationService`:
```cpp
void PhysicsSimulationService::OnSceneUnloading()
{
	Log(Verbose, "Clearing all physics simulation data...");

	g_Engine->Get<BVHService>()->ClearNodes();
	m_Impl->CreateRootComponent();
	m_Impl->m_TotalSceneBoundary.Reset();
	m_Impl->m_StaticSceneBoundary.Reset();
	m_Impl->m_VisibleSceneBoundary.Reset();

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().OnSceneUnloading();
#endif

	Log(Success, "All physics simulation data has been cleared.");
}
```

- [ ] **Step 4: Verify build compiles**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: zero errors.

---

## Task 4: Add `OnSceneLoaded()` to `BillboardDrawCallService`

**Files:**
- Modify: `Source/Engine/Services/BillboardDrawCallService.h`
- Modify: `Source/Engine/Services/BillboardDrawCallService.cpp`

- [ ] **Step 1: Add `OnSceneLoaded()` to header**

In `BillboardDrawCallService.h`, add to public interface:
```cpp
void OnSceneLoaded();
```

- [ ] **Step 2: Implement and remove callback registration**

In `BillboardDrawCallService.cpp` (`BillboardDrawCallServiceImpl`):

Remove from `Setup()`:
```cpp
// Remove:
f_SceneLoadingFinishedCallback = [&]() { ... };
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_SceneLoadingFinishedCallback, 0);
```

Also remove `f_SceneLoadingFinishedCallback` from `BillboardDrawCallServiceImpl` struct and any `std::function<void()>` field.

Add a standalone impl method:
```cpp
void BillboardDrawCallServiceImpl::OnSceneLoaded()
{
	m_BillboardPassDrawCallInfoVector.resize(3);
	m_BillboardPassDrawCallInfoVector[0].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
	m_BillboardPassDrawCallInfoVector[1].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
	m_BillboardPassDrawCallInfoVector[2].iconTexture = g_Engine->Get<TemplateAssetService>()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
}
```

In `BillboardDrawCallService::Setup()`, call `m_Impl->OnSceneLoaded()` directly (replacing the previous `f_SceneLoadingFinishedCallback()` direct call), and add `OnSceneLoaded()` as a public forwarder:

```cpp
void BillboardDrawCallService::OnSceneLoaded()
{
	m_Impl->OnSceneLoaded();
}
```

Keep the `m_Impl->OnSceneLoaded()` call in `Setup()` for initial setup (replaces the old `f_SceneLoadingFinishedCallback()` invocation).

Remove `#include "SceneService.h"` from `BillboardDrawCallService.cpp` if unused after removal.

- [ ] **Step 3: Verify build compiles**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: zero errors.

---

## Task 5: Refactor `SceneService` — replace callbacks with explicit calls + simple lists

**Files:**
- Modify: `Source/Engine/Services/SceneService.h`
- Modify: `Source/Engine/Services/SceneService.cpp`

- [ ] **Step 1: Rewrite SceneService.h**

Replace the header with:
```cpp
#pragma once
#include "../Interface/IService.h"
#include "../Common/EntityID.h"

namespace Inno
{
	class SceneService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(SceneService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		std::string GetCurrentSceneName();
		bool Load(const char* fileName, bool AsyncLoad = false);
		bool Save(const char* fileName);
		bool IsLoading();

		bool AddSceneUnloadingCallback(std::function<void()>* functor);
		bool AddSceneLoadedCallback(std::function<void()>* functor);

	private:
		bool LoadAsync(const char* fileName);
		bool LoadSync(const char* fileName);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<std::function<void()>*> m_sceneUnloadingCallbacks;
		std::vector<std::function<void()>*> m_sceneLoadedCallbacks;

		std::atomic<bool> m_IsLoading = false;
		std::atomic<bool> m_prepareForLoadingScene = false;

		std::string m_nextLoadingScene;
		std::string m_currentScene;

		std::atomic<bool> m_needUpdate = true;
	};
}
```

- [ ] **Step 2: Rewrite SceneService.cpp**

Replace the full file with:
```cpp
#include "SceneService.h"
#include "../Common/LogService.h"
#include "AssetService.h"
#include "EntityRegistry.h"
#include "IGraphicsService.h"
#include "TransformService.h"
#include "PhysicsSimulationService.h"
#include "BillboardDrawCallService.h"

#include "../Engine.h"
using namespace Inno;

bool SceneService::LoadAsync(const char* fileName)
{
	if (!m_IsLoading)
	{
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool SceneService::LoadSync(const char* fileName)
{
	m_IsLoading = true;

	m_currentScene = fileName;

	Log(Verbose, "Loading scene ", fileName, "...");

	// Unloading phase — order is critical:
	// 1. Free GPU resources first (while component pointers still valid)
	g_Engine->getGraphicsService()->OnSceneUnloading();

	// 2. Destroy scene-scoped components
	g_Engine->Get<EntityRegistry>()->CleanUp(ObjectLifespan::Scene);
	Log(Success, "Scene entities cleaned up.");

	// 3. Clear transform hierarchy (nodes index into now-empty storage, safe to reset)
	g_Engine->Get<TransformService>()->OnSceneUnloading();

	// 4. Clear physics simulation state and PhysX actors
	g_Engine->Get<PhysicsSimulationService>()->OnSceneUnloading();

	// 5. Client unloading callbacks (e.g. GIResolvePass::DeleteGPUBuffers)
	for (auto* cb : m_sceneUnloadingCallbacks)
		(*cb)();

	// Load the new scene
	AssetService::LoadScene(fileName);

	Log(Verbose, "Scene ", fileName, " has been loaded.");

	// Loaded phase:
	// 6. Refresh engine service state that depends on loaded scene data
	g_Engine->Get<BillboardDrawCallService>()->OnSceneLoaded();

	// 7. Client loaded callbacks (GIDataLoader, GIResolvePass, VXGIRenderer, WorldSystem, Editor)
	for (auto* cb : m_sceneLoadedCallbacks)
		(*cb)();

	m_needUpdate = true;
	m_IsLoading = false;

	Log(Success, "Scene ", fileName, " has been loaded.");

	return true;
}

bool SceneService::Setup(IServiceConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool SceneService::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "SceneService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "Object is not created!");
		return false;
	}
}

bool SceneService::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		if (m_prepareForLoadingScene)
		{
			m_prepareForLoadingScene = false;

			LoadSync(m_nextLoadingScene.c_str());
		}
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool SceneService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "SceneService has been terminated.");
	return true;
}

ObjectStatus SceneService::GetStatus()
{
	return m_ObjectStatus;
}

std::string SceneService::GetCurrentSceneName()
{
	if (m_currentScene.empty())
	{
		return "Untitled";
	}

	std::string l_SceneName = m_currentScene;

	auto l_ExtensionPos = l_SceneName.find(".InnoScene");
	if (l_ExtensionPos != std::string::npos)
	{
		l_SceneName = l_SceneName.substr(0, l_ExtensionPos);
	}

	auto l_LastSlashPos = l_SceneName.rfind("//");
	if (l_LastSlashPos != std::string::npos)
	{
		l_SceneName = l_SceneName.substr(l_LastSlashPos + 2);
	}
	else
	{
		l_LastSlashPos = l_SceneName.rfind('/');
		if (l_LastSlashPos != std::string::npos)
			l_SceneName = l_SceneName.substr(l_LastSlashPos + 1);
	}

	return l_SceneName.empty() ? "Untitled" : l_SceneName;
}

bool SceneService::Load(const char* fileName, bool AsyncLoad)
{
	if (m_currentScene == fileName)
	{
		Log(Warning, "Scene ", fileName, " has already loaded now.");
		return true;
	}

	if (m_nextLoadingScene == fileName)
	{
		Log(Warning, "Scene ", fileName, " has been scheduled for loading.");
		return true;
	}

	if (AsyncLoad)
	{
		return LoadAsync(fileName);
	}
	else
	{
		return LoadSync(fileName);
	}
}

bool SceneService::Save(const char* fileName)
{
	if (!strcmp(fileName, ""))
	{
		return AssetService::SaveScene(m_currentScene.c_str());
	}
	else
	{
		return AssetService::SaveScene(fileName);
	}
}

bool SceneService::IsLoading()
{
	return m_IsLoading;
}

bool SceneService::AddSceneUnloadingCallback(std::function<void()>* functor)
{
	m_sceneUnloadingCallbacks.push_back(functor);
	return true;
}

bool SceneService::AddSceneLoadedCallback(std::function<void()>* functor)
{
	m_sceneLoadedCallbacks.push_back(functor);
	return true;
}
```

- [ ] **Step 3: Verify build compiles**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: Many compile errors from all sites still calling `AddSceneLoadingStartedCallback` / `AddSceneLoadingFinishedCallback` — these are fixed in Task 6.

---

## Task 6: Update all client/editor call sites to use new API

**Files:**
- Modify: `Source/DefaultClient/RenderingClient/GIResolvePass.cpp`
- Modify: `Source/DefaultClient/RenderingClient/GIDataLoader.cpp`
- Modify: `Source/DefaultClient/RenderingClient/VXGIRenderer.cpp`
- Modify: `Source/DefaultClient/LogicClient/World.inl`
- Modify: `Source/Editor/worldexplorer.cpp`

- [ ] **Step 1: Update GIResolvePass.cpp**

Find in `GIResolvePass::Setup()`:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_sceneLoadingStartedCallback, 0);
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);
```
Replace with:
```cpp
g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneLoadingStartedCallback);
g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);
```

- [ ] **Step 2: Update GIDataLoader.cpp**

Find:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);
```
Replace with:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);
```

- [ ] **Step 3: Update VXGIRenderer.cpp**

Find:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);
```
Replace with:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);
```

- [ ] **Step 4: Update World.inl**

Find:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);
```
Replace with:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);
```

- [ ] **Step 5: Update worldexplorer.cpp**

Find:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishCallback, 2);
```
Replace with:
```cpp
g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishCallback);
```

- [ ] **Step 6: Verify build compiles clean**

```
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK
```

Expected: zero errors.

---

## Task 7: Runtime Test

- [ ] **Step 1: Run GPU validation test**

```
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
```

Expected: exit code 0

- [ ] **Step 2: Check the latest log for scene reload errors**

```
$latest = Get-ChildItem 'C:\GitRepo\InnocenceEngine\Bin\*.Log' | Sort-Object LastWriteTime -Descending | Select-Object -First 1; Select-String -Path $latest.FullName -Pattern "error|Error|ERROR|crash|COM" | Select-Object -First 30
```

Expected: No D3D12 validation errors, no COM ptr crashes, scene reload succeeds.

---

## Task 8: Commit

- [ ] **Step 1: Stage and commit**

```bash
git add Source/Engine/Services/SceneService.h \
        Source/Engine/Services/SceneService.cpp \
        Source/Engine/Services/IGraphicsService.h \
        Source/Engine/Services/Common/IGraphicsService.cpp \
        Source/Engine/Services/TransformService.h \
        Source/Engine/Services/TransformService.cpp \
        Source/Engine/Services/PhysicsSimulationService.h \
        Source/Engine/Services/PhysicsSimulationService.cpp \
        Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.h \
        Source/Engine/ThirdParty/PhysXWrapper/PhysXWrapper.cpp \
        Source/Engine/Services/BillboardDrawCallService.h \
        Source/Engine/Services/BillboardDrawCallService.cpp \
        Source/DefaultClient/RenderingClient/GIResolvePass.cpp \
        Source/DefaultClient/RenderingClient/GIDataLoader.cpp \
        Source/DefaultClient/RenderingClient/VXGIRenderer.cpp \
        Source/DefaultClient/LogicClient/World.inl \
        Source/Editor/worldexplorer.cpp
git commit -m "refactor: replace SceneService priority callbacks with explicit lifecycle methods

Engine services (IGraphicsService, TransformService, PhysicsSimulationService,
BillboardDrawCallService) now implement OnSceneUnloading/OnSceneLoaded methods
called directly by SceneService::LoadSync in a hardcoded, deterministic order.
PhysXWrapper scene cleanup is consolidated into PhysicsSimulationService.
Client services (GIResolvePass, GIDataLoader, VXGIRenderer, WorldSystem, Editor)
use a simple no-sort registration list via AddSceneUnloadingCallback/AddSceneLoadedCallback.
Eliminates the priority-sort instability bug that caused COM ptr crashes on reload."
```
