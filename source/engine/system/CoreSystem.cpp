#include "CoreSystem.h"

void CoreSystem::setup()
{
	g_pMemorySystem = new INNO_MEMORY_SYSTEM;
	g_pMemorySystem->setup();
	g_pLogSystem = g_pMemorySystem->spawn<INNO_LOG_SYSTEM>();
	g_pLogSystem->setup();
	g_pTaskSystem = g_pMemorySystem->spawn<INNO_TASK_SYSTEM>();
	g_pTaskSystem->setup();
	g_pTimeSystem = g_pMemorySystem->spawn<INNO_TIME_SYSTEM>();
	g_pTimeSystem->setup();
	g_pLogSystem->printLog("MemorySystem setup finished.");
	g_pLogSystem->printLog("LogSystem setup finished.");
	g_pLogSystem->printLog("TaskSystem setup finished.");
	g_pLogSystem->printLog("TimeSystem setup finished.");
	g_pGameSystem = g_pMemorySystem->spawn<INNO_GAME_SYSTEM>();
	g_pGameSystem->setup();
	g_pLogSystem->printLog("GameSystem setup finished.");
	g_pAssetSystem = g_pMemorySystem->spawn<INNO_ASSET_SYSTEM>();
	g_pAssetSystem->setup();
	g_pLogSystem->printLog("AssetSystem setup finished.");
	g_pRenderingSystem = g_pMemorySystem->spawn<INNO_RENDERING_SYSTEM>();
	g_pRenderingSystem->setup();
	g_pLogSystem->printLog("RenderingSystem setup finished.");

	if (g_pGameSystem->getStatus() == objectStatus::ALIVE)
	{
		g_pRenderingSystem->setWindowName(g_pGameSystem->getGameName());
		m_objectStatus = objectStatus::ALIVE;
		g_pLogSystem->printLog("CoreSystem setup finished.");
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("CoreSystem stand-by.");
	}
}

void CoreSystem::initialize()
{
	g_pMemorySystem->initialize();
	g_pLogSystem->initialize();
	g_pTaskSystem->initialize();
	g_pTimeSystem->initialize();
	g_pGameSystem->initialize();
	g_pAssetSystem->initialize();
	g_pRenderingSystem->initialize();
	g_pLogSystem->printLog("CoreSystem has been initialized.");
}

void CoreSystem::update()
{
	// time System should update without any limitation.
	g_pTimeSystem->update();

	// a frame counter occurred.
	// @TODO: Async rendering
	//if (g_pTimeSystem->getStatus() == objectStatus::ALIVE)
	//{

	// async game simulation
	std::async(&IGameSystem::update, g_pGameSystem);
	//// sync game simulation
	//g_pGameSystem->update();
	if (g_pRenderingSystem->getStatus() == objectStatus::ALIVE)
	{
		if (g_pGameSystem->needRender() && g_pRenderingSystem->canRender())
		{
			//std::async(&IRenderingSystem::render, g_pRenderingSystem);
			g_pRenderingSystem->update();
			g_pRenderingSystem->render();
		}
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("CoreSystem is stand-by.");
	}
#ifdef DEBUG
	//g_pMemorySystem->dumpToFile("../" + g_pTimeSystem->getCurrentTimeInLocal() + ".innoMemoryDump");
#endif // DEBUG
}

void CoreSystem::shutdown()
{
	g_pRenderingSystem->shutdown();
	g_pGameSystem->shutdown();
	g_pAssetSystem->shutdown();
	g_pTimeSystem->shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("CoreSystem has been shutdown.");
	//@TODO: dangerous
	g_pLogSystem->shutdown();
	g_pMemorySystem->shutdown();

	std::this_thread::sleep_for(std::chrono::seconds(2));
}

const objectStatus & CoreSystem::getStatus() const
{
	return m_objectStatus;
}

void CoreSystem::taskTest()
{
	g_pLogSystem->printLog("task");
}
