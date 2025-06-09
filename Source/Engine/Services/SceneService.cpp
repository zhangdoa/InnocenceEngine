#include "SceneService.h"
#include "../Common/LogService.h"
#include "../Common/TaskScheduler.h"
#include "AssetService.h"
#include "ComponentManager.h"

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

bool SceneService::Setup(ISystemConfig* systemConfig)
{
	f_SceneLoadingStartedCallback = [&]() 
	{
		Log(Verbose, "Resetting scene hierarchy map...");

		m_SceneHierarchyMap.clear();
		g_Engine->Get<ComponentManager>()->CleanUp(ObjectLifespan::Scene);

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
	auto l_currentSceneName = m_currentScene.substr(0, m_currentScene.find(".InnoScene"));
	l_currentSceneName = l_currentSceneName.substr(l_currentSceneName.rfind("//") + 2);
	return l_currentSceneName;
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

template<typename T>
void SceneService::AddComponentToSceneHierarchyMap()
{
	auto l_Components = g_Engine->Get<ComponentManager>()->GetAll<T>();
	for (auto i : l_Components)
	{
		auto l_componentPair = std::make_pair(i->GetTypeID(), i);
		auto l_result = m_SceneHierarchyMap.find(i->m_Owner);
		if (l_result != m_SceneHierarchyMap.end())
		{
			l_result->second.emplace(l_componentPair);
		}
		else
		{
			m_SceneHierarchyMap.emplace(i->m_Owner, std::set<ComponentPair>{ l_componentPair });
		}
	}
}

const SceneHierarchyMap& SceneService::getSceneHierarchyMap()
{
	if (m_needUpdate)
	{
		AddComponentToSceneHierarchyMap<TransformComponent>();
		AddComponentToSceneHierarchyMap<ModelComponent>();
		AddComponentToSceneHierarchyMap<LightComponent>();
		AddComponentToSceneHierarchyMap<CameraComponent>();

		m_needUpdate = false;
	}

	return m_SceneHierarchyMap;
}