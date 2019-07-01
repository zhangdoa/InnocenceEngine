#include "CameraComponentManager.h"
#include "../Component/CameraComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace CameraComponentManagerNS
{
	const size_t m_MaxComponentCount = 32;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<CameraComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, CameraComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;
}

using namespace CameraComponentManagerNS;

bool InnoCameraComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(CameraComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(CameraComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoCameraComponentManager::Initialize()
{
	return true;
}

bool InnoCameraComponentManager::Simulate()
{
	return true;
}

bool InnoCameraComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoCameraComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(CameraComponent);
}

void InnoCameraComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(CameraComponent);
}

InnoComponent* InnoCameraComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(CameraComponent, parentEntity);
}

const std::vector<CameraComponent*>& InnoCameraComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}