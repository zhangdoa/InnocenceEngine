#include "CoreManager.h"

void CoreManager::setup()
{
	g_pMemoryManager = new INNO_MEMORY_MANAGER;
	g_pMemoryManager->setup();
	g_pLogManager = g_pMemoryManager->spawn<INNO_LOG_MANAGER>();
	g_pLogManager->setup();
	//g_pTaskManager = g_pMemoryManager->spawn<INNO_TASK_MANAGER>();
	g_pTimeManager = g_pMemoryManager->spawn<INNO_TIME_MANAGER>();
	g_pTimeManager->setup();
	g_pLogManager->printLog("MemoryManager setup finished.");
	g_pLogManager->printLog("LogManager setup finished.");
	g_pLogManager->printLog("TimeManager setup finished.");
	g_pRenderingManager = g_pMemoryManager->spawn<INNO_RENDERING_MANAGER>();
	g_pRenderingManager->setup();
	g_pLogManager->printLog("RenderingManager setup finished.");
	g_pAssetManager = g_pMemoryManager->spawn<INNO_ASSET_MANAGER>();
	g_pAssetManager->setup();
	g_pLogManager->printLog("AssetManager setup finished.");
	g_pGameManager = g_pMemoryManager->spawn<INNO_GAME_MANAGER>();
	g_pGameManager->setup();
	g_pLogManager->printLog("GameManager setup finished.");
	if (g_pGameManager->getStatus() == objectStatus::ALIVE)
	{
		GLWindowManager::getInstance().setWindowName(g_pGameManager->getGameName());
		m_objectStatus = objectStatus::ALIVE;
		g_pLogManager->printLog("CoreManager setup finished.");
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogManager->printLog("CoreManager stand-by.");
	}
}

void CoreManager::initialize()
{
	g_pMemoryManager->initialize();
	g_pLogManager->initialize();
	g_pTimeManager->initialize();
	g_pRenderingManager->initialize();
	g_pAssetManager->initialize();
	g_pGameManager->initialize();

	g_pLogManager->printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	g_pTimeManager->update();

	// a frame counter occurred.
	// @TODO: Async rendering
	if (g_pTimeManager->getStatus() == objectStatus::ALIVE)
	{
		if (g_pRenderingManager->getStatus() == objectStatus::ALIVE)
		{
			auto l_tickTime = g_pTimeManager->getcurrentTime();
			// game simulation
			g_pGameManager->update();

			if (g_pGameManager->needRender())
			{
				g_pRenderingManager->render();
			}

			g_pRenderingManager->update();
			l_tickTime = g_pTimeManager->getcurrentTime() - l_tickTime;
#ifdef DEBUG
			//g_pMemoryManager->dumpToFile("../" + g_pTimeManager->getCurrentTimeInLocal() + ".innoMemoryDump");
#endif // DEBUG
		}
		else
		{
			m_objectStatus = objectStatus::STANDBY;
			g_pLogManager->printLog("CoreManager is stand-by.");
		}

	}
}


void CoreManager::shutdown()
{
	g_pGameManager->shutdown();
	g_pAssetManager->shutdown();
	g_pRenderingManager->shutdown();
	g_pTimeManager->shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogManager->printLog("CoreManager has been shutdown.");
	//@TODO: dangerous
	g_pLogManager->shutdown();
	g_pMemoryManager->shutdown();

	std::this_thread::sleep_for(std::chrono::seconds(2));
}

const objectStatus & CoreManager::getStatus() const
{
	return m_objectStatus;
}