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

	// 5. Client unloading callbacks
	for (auto* cb : m_sceneUnloadingCallbacks)
		(*cb)();

	// Load the new scene
	AssetService::LoadScene(fileName);

	Log(Verbose, "Scene ", fileName, " has been loaded.");

	// Loaded phase:
	// 6. Refresh engine service state that depends on loaded scene data
	g_Engine->Get<BillboardDrawCallService>()->OnSceneLoaded();

	// 7. Client loaded callbacks (GIDataLoader, VXGIRenderer, WorldSystem, Editor)
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
