#include "SceneService.h"
#include "../Common/LogService.h"
#include "../Common/TaskScheduler.h"
#include "AssetService.h"
#include "EntityRegistry.h"

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

	std::sort(m_sceneLoadingStartCallbacks.begin(), m_sceneLoadingStartCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second < B.second;
		});

	std::sort(m_sceneLoadingFinishCallbacks.begin(), m_sceneLoadingFinishCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second < B.second;
		});

	Log(Verbose, "Loading scene ", fileName, "...");

	for (auto& i : m_sceneLoadingStartCallbacks)
	{
		Log(Verbose, "Scene loading start callback (priority: ", i.second, ") is called.");
		(*i.first)();
	}

	AssetService::LoadScene(fileName);

	Log(Verbose, "Scene ", fileName, " has been loaded.");

	for (auto& i : m_sceneLoadingFinishCallbacks)
	{
		Log(Verbose, "Scene loading finish callback (priority: ", i.second, ") is called.");
		(*i.first)();
	}

	m_IsLoading = false;

	Log(Success, "Scene ", fileName, " has been loaded.");
	
	return true;
}

bool SceneService::Setup(IServiceConfig* systemConfig)
{
	f_SceneLoadingStartedCallback = [&]()
	{
		Log(Verbose, "Resetting scene hierarchy map...");

		m_SceneHierarchyMap.clear();
		g_Engine->Get<EntityRegistry>()->CleanUp(ObjectLifespan::Scene);

		Log(Success, "Scene hierarchy map has been reset.");
	};

	f_SceneLoadingFinishCallback = [&]() 
	{
		m_needUpdate = true;
	};

	AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 0);
	AddSceneLoadingFinishedCallback(&f_SceneLoadingFinishCallback, 0);

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

bool SceneService::AddSceneLoadingStartedCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingStartCallbacks.emplace_back(functor, priority);
	return true;
}

bool SceneService::AddSceneLoadingFinishedCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingFinishCallbacks.emplace_back(functor, priority);
	return true;
}

const SceneHierarchyMap& SceneService::getSceneHierarchyMap()
{
	if (m_needUpdate)
	{
		m_needUpdate = false;
	}

	return m_SceneHierarchyMap;
}