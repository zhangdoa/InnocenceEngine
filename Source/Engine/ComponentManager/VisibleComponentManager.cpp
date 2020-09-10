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
	std::function<void(VisibleComponent*, bool)> f_LoadModelTask;
	std::function<void(VisibleComponent*, bool)> f_AssignProceduralModelTask;
	std::function<void(VisibleComponent*)> f_PDCTask;
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

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	f_LoadModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = g_pModuleManager->getAssetSystem()->loadModel(i->m_modelFileName.c_str(), AsyncLoad);
	};

	f_AssignProceduralModelTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_model = g_pModuleManager->getAssetSystem()->addProceduralModel(i->m_proceduralMeshShape);
		auto l_pair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(i->m_model->meshMaterialPairs.m_startOffset);
		g_pModuleManager->getRenderingFrontend()->registerMaterialDataComponent(l_pair->material, AsyncLoad);
	};

	// @TODO: Concurrency
	f_PDCTask = [=](VisibleComponent* i)
	{
		g_pModuleManager->getPhysicsSystem()->generatePhysicsProxy(i);

		i->m_ObjectStatus = ObjectStatus::Activated;
	};

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

const std::vector<VisibleComponent*>& InnoVisibleComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}

void InnoVisibleComponentManager::LoadAssetsForComponents(bool AsyncLoad)
{
	// @TODO: Load unit model first

	for (auto i : m_Components)
	{
		if (i->m_meshSource != MeshSource::Customized)
		{
			if (AsyncLoad)
			{
				auto l_loadModelTask = g_pModuleManager->getTaskSystem()->submit("AssignProceduralModelTask", 4, nullptr, f_AssignProceduralModelTask, i, true);
				g_pModuleManager->getTaskSystem()->submit("PDCTask", 4, l_loadModelTask, f_PDCTask, i);
			}
			else
			{
				f_AssignProceduralModelTask(i, false);
				f_PDCTask(i);
			}
		}
		else
		{
			if (!i->m_modelFileName.empty())
			{
				if (AsyncLoad)
				{
					auto l_loadModelTask = g_pModuleManager->getTaskSystem()->submit("LoadModelTask", 4, nullptr, f_LoadModelTask, i, true);
					g_pModuleManager->getTaskSystem()->submit("PDCTask", 4, l_loadModelTask, f_PDCTask, i);
				}
				else
				{
					f_LoadModelTask(i, false);
					f_PDCTask(i);
				}
			}
			else
			{
				InnoLogger::Log(LogLevel::Warning, "VisibleComponentManager: Custom shape mesh specified without a model preset file.");
			}
		}
	}
}