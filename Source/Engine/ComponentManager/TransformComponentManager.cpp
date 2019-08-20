#include "TransformComponentManager.h"
#include "../Component/TransformComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace TransformComponentManagerNS
{
	const size_t m_MaxComponentCount = 32768;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<TransformComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, TransformComponent*> m_ComponentsMap;
	InnoEntity* m_RootTransformEntity;
	TransformComponent* m_RootTransformComponent;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

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
		auto l_tickTime = g_pModuleManager->getTickTime();
		auto l_ratio = (1.0f - l_tickTime / 100.0f);
		l_ratio = InnoMath::clamp(l_ratio, 0.01f, 0.99f);

		std::for_each(m_Components.begin(), m_Components.end(), [&](TransformComponent* val)
		{
			if (!InnoMath::isCloseEnough(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos))
			{
				val->m_localTransformVector.m_pos = InnoMath::lerp(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos, l_ratio);
			}

			if (!InnoMath::isCloseEnough(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot))
			{
				val->m_localTransformVector.m_rot = InnoMath::slerp(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot, l_ratio);
			}

			if (!InnoMath::isCloseEnough(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale))
			{
				val->m_localTransformVector.m_scale = InnoMath::lerp(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale, l_ratio);
			}

			if (val->m_parentTransformComponent)
			{
				val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
				val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
			}
		});
	}
}

using namespace TransformComponentManagerNS;

bool InnoTransformComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(TransformComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(TransformComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
		SortTransformComponentsVector();

		for (auto i : m_Components)
		{
			i->m_localTransformVector_target = i->m_localTransformVector;

			if (i->m_parentTransformComponent)
			{
				i->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(i->m_localTransformVector, i->m_parentTransformComponent->m_globalTransformVector, i->m_parentTransformComponent->m_globalTransformMatrix);
				i->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(i->m_globalTransformVector);
			}
		}
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	m_RootTransformEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, "RootTransform/");

	m_RootTransformComponent = SpawnComponent(TransformComponent, m_RootTransformEntity, ObjectSource::Runtime, ObjectUsage::Engine);

	m_RootTransformComponent->m_localTransformVector_target = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformVector = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(m_RootTransformComponent->m_globalTransformVector);

	return true;
}

bool InnoTransformComponentManager::Initialize()
{
	return true;
}

bool InnoTransformComponentManager::Simulate()
{
	auto l_SimulateTask = g_pModuleManager->getTaskSystem()->submit("TransformComponentsSimulateTask", 0, nullptr, [&]()
	{
		SimulateTransformComponents();
	});

	return true;
}

bool InnoTransformComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoTransformComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(TransformComponent);
}

void InnoTransformComponentManager::Destroy(InnoComponent * component)
{
	DestroyComponentImpl(TransformComponent);
}

InnoComponent* InnoTransformComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(TransformComponent, parentEntity);
}

void InnoTransformComponentManager::SaveCurrentFrameTransform()
{
	std::for_each(m_Components.begin(), m_Components.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

const TransformComponent * InnoTransformComponentManager::GetRootTransformComponent() const
{
	return m_RootTransformComponent;
}

const std::vector<TransformComponent*>& InnoTransformComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}