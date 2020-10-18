#include "SceneSystem.h"
#include "../Core/InnoLogger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

using SceneLoadingCallback = std::pair<std::function<void()>*, int32_t>;

namespace InnoSceneSystemNS
{
	bool saveScene(const char* fileName);
	bool loadScene(const char* fileName);
	bool loadSceneAsync(const char* fileName);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	std::vector<SceneLoadingCallback> m_sceneLoadingStartCallbacks;
	std::vector<SceneLoadingCallback> m_sceneLoadingFinishCallbacks;

	std::atomic<bool> m_isLoadingScene = false;
	std::atomic<bool> m_prepareForLoadingScene = false;

	std::string m_nextLoadingScene;
	std::string m_currentScene;

	SceneHierarchyMap m_SceneHierarchyMap;
	std::atomic<bool> m_needUpdate = true;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace InnoSceneSystemNS;

bool InnoSceneSystemNS::saveScene(const char* fileName)
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

bool InnoSceneSystemNS::loadSceneAsync(const char* fileName)
{
	if (!m_isLoadingScene)
	{
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool InnoSceneSystemNS::loadScene(const char* fileName)
{
	m_isLoadingScene = true;
	g_Engine->getTaskSystem()->waitAllTasksToFinish();

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

	g_Engine->getAssetSystem()->loadAssetsForComponents();

	m_isLoadingScene = false;

	InnoLogger::Log(LogLevel::Success, "SceneSystem: Scene ", fileName, " has been loaded.");

	return true;
}

bool InnoSceneSystem::Setup(ISystemConfig* systemConfig)
{
	f_SceneLoadingStartCallback = [&]() {
		m_SceneHierarchyMap.clear();
		g_Engine->getComponentManager()->CleanUp(ObjectLifespan::Scene);
	};

	f_SceneLoadingFinishCallback = [&]() {
		m_needUpdate = true;
	};

	addSceneLoadingStartCallback(&f_SceneLoadingStartCallback, -1);
	addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback, -1);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool InnoSceneSystem::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "SceneSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "SceneSystem: Object is not created!");
		return false;
	}
}

bool InnoSceneSystem::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		if (m_prepareForLoadingScene)
		{
			m_prepareForLoadingScene = false;

			InnoSceneSystemNS::loadScene(m_nextLoadingScene.c_str());
		}
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoSceneSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "SceneSystem has been terminated.");
	return true;
}

ObjectStatus InnoSceneSystem::GetStatus()
{
	return m_ObjectStatus;
}

std::string InnoSceneSystem::getCurrentSceneName()
{
	auto l_currentSceneName = m_currentScene.substr(0, m_currentScene.find(".InnoScene"));
	l_currentSceneName = l_currentSceneName.substr(l_currentSceneName.rfind("//") + 2);
	return l_currentSceneName;
}

bool InnoSceneSystem::loadScene(const char* fileName, bool AsyncLoad)
{
	if (m_currentScene == fileName)
	{
		InnoLogger::Log(LogLevel::Warning, "SceneSystem: Scene ", fileName, " has already loaded now.");
		return true;
	}

	if (m_nextLoadingScene == fileName)
	{
		InnoLogger::Log(LogLevel::Warning, "SceneSystem: Scene ", fileName, " has been scheduled for loading.");
		return true;
	}

	if (AsyncLoad)
	{
		return InnoSceneSystemNS::loadSceneAsync(fileName);
	}
	else
	{
		return InnoSceneSystemNS::loadScene(fileName);
	}
}

bool InnoSceneSystem::saveScene(const char* fileName)
{
	return InnoSceneSystemNS::saveScene(fileName);
}

bool InnoSceneSystem::isLoadingScene()
{
	return m_isLoadingScene;
}

bool InnoSceneSystem::addSceneLoadingStartCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingStartCallbacks.emplace_back(functor, priority);
	return true;
}

bool InnoSceneSystem::addSceneLoadingFinishCallback(std::function<void()>* functor, int32_t priority)
{
	m_sceneLoadingFinishCallbacks.emplace_back(functor, priority);
	return true;
}

template<typename T>
void AddComponentToSceneHierarchyMap()
{
	auto l_Components = g_Engine->getComponentManager()->GetAll<T>();
	for (auto i : l_Components)
	{
		auto l_result = m_SceneHierarchyMap.find(i->m_Owner);
		if (l_result != m_SceneHierarchyMap.end())
		{
			l_result->second.emplace(i);
		}
		else
		{
			m_SceneHierarchyMap.emplace(i->m_Owner, std::set<InnoComponent*>{ i });
		}
	}\
}

const SceneHierarchyMap& InnoSceneSystem::getSceneHierarchyMap()
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