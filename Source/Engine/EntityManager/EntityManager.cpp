#include "EntityManager.h"
#include "../Template/ObjectPool.h"
#include "../Core/InnoLogger.h"
#include "../Core/InnoRandomizer.h"

#include "../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace EntityManagerNS
{
	const size_t m_MaxEntity = 65536;
	TObjectPool<InnoEntity>* m_EntityPool;
	ThreadSafeVector<InnoEntity*> m_Entities;

	std::function<void()> f_SceneLoadingStartCallback;
}

using namespace EntityManagerNS;

bool InnoEntityManager::Setup()
{
	m_EntityPool = TObjectPool<InnoEntity>::Create(m_MaxEntity);

	f_SceneLoadingStartCallback = [&]() {
		for (auto i : m_Entities)
		{
			if (i->m_ObjectLifespan == ObjectLifespan::Scene)
			{
				i->m_ObjectStatus = ObjectStatus::Terminated;
				m_EntityPool->Destroy(i);
			}
		}

		m_Entities.erase(
			std::remove_if(m_Entities.begin(), m_Entities.end(),
				[&](auto val) {
					return val->m_ObjectLifespan == ObjectLifespan::Scene;
				}), m_Entities.end());
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);

	return true;
}

bool InnoEntityManager::Initialize()
{
	return true;
}

bool InnoEntityManager::Simulate()
{
	return true;
}

bool InnoEntityManager::Terminate()
{
	return true;
}

InnoEntity* InnoEntityManager::Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName)
{
	auto l_Entity = m_EntityPool->Spawn();

	if (l_Entity)
	{
		l_Entity->m_ObjectStatus = ObjectStatus::Created;
		m_Entities.emplace_back(l_Entity);
		auto l_UUID = InnoRandomizer::GenerateUUID();

		l_Entity->m_UUID = l_UUID;
		l_Entity->m_InstanceName = entityName;
		l_Entity->m_Serializable = serializable;
		l_Entity->m_ObjectLifespan = objectLifespan;
		l_Entity->m_ObjectStatus = ObjectStatus::Activated;

		InnoLogger::Log(LogLevel::Verbose, "EntityManager: Entity ", l_Entity->m_InstanceName.c_str(), " has been created.");
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "EntityManager: Can not creat Entity ", entityName, ".");
	}

	return l_Entity;
}

bool InnoEntityManager::Destroy(InnoEntity* entity)
{
	m_Entities.eraseByValue(entity);
	InnoLogger::Log(LogLevel::Verbose, "EntityManager: Entity ", entity->m_InstanceName.c_str(), " has been removed.");
	m_EntityPool->Destroy(entity);
	return true;
}

std::optional<InnoEntity*> InnoEntityManager::Find(const char* entityName)
{
	auto l_FindResult = std::find_if(
		m_Entities.begin(),
		m_Entities.end(),
		[&](auto val) -> bool {
			return val->m_InstanceName == entityName;
		});

	if (l_FindResult != m_Entities.end())
	{
		return *l_FindResult;
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "EntityManager: Can't find entity by name ", entityName, "!");
		return std::nullopt;
	}
}

const std::vector<InnoEntity*>& InnoEntityManager::GetEntities()
{
	return m_Entities.getRawData();
}