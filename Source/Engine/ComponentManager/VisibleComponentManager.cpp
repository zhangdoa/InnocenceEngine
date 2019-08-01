#include "VisibleComponentManager.h"
#include "../Component/VisibleComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

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

	void assignUnitMesh(MeshShapeType meshUsageType, VisibleComponent* visibleComponent)
	{
		if (meshUsageType != MeshShapeType::Custom)
		{
			auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(meshUsageType);
			auto l_material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
			l_material->m_objectStatus = ObjectStatus::Created;
			visibleComponent->m_modelMap.emplace(l_mesh, l_material);
		}
		else
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "AssetSystem: don't assign unit mesh to a custom mesh shape component!");
		}
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

void InnoVisibleComponentManager::Destory(InnoComponent * component)
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

void InnoVisibleComponentManager::LoadAssetsForComponents()
{
	for (auto i : m_Components)
	{
		if (i->m_visiblilityType != VisiblilityType::Invisible)
		{
			if (i->m_meshShapeType == MeshShapeType::Custom)
			{
				if (!i->m_modelFileName.empty())
				{
					g_pModuleManager->getTaskSystem()->submit("LoadAssetTask", [=]()
					{
						i->m_modelMap = g_pModuleManager->getFileSystem()->loadModel(i->m_modelFileName);
						g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(i);
						i->m_objectStatus = ObjectStatus::Activated;
					});
				}
			}
			else
			{
				g_pModuleManager->getTaskSystem()->submit("LoadAssetTask", [=]()
				{
					assignUnitMesh(i->m_meshShapeType, i);
					g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(i);
					i->m_objectStatus = ObjectStatus::Activated;
				});
			}
		}
	}
}