#include "EntityManager.h"
#include "../Common/ObjectPool.h"
#include "../Common/LogService.h"
#include "../Common/Randomizer.h"
#include "../Common/ThreadSafeVector.h"
#include "SceneSystem.h"

#include "../Engine.h"

using namespace Inno;
;

namespace EntityManagerNS
{
	const size_t m_MaxEntity = 65536;
	TObjectPool<Entity>* m_EntityPool;
	ThreadSafeVector<Entity*> m_Entities;

	std::function<void()> f_SceneLoadingStartedCallback;
}

using namespace EntityManagerNS;

bool EntityManager::Setup(ISystemConfig* systemConfig)
{
	m_EntityPool = TObjectPool<Entity>::Create(m_MaxEntity);

	f_SceneLoadingStartedCallback = [&]() 
	{
		Log(Verbose, "Clearing all entities...");

		m_Entities.erase(
			std::remove_if(m_Entities.begin(), m_Entities.end(),
				[&](auto val) {
					if(val->m_ObjectLifespan == ObjectLifespan::Scene)
					{
						val->m_ObjectStatus = ObjectStatus::Terminated;
						m_EntityPool->Destroy(val);
						return true;
					}
					return false;
				}), m_Entities.end());

		Log(Success, "All entities have been cleared.");
	};

	g_Engine->Get<SceneSystem>()->AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 0);

	return true;
}

bool EntityManager::Initialize()
{
	return true;
}

bool EntityManager::Update()
{
	return true;
}

bool EntityManager::Terminate()
{
	return true;
}

ObjectStatus EntityManager::GetStatus()
{
	return ObjectStatus();
}

Entity* EntityManager::Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName)
{
	auto l_Entity = m_EntityPool->Spawn();

	if (l_Entity)
	{
		l_Entity->m_ObjectStatus = ObjectStatus::Created;
		m_Entities.emplace_back(l_Entity);
		auto l_UUID = Randomizer::GenerateUUID();

		l_Entity->m_UUID = l_UUID;
		l_Entity->m_InstanceName = entityName;
		l_Entity->m_Serializable = serializable;
		l_Entity->m_ObjectLifespan = objectLifespan;
		l_Entity->m_ObjectStatus = ObjectStatus::Activated;

		Log(Verbose, "Entity ", l_Entity->m_InstanceName.c_str(), " has been created.");
	}
	else
	{
		Log(Warning, "Can not creat Entity ", entityName, ".");
	}

	return l_Entity;
}

bool EntityManager::Destroy(Entity* entity)
{
	m_Entities.eraseByValue(entity);
	Log(Verbose, "Entity ", entity->m_InstanceName.c_str(), " has been removed.");
	m_EntityPool->Destroy(entity);
	return true;
}

std::optional<Entity*> EntityManager::Find(const char* entityName)
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
		Log(Warning, "Can't find entity by name ", entityName, "!");
		return std::nullopt;
	}
}

const std::vector<Entity*>& EntityManager::GetEntities()
{
	return m_Entities.getRawData();
}