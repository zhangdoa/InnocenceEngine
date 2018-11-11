#include "InnoApplication.h"
#include "../system/CoreSystem.h"
#include "../../game/GameInstance.h"

namespace InnoApplication
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	ICoreSystem* g_pCoreSystem;
	std::unique_ptr<InnoCoreSystem> m_pCoreSystem;
	IGameInstance* g_pGameInstance;
	std::unique_ptr<GameInstance> m_pGameInstance;
}

bool InnoApplication::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{	
	m_pCoreSystem = std::make_unique<InnoCoreSystem>();
	g_pCoreSystem = m_pCoreSystem.get();

	m_pGameInstance = std::make_unique<GameInstance>();
	g_pGameInstance = m_pGameInstance.get();

	if (g_pCoreSystem)
	{
		if (!g_pCoreSystem->setup())
		{
			return false;
		}

		if (!g_pCoreSystem->getTimeSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("TimeSystem setup finished.");

		if (!g_pCoreSystem->getLogSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("LogSystem setup finished.");

		if (!g_pCoreSystem->getMemorySystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("MemorySystem setup finished.");

		if (!g_pCoreSystem->getTaskSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("TaskSystem setup finished.");

		// @TODO: Time-domain coupling
		g_pCoreSystem->getGameSystem()->g_pMemorySystem = g_pCoreSystem->getMemorySystem();

		if (!g_pCoreSystem->getGameSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("GameSystem setup finished.");

		if (!g_pGameInstance->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("GameInstance setup finished.");

		if (!g_pCoreSystem->getAssetSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("AssetSystem setup finished.");

		if (!g_pCoreSystem->getPhysicsSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("PhysicsSystem setup finished.");

		if (!g_pCoreSystem->getVisionSystem()->setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog("VisionSystem setup finished.");

		m_objectStatus = objectStatus::ALIVE;

		g_pCoreSystem->getLogSystem()->printLog("Engine setup finished.");
		return true;
	}
	else
	{
		return false;
	}
}

bool InnoApplication::initialize()
{
	if (!g_pCoreSystem->getTimeSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->initialize())
	{
		return false;
	}

	if (!g_pGameInstance->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getVisionSystem()->initialize())
	{
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog("Engine has been initialized.");

	return true;
}

bool InnoApplication::update()
{
	if (!g_pCoreSystem->getTimeSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->update())
	{
		return false;
	}

	if (!g_pGameInstance->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->update())
	{
		return false;
	}

	if (g_pCoreSystem->getVisionSystem()->getStatus() == objectStatus::ALIVE)
	{
		if (!g_pCoreSystem->getVisionSystem()->update())
		{
			return false;
		}
		g_pCoreSystem->getGameSystem()->saveComponentsCapture();

		return true;
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog("Engine is stand-by.");
		return false;
	}
	return true;
}

bool InnoApplication::terminate()
{
	if (!g_pCoreSystem->getVisionSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->terminate())
	{
		return false;
	}

	if (!g_pGameInstance->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getTimeSystem()->terminate())
	{
		return false;
	}

	m_objectStatus = objectStatus::SHUTDOWN;
	return true;
}

objectStatus InnoApplication::getStatus()
{
	return m_objectStatus;
}

