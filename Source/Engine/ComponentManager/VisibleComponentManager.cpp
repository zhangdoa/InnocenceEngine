#include "VisibleComponentManager.h"
#include "../Component/VisibleComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace VisibleComponentManagerNS
{
	const size_t m_MaxComponentCount = 32768;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<VisibleComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, VisibleComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
	std::function<void(VisibleComponent*, bool)> f_LoadAssetTask;
	std::function<void(VisibleComponent*)> f_AssignUnitMeshTask;

	void assignUnitMesh(MeshShapeType meshShapeType, VisibleComponent* visibleComponent)
	{
		auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(meshShapeType);
		auto l_material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
		l_material->m_objectStatus = ObjectStatus::Created;
		visibleComponent->m_modelMap.emplace(l_mesh, l_material);
	}
}

using namespace VisibleComponentManagerNS;

bool InnoVisibleComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(VisibleComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(VisibleComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	f_LoadAssetTask = [=](VisibleComponent* i, bool AsyncLoad)
	{
		i->m_modelMap = g_pModuleManager->getFileSystem()->loadModel(i->m_modelFileName, AsyncLoad);

		auto l_transformComponent = GetComponent(TransformComponent, i->m_parentEntity);
		auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

		for (auto j : i->m_modelMap)
		{
			g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(j.first);
			g_pModuleManager->getPhysicsSystem()->generateAABBInWorldSpace(j.first->m_PDC, l_globalTm);
		}

		g_pModuleManager->getPhysicsSystem()->generatePhysicsProxy(i);
		i->m_objectStatus = ObjectStatus::Activated;
	};

	f_AssignUnitMeshTask = [=](VisibleComponent* i)
	{
		assignUnitMesh(i->m_meshShapeType, i);

		auto l_transformComponent = GetComponent(TransformComponent, i->m_parentEntity);
		auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

		for (auto j : i->m_modelMap)
		{
			g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(j.first);
			g_pModuleManager->getPhysicsSystem()->generateAABBInWorldSpace(j.first->m_PDC, l_globalTm);
		}

		g_pModuleManager->getPhysicsSystem()->generatePhysicsProxy(i);

		i->m_objectStatus = ObjectStatus::Activated;
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

InnoComponent * InnoVisibleComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(VisibleComponent);
}

void InnoVisibleComponentManager::Destroy(InnoComponent * component)
{
	DestroyComponentImpl(VisibleComponent);
}

InnoComponent* InnoVisibleComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(VisibleComponent, parentEntity);
}

const std::vector<VisibleComponent*>& InnoVisibleComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}

void InnoVisibleComponentManager::LoadAssetsForComponents(bool AsyncLoad)
{
	for (auto i : m_Components)
	{
		if (i->m_visiblilityType != VisiblilityType::Invisible)
		{
			if (!i->m_modelFileName.empty())
			{
				if (AsyncLoad)
				{
					g_pModuleManager->getTaskSystem()->submit("LoadAssetTask", 4, nullptr, f_LoadAssetTask, i, true);
				}
				else
				{
					f_LoadAssetTask(i, false);
				}
			}
			else
			{
				if (i->m_meshShapeType != MeshShapeType::Custom)
				{
					if (AsyncLoad)
					{
						g_pModuleManager->getTaskSystem()->submit("AssignUnitMeshTask", 4, nullptr, f_AssignUnitMeshTask, i);
					}
					else
					{
						f_AssignUnitMeshTask(i);
					}
				}
				else
				{
					InnoLogger::Log(LogLevel::Warning, "VisibleComponentManager: Custom shape mesh specified without a model preset file.");
				}
			}
		}
	}
}