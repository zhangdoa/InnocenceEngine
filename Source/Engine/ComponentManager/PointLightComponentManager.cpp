#include "PointLightComponentManager.h"
#include "../Component/PointLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace PointLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 1024;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<PointLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, PointLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace PointLightComponentManagerNS;

bool InnoPointLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(PointLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(PointLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoPointLightComponentManager::Initialize()
{
	return true;
}

bool InnoPointLightComponentManager::Simulate()
{
	return true;
}

bool InnoPointLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoPointLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(PointLightComponent);
}

void InnoPointLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(PointLightComponent);
}

InnoComponent* InnoPointLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(PointLightComponent, parentEntity);
}

const std::vector<PointLightComponent*>& InnoPointLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}