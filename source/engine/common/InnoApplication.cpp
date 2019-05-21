#include "InnoApplication.h"
#include "../system/CoreSystem.h"
#include "../../game/GameInstance.h"

namespace InnoApplication
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::unique_ptr<InnoCoreSystem> m_pCoreSystem;
	std::unique_ptr<GameInstance> m_pGameInstance;
}

bool InnoApplication::setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pCoreSystem = std::make_unique<InnoCoreSystem>();
	if (!m_pCoreSystem.get())
	{
		return false;
	}

	m_pGameInstance = std::make_unique<GameInstance>();
	if (!m_pGameInstance.get())
	{
		return false;
	}

	if (!m_pCoreSystem.get()->setup(appHook, extraHook, pScmdline))
	{
		return false;
	}

	if (!m_pGameInstance->setup())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoApplication::initialize()
{
	if (!m_pCoreSystem->initialize())
	{
		return false;
	}

	if (!m_pGameInstance->initialize())
	{
		return false;
	}

	m_objectStatus = ObjectStatus::Activated;
	return true;
}

bool InnoApplication::update()
{
	if (!m_pGameInstance->update())
	{
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
	if (!m_pCoreSystem->update())
	{
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
	return true;
}

bool InnoApplication::terminate()
{
	if (!m_pGameInstance->terminate())
	{
		return false;
	}
	if (!m_pCoreSystem->terminate())
	{
		return false;
	}
	m_objectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus InnoApplication::getStatus()
{
	return m_objectStatus;
}