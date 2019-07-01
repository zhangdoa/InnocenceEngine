#include "DirectionalLightComponentManager.h"
#include "../Component/DirectionalLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace DirectionalLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 16;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<DirectionalLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, DirectionalLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace DirectionalLightComponentManagerNS;

bool InnoDirectionalLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(DirectionalLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(DirectionalLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoDirectionalLightComponentManager::Initialize()
{
	return true;
}

bool InnoDirectionalLightComponentManager::Simulate()
{
	return true;
}

bool InnoDirectionalLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoDirectionalLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(DirectionalLightComponent);
}

void InnoDirectionalLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(DirectionalLightComponent);
}

InnoComponent* InnoDirectionalLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(DirectionalLightComponent, parentEntity);
}

const std::vector<DirectionalLightComponent*>& InnoDirectionalLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}