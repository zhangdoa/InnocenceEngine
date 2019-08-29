#include "SceneHierarchyManager.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ILightComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace InnoSceneHierarchyManagerNS
{
	SceneHierarchyMap m_SceneHierarchyMap;
	std::atomic<bool> m_needUpdate = true;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace InnoSceneHierarchyManagerNS;

bool InnoSceneHierarchyManager::Setup()
{
	f_SceneLoadingStartCallback = [&]() {
		m_SceneHierarchyMap.clear();
	};

	f_SceneLoadingFinishCallback = [&]() {
		m_needUpdate = true;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoSceneHierarchyManager::Initialize()
{
	return true;
}

bool InnoSceneHierarchyManager::Terminate()
{
	return true;
}

#define AddComponentToSceneHierarchyMap( className ) \
{ \
	auto l_Components = GetComponentManager(className)->GetAllComponents(); \
	for (auto i : l_Components) \
	{ \
		auto l_result = m_SceneHierarchyMap.find(i->m_parentEntity); \
		if (l_result != m_SceneHierarchyMap.end()) \
		{ \
			l_result->second.emplace(i); \
		} \
		else \
		{ \
			m_SceneHierarchyMap.emplace(i->m_parentEntity, std::set<InnoComponent*>{ i }); \
		} \
	}\
}

const SceneHierarchyMap & InnoSceneHierarchyManager::GetSceneHierarchyMap()
{
	if (m_needUpdate)
	{
		AddComponentToSceneHierarchyMap(TransformComponent);
		AddComponentToSceneHierarchyMap(VisibleComponent);
		AddComponentToSceneHierarchyMap(LightComponent);
		AddComponentToSceneHierarchyMap(CameraComponent);

		m_needUpdate = false;
	}

	return m_SceneHierarchyMap;
}