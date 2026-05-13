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

	g_Engine->Get<FrameManagementService>()->WaitForGPUIdle();

	// Client unloading callbacks must run before resource destruction so
	// clients release component pointers before the underlying GPU resources
	// go away.
	for (auto* cb : m_sceneUnloadingCallbacks)
		(*cb)();

	g_Engine->Get<MeshResourceService>()->OnSceneUnloading();
	g_Engine->Get<TextureResourceService>()->OnSceneUnloading();
	g_Engine->Get<MaterialResourceService>()->OnSceneUnloading();

	// Bump asset-table generation so AllocateMesh/Material/Texture cannot
	// hand the next scene a stale handle that name-collides with a just-freed
	// GPU resource.
	AssetService::ReleaseAssetsByLifespan(ObjectLifespan::Scene);

	g_Engine->Get<EntityRegistry>()->CleanUp(ObjectLifespan::Scene);
	JSONWrapper::ClearLoadedCompFilenames();
	Log(Success, "Scene entities cleaned up.");

	g_Engine->Get<TransformService>()->OnSceneUnloading();
	g_Engine->Get<PhysicsSimulationService>()->OnSceneUnloading();

	AssetService::LoadScene(fileName);

	// Drain deferred-init queues synchronously: the first post-load draw must
	// not see a mix of Activated and still-pending components, or meshes will
	// reference unuploaded vertex/index buffers. GPU-idle afterwards ensures
	// upload command lists complete before the next frame references them.
	g_Engine->Get<MeshResourceService>()->InitializeComponents();
	g_Engine->Get<TextureResourceService>()->InitializeComponents();
	g_Engine->Get<MaterialResourceService>()->InitializeComponents();
	g_Engine->Get<GPUBufferResourceService>()->InitializeComponents();
	g_Engine->Get<FrameManagementService>()->WaitForGPUIdle();

	g_Engine->Get<BillboardDrawCallService>()->OnSceneLoaded();

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
