#include "SpotLightComponentManager.h"
#include "../Component/SpotLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace SpotLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 512;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<SpotLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, SpotLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace SpotLightComponentManagerNS;

bool InnoSpotLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(SpotLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(SpotLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoSpotLightComponentManager::Initialize()
{
	return true;
}

bool InnoSpotLightComponentManager::Simulate()
{
	return true;
}

bool InnoSpotLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoSpotLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(SpotLightComponent);
}

void InnoSpotLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(SpotLightComponent);
}

InnoComponent* InnoSpotLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(SpotLightComponent, parentEntity);
}

const std::vector<SpotLightComponent*>& InnoSpotLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}