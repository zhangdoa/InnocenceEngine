#include "SceneService.h"
#include "../Common/LogService.h"
#include "AssetService.h"
#include "EntityRegistry.h"
#include "MeshResourceService.h"
#include "TextureResourceService.h"
#include "MaterialResourceService.h"
#include "GPUBufferResourceService.h"
#include "TransformService.h"
#include "PhysicsSimulationService.h"
#include "BillboardDrawCallService.h"
#include "FrameManagementService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

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
	// 0. Flush all GPU work before destroying resources that may still be in flight
	g_Engine->Get<FrameManagementService>()->WaitForGPUIdle();

	// 1. Client unloading callbacks fire FIRST so clients release their references
	// (RenderPassComponent*, GPUBufferComponent*, etc.) before the engine destroys
	// the underlying resources. Running them later (the previous order) meant client
	// callbacks dereferenced freed pointers — a latent crash whenever a client did
	// any non-trivial cleanup beyond clearing its own member to nullptr.
	for (auto* cb : m_sceneUnloadingCallbacks)
		(*cb)();

	// 2. Free GPU resources (component pointers still valid for the OnSceneUnloading
	// pass which iterates the resource pools)
	g_Engine->Get<MeshResourceService>()->OnSceneUnloading();
	g_Engine->Get<TextureResourceService>()->OnSceneUnloading();
	g_Engine->Get<MaterialResourceService>()->OnSceneUnloading();

	// 2b. Release scene-lifespan assets from the AssetService asset tables (TASK-52).
	// Without this, AllocateMeshAsset/Material/Texture continues to return the prior
	// scene's asset handle on name collision (residency still reads as Resident) while
	// its underlying GPU resources have just been freed — the new scene's MeshComponent
	// inherits a stale GPU VA and the first ExecuteIndirect / TLAS build that touches
	// it page-faults. Releasing here bumps the generation, clears the LUT, and forces
	// AllocateMeshAsset to hand the new scene a fresh slot.
	AssetService::ReleaseAssetsByLifespan(ObjectLifespan::Scene);

	// 3. Destroy scene-scoped components
	g_Engine->Get<EntityRegistry>()->CleanUp(ObjectLifespan::Scene);
	JSONWrapper::ClearLoadedCompFilenames();
	Log(Success, "Scene entities cleaned up.");

	// 4. Clear transform hierarchy (nodes index into now-empty storage, safe to reset)
	g_Engine->Get<TransformService>()->OnSceneUnloading();

	// 5. Clear physics simulation state and PhysX actors
	g_Engine->Get<PhysicsSimulationService>()->OnSceneUnloading();

	// Load the new scene
	AssetService::LoadScene(fileName);

	// 5b. Drain the deferred-initialization queues synchronously before
	// rendering resumes. AssetService::LoadScene queues mesh / texture /
	// material / GPU-buffer init tasks; FrameManagementService::Update
	// drains them one frame at a time. Without this drain, the first
	// post-load draw sees a mix of Activated and still-pending components
	// — Activated meshes draw, non-Activated meshes either skip or
	// reference garbage vertex/index buffers. The shader-ball-varies-
	// every-launch symptom (TASK-109) traces directly to this race.
	// GPU-idle wait afterwards to ensure upload command lists complete
	// before the next command list references the resources.
	g_Engine->Get<MeshResourceService>()->InitializeComponents();
	g_Engine->Get<TextureResourceService>()->InitializeComponents();
	g_Engine->Get<MaterialResourceService>()->InitializeComponents();
	g_Engine->Get<GPUBufferResourceService>()->InitializeComponents();
	g_Engine->Get<FrameManagementService>()->WaitForGPUIdle();

	// Loaded phase:
	// 6. Refresh engine service state that depends on loaded scene data
	g_Engine->Get<BillboardDrawCallService>()->OnSceneLoaded();

	// 7. Client loaded callbacks (GIDataLoader, VXGIRenderer, WorldSystem, Editor)
	for (auto* cb : m_sceneLoadedCallbacks)
		(*cb)();

	m_needUpdate = true;

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

	auto l_LastSlashPos = l_SceneName.rfind('/');
	if (l_LastSlashPos != std::string::npos)
		l_SceneName = l_SceneName.substr(l_LastSlashPos + 1);

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

void SceneService::ClearLoadingFlag()
{
	m_IsLoading = false;
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
