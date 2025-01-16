#include "SceneSystem.h"
#include "../Common/Logger.h"
#include "../Common/TaskScheduler.h"
#include "AssetSystem.h"
#include "ComponentManager.h"

#include "../Engine.h"
using namespace Inno;
;

#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

bool SceneSystem::loadSceneAsync(const char* fileName)
{
	if (!m_isLoadingScene)
	{
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool SceneSystem::loadSceneSync(const char* fileName)
{
	m_isLoadingScene = true;
	g_Engine->Get<TaskScheduler>()->WaitSync();

	m_currentScene = fileName;

	std::sort(m_sceneLoadingStartCallbacks.begin(), m_sceneLoadingStartCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second > B.second;
		});

	std::sort(m_sceneLoadingFinishCallbacks.begin(), m_sceneLoadingFinishCallbacks.end(),
		[&](SceneLoadingCallback A, SceneLoadingCallback B) {
			return A.second > B.second;
		});

	for (auto& i : m_sceneLoadingStartCallbacks)
	{
		(*i.first)();
	}

	JSONWrapper::loadScene(fileName);

	for (auto& i : m_sceneLoadingFinishCallbacks)
	{
		(*i.first)();
	}

	g_Engine->Get<AssetSystem>()->LoadAssetsForComponents(false);

	m_isLoadingScene = false;

	g_Engine->Get<Logger>()->Log(LogLevel::Success, "SceneSystem: Scene ", fileName, " has been loaded.");

	return true;
}

bool SceneSystem::Setup(ISystemConfig* systemConfig)
{
	f_SceneLoadingStartCallback = [&]() {
		m_SceneHierarchyMap.clear();
		g_Engine->Get<ComponentManager>()->CleanUp(ObjectLifespan::Scene);
	};

	f_SceneLoadingFinishCallback = [&]() {
		m_needUpdate = true;
	};

	addSceneLoadingStartCallback(&f_SceneLoadingStartCallback, -1);
	addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback, -1);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SceneSystem::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		g_Engine->Get<Logger>()->Log(LogLevel::Success, "SceneSystem has been initialized.");
		return true;
	}
	else
	{
		g_Engine->Get<Logger>()->Log(LogLevel::Error, "SceneSystem: Object is not created!");
		return false;
	}
}

bool SceneSystem::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		if (m_prepareForLoadingScene)
		{
			m_prepareForLoadingScene = false;

			loadSceneSync(m_nextLoadingScene.c_str());
		}
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool SceneSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "SceneSystem has been terminated.");
	return true;
}

ObjectStatus SceneSystem::GetStatus()
{
	return m_ObjectStatus;
}

std::string SceneSystem::getCurrentSceneName()
{
	auto l_currentSceneName = m_currentScene.substr(0, m_currentScene.find(".InnoScene"));
	l_currentSceneName = l_currentSceneName.substr(l_currentSceneName.rfind("//") + 2);
	return l_currentSceneName;
}

bool SceneSystem::loadScene(const char* fileName, bool AsyncLoad)
{
	if (m_currentScene == fileName)
	{
		g_Engine->Get<Logger>()->Log(LogLevel::Warning, "SceneSystem: Scene ", fileName, " has already loaded now.");
		return true;
	}

	if (m_nextLoadingScene == fileName)
	{
		g_Engine->Get<Logger>()->Log(LogLevel::Warning, "SceneSystem: Scene ", fileName, " has been scheduled for loading.");
		return true;
	}

	if (AsyncLoad)
	{
		return loadSceneAsync(fileName);
	}
	else
	{
		return loadScene(fileName);
	}
}

bool SceneSystem::saveScene(const char* fileName)
{
	if (!strcmp(fileName, ""))
	{
		return JSONWrapper::saveScene(m_currentScene.c_str());
	}
	else
	{
		return JSONWrapper::saveScene(fileName);
	}
}

bool SceneSystem::isLoadingScene()
{
	return m_isLoadingScene;
}

bool SceneSystem::addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingStartCallbacks.emplace_back(functor, priority);
	return true;
}

bool SceneSystem::addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingFinishCallbacks.emplace_back(functor, priority);
	return true;
}

template<typename T>
void SceneSystem::AddComponentToSceneHierarchyMap()
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

const SceneHierarchyMap& SceneSystem::getSceneHierarchyMap()
{
	if (m_needUpdate)
	{
		AddComponentToSceneHierarchyMap<TransformComponent>();
		AddComponentToSceneHierarchyMap<VisibleComponent>();
		AddComponentToSceneHierarchyMap<LightComponent>();
		AddComponentToSceneHierarchyMap<CameraComponent>();

		m_needUpdate = false;
	}

	return m_SceneHierarchyMap;
}