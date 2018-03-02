#include "CoreManager.h"

void CoreManager::setup()
{
	// emplace_back in a static order.
	m_childEventManager.emplace_back(m_pMemoryManager);
	m_childEventManager.emplace_back(m_pLogManager);
	m_childEventManager.emplace_back(m_pTaskManager);
	m_childEventManager.emplace_back(m_pTimeManager);
	m_childEventManager.emplace_back(m_pRenderingManager);
	m_childEventManager.emplace_back(m_pAssetManager);

	m_pMemoryManager = new MemoryManager();
	m_pMemoryManager->setup();
	m_pLogManager = m_pMemoryManager->spawn<LogManager>();
	m_pTaskManager = m_pMemoryManager->spawn<TaskManager>();
	m_pTimeManager = m_pMemoryManager->spawn<TimeManager>();
	m_pRenderingManager = m_pMemoryManager->spawn<RenderingManager>();

	m_pAssetManager = m_pMemoryManager->spawn<AssetManager>();

	m_pLogManager->printLog("Start to setup all the managers.");
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
		m_pLogManager->printLog("No game added!");
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

	m_pLogManager->printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	m_pTimeManager->update();

	// when time manager's status was objectStatus::ALIVE, that means we can update other managers, a frame counter occurred.
	if (m_pTimeManager->getStatus() == objectStatus::ALIVE)
	{

		if (m_pRenderingManager->getStatus() == objectStatus::ALIVE)
		{
			auto l_tickTime = m_pTimeManager->getcurrentTime();
			// game data update
			g_pGame->update();
			if (g_pGame->needRender)
			{
				m_pRenderingManager->render();
			}
			m_pRenderingManager->update();
			l_tickTime = m_pTimeManager->getcurrentTime() - l_tickTime;
			//LogManager::getInstance().printLog(l_tickTime);
		}
		else
		{
			this->setStatus(objectStatus::STANDBY);
			m_pLogManager->printLog("CoreManager is stand-by.");
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
	m_pLogManager->printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
