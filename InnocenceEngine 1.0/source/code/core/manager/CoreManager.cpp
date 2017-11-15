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

TimeManager & CoreManager::getTimeManager() const
{
	return TimeManager::getInstance();
}

RenderingManager & CoreManager::getRenderingManager() const
{
	return RenderingManager::getInstance();
}

LogManager & CoreManager::getLogManager() const
{
	return LogManager::getInstance();
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
		m_childEventManager[i].get()->excute(executeMessage::INITIALIZE);
	}

	try {
		m_gameData->excute(executeMessage::INITIALIZE);
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
	TimeManager::getInstance().excute(executeMessage::UPDATE);

	// when time manager's status was execMessage::INITIALIZED, that means we can update other managers, a frame counter occurred.
	if (TimeManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		if (RenderingManager::getInstance().getStatus() == objectStatus::ALIVE)
		{			
			// game data update
			m_gameData->excute(executeMessage::UPDATE);
			RenderingManager::getInstance().excute(executeMessage::UPDATE);
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
	m_gameData->excute(executeMessage::SHUTDOWN);
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->excute(executeMessage::SHUTDOWN);
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
