#include "VisibleComponentManager.h"
#include "../Component/VisibleComponent.h"
#include "../Template/ObjectPool.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"

#include "../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace VisibleComponentManagerNS
{
	const size_t m_MaxComponentCount = 32768;
	size_t m_CurrentComponentIndex = 0;
	TObjectPool<VisibleComponent>* m_ComponentPool;
	ThreadSafeVector<VisibleComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, VisibleComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace VisibleComponentManagerNS;

bool InnoVisibleComponentManager::Setup()
{
	m_ComponentPool = TObjectPool<VisibleComponent>::Create(m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);
	m_ComponentsMap.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(VisibleComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getSceneSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getSceneSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoVisibleComponentManager::Initialize()
{
	return true;
}

bool InnoVisibleComponentManager::Simulate()
{
	return true;
}

bool InnoVisibleComponentManager::PostFrame()
{
	return true;
}

bool InnoVisibleComponentManager::Terminate()
{
	return true;
}

InnoComponent* InnoVisibleComponentManager::Spawn(const InnoEntity* parentEntity, bool serializable, ObjectLifespan objectLifespan)
{
	SpawnComponentImpl(VisibleComponent);
}

void InnoVisibleComponentManager::Destroy(InnoComponent* component)
{
	DestroyComponentImpl(VisibleComponent);
}

InnoComponent* InnoVisibleComponentManager::Find(const InnoEntity* parentEntity)
{
	GetComponentImpl(VisibleComponent, parentEntity);
}

VisibleComponent* InnoVisibleComponentManager::Get(std::size_t index)
{
	if (index >= m_Components.size())
	{
		return nullptr;
	}
	return m_Components[index];
}

const std::vector<VisibleComponent*>& InnoVisibleComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}