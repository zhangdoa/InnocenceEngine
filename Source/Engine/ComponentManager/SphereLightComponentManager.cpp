#include "SphereLightComponentManager.h"
#include "../Component/SphereLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../Common/InnoMathHelper.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace SphereLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 128;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<SphereLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, SphereLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

	void UpdateColorTemperature(SphereLightComponent* rhs);
}

void SphereLightComponentManagerNS::UpdateColorTemperature(SphereLightComponent * rhs)
{
	if (rhs->m_UseColorTemperature)
	{
		rhs->m_RGBColor = InnoMath::colorTemperatureToRGB(rhs->m_ColorTemperature);
	}
}

using namespace SphereLightComponentManagerNS;

bool InnoSphereLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(SphereLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(SphereLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoSphereLightComponentManager::Initialize()
{
	return true;
}

bool InnoSphereLightComponentManager::Simulate()
{
	for (auto i : m_Components)
	{
		UpdateColorTemperature(i);
	}

	return true;
}

bool InnoSphereLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoSphereLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(SphereLightComponent);
}

void InnoSphereLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(SphereLightComponent);
}

InnoComponent* InnoSphereLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(SphereLightComponent, parentEntity);
}

const std::vector<SphereLightComponent*>& InnoSphereLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}