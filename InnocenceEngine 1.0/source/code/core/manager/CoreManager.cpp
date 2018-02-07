#include "../../main/stdafx.h"
#include "CoreManager.h"

void CoreManager::setGameData(IGameData * gameData)
{
	m_gameData = gameData;
}

void CoreManager::setup()
{	// emplace_back in a static order.
	m_childEventManager.emplace_back(&LogManager::getInstance());
	m_childEventManager.emplace_back(&MemoryManager::getInstance());
	m_childEventManager.emplace_back(&TaskManager::getInstance());
	m_childEventManager.emplace_back(&TimeManager::getInstance());
	m_childEventManager.emplace_back(&SceneGraphManager::getInstance());
	m_childEventManager.emplace_back(&RenderingManager::getInstance());
	m_childEventManager.emplace_back(&AssetManager::getInstance());
	LogManager::getInstance().printLog("Start to setup all the managers.");
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->setup();
	}

	std::string l_gameName;
	try {
		m_gameData->getGameName(l_gameName);
	}
	catch (std::exception& e) {
		LogManager::getInstance().printLog("No game added!");
		LogManager::getInstance().printLog(e.what());
	}
	GLWindowManager::getInstance().setWindowName(l_gameName);

	m_gameData->setup();
}

void CoreManager::initialize()
{
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->initialize();
	}

	m_gameData->initialize();

	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	TimeManager::getInstance().update();

	// when time manager's status was execMessage::INITIALIZED, that means we can update other managers, a frame counter occurred.
	if (TimeManager::getInstance().getStatus() == objectStatus::ALIVE)
	{

		if (RenderingManager::getInstance().getStatus() == objectStatus::ALIVE)
		{
			auto l_tickTime = TimeManager::getInstance().getcurrentTime();
			// game data update
			m_gameData->update();
			if (m_gameData->needRender)
			{
				RenderingManager::getInstance().render();
			}
			RenderingManager::getInstance().update();
			l_tickTime = TimeManager::getInstance().getcurrentTime() - l_tickTime;
			//LogManager::getInstance().printLog(l_tickTime);
		}	
		else
		{
			this->setStatus(objectStatus::STANDBY);
			LogManager::getInstance().printLog("CoreManager is stand-by.");
		}

	}
}


void CoreManager::shutdown()
{
	m_gameData->shutdown();
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
	LogManager::getInstance().printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
