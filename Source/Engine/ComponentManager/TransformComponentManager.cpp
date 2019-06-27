#include "TransformComponentManager.h"
#include "../Component/TransformComponent.h"
#include "../Core/InnoMemory.h"
#include "CommonMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace TransformComponentManagerNS
{
	const size_t m_MaxComponent = 32768;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<TransformComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, TransformComponent*> m_ComponentsMap;
	TransformComponent* m_RootTransformComponent;

	std::function<void()> f_SceneLoadingStartCallback;

	void SortTransformComponentsVector()
	{
		//construct the hierarchy tree
		for (auto i : m_Components)
		{
			if (i->m_parentTransformComponent)
			{
				i->m_transformHierarchyLevel = i->m_parentTransformComponent->m_transformHierarchyLevel + 1;
			}
		}
		//from top to bottom
		std::sort(m_Components.begin(), m_Components.end(), [&](TransformComponent* a, TransformComponent* b)
		{
			return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
		});
	}

	void SimulateTransformComponents()
	{
		std::for_each(m_Components.begin(), m_Components.end(), [&](TransformComponent* val)
		{
			if (val->m_parentTransformComponent)
			{
				val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
				val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
				val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
			}
		});
	}
}

using namespace TransformComponentManagerNS;

bool TransformComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(TransformComponent), m_MaxComponent);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(TransformComponent);
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);

	return true;
}

bool TransformComponentManager::Initialize()
{
	return true;
}

bool TransformComponentManager::Simulate()
{
	return true;
}

bool TransformComponentManager::Terminate()
{
	return true;
}

InnoComponent * TransformComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponent(TransformComponent);
}

void TransformComponentManager::Destory(InnoComponent * component)
{
	DestroyComponent(TransformComponent);
}