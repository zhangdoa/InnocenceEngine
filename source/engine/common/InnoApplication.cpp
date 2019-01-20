#include "InnoApplication.h"
#include "../system/CoreSystem.h"
#include "../../game/GameInstance.h"

namespace InnoApplication
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TimeSystem setup finished.");

		if (!g_pCoreSystem->getLogSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "LogSystem setup finished.");

		if (!g_pCoreSystem->getMemorySystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem setup finished.");

		if (!g_pCoreSystem->getTaskSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem setup finished.");

		if (!g_pCoreSystem->getFileSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem setup finished.");

		// @TODO: Time-domain coupling
		g_pCoreSystem->getGameSystem()->g_pMemorySystem = g_pCoreSystem->getMemorySystem();
		g_pCoreSystem->getGameSystem()->setGameInstance(g_pGameInstance);

		if (!g_pCoreSystem->getGameSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem setup finished.");

		if (!g_pCoreSystem->getAssetSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem setup finished.");

		if (!g_pCoreSystem->getPhysicsSystem()->setup())
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem setup finished.");

		if (!g_pCoreSystem->getVisionSystem()->setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
		{
			return false;
		}
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem setup finished.");

		m_objectStatus = ObjectStatus::ALIVE;

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine setup finished.");
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

	if (!g_pCoreSystem->getFileSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->initialize())
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

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been initialized.");

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

	if (!g_pCoreSystem->getFileSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->update())
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

	if (g_pCoreSystem->getVisionSystem()->getStatus() == ObjectStatus::ALIVE)
	{
		if (!g_pCoreSystem->getVisionSystem()->update())
		{
			return false;
		}
		g_pCoreSystem->getGameSystem()->saveComponentsCapture();
	}
	else
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
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

	if (!g_pCoreSystem->getFileSystem()->terminate())
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

	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been terminated.");
	return true;
}

ObjectStatus InnoApplication::getStatus()
{
	return m_objectStatus;
}