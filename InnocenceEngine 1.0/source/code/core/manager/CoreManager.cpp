#include "../../main/stdafx.h"
#include "CoreManager.h"


CoreManager::CoreManager()
{
}


CoreManager::~CoreManager()
{
}

void CoreManager::setGameData(IGameData * gameData)
{
	m_gameData = gameData;
}


void CoreManager::initialize()
{
	// emplace_back in a static order.
	m_childEventManager.emplace_back(&AssetManager::getInstance());
	m_childEventManager.emplace_back(&TimeManager::getInstance());
	m_childEventManager.emplace_back(&SceneGraphManager::getInstance());
	m_childEventManager.emplace_back(&RenderingManager::getInstance());

	std::string l_gameName;
	m_gameData->getGameName(l_gameName);

	GLWindowManager::getInstance().setWindowName(l_gameName);

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->initialize();
	}

	try {
		m_gameData->initialize();
	}
	catch (std::exception& e) {
		LogManager::getInstance().printLog("No game added!");
		LogManager::getInstance().printLog(e.what());
	}

	// @TODO
	RenderingManager::getInstance().initInput();

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
			RenderingManager::getInstance().update();
			l_tickTime = TimeManager::getInstance().getcurrentTime() - l_tickTime;
			LogManager::getInstance().printLog(l_tickTime);
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
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
