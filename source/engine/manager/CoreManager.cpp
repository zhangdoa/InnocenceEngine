#include "CoreManager.h"

void CoreManager::setup()
{
	// emplace_back in a static order.
	m_childEventManager.emplace_back(g_pMemoryManager);
	m_childEventManager.emplace_back(g_pLogManager);
	//m_childEventManager.emplace_back(g_pTaskManager);
	m_childEventManager.emplace_back(g_pTimeManager);
	m_childEventManager.emplace_back(g_pRenderingManager);
	m_childEventManager.emplace_back(g_pAssetManager);

	g_pMemoryManager = new INNO_MEMORY_MANAGER;
	g_pMemoryManager->setup();
	g_pLogManager = g_pMemoryManager->spawn<INNO_LOG_MANAGER>();
	//g_pTaskManager = g_pMemoryManager->spawn<INNO_TASK_MANAGER>();
	g_pTimeManager = g_pMemoryManager->spawn<INNO_TIME_MANAGER>();
	g_pRenderingManager = g_pMemoryManager->spawn<INNO_RENDERING_MANAGER>();
	g_pAssetManager = g_pMemoryManager->spawn<INNO_ASSET_MANAGER>();

	g_pLogManager->printLog("Start to setup all the managers.");

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->setup();
	}
	if (g_pGame != nullptr)
	{
		std::string l_gameName;
		g_pGame->getGameName(l_gameName);
		GLWindowManager::getInstance().setWindowName(l_gameName);
		g_pGame->setup();
		this->setStatus(objectStatus::ALIVE);
	}
	else
	{
		g_pLogManager->printLog("No game added!");
		this->setStatus(objectStatus::STANDBY);
	}
}

void CoreManager::initialize()
{
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->initialize();
	}

	g_pGame->initialize();

	g_pLogManager->printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	g_pTimeManager->update();

	// when time manager's status was objectStatus::ALIVE, that means we can update other managers, a frame counter occurred.
	if (g_pTimeManager->getStatus() == objectStatus::ALIVE)
	{

		if (g_pRenderingManager->getStatus() == objectStatus::ALIVE)
		{
			auto l_tickTime = g_pTimeManager->getcurrentTime();
			// game data update
			g_pGame->update();
			if (g_pGame->needRender)
			{
				g_pRenderingManager->render();
			}
			g_pRenderingManager->update();
			l_tickTime = g_pTimeManager->getcurrentTime() - l_tickTime;
			//LogManager::getInstance().printLog(l_tickTime);
		}
		else
		{
			this->setStatus(objectStatus::STANDBY);
			g_pLogManager->printLog("CoreManager is stand-by.");
		}

	}
}


void CoreManager::shutdown()
{
	//m_gameData->shutdown();
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->shutdown();
	}
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].release();
	}
	this->setStatus(objectStatus::SHUTDOWN);
	g_pLogManager->printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
