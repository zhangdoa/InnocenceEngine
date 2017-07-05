#include "stdafx.h"
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

void CoreManager::init()
{
	m_childEventManager.emplace_back(&m_timeManager);
	m_childEventManager.emplace_back(&m_graphicManager); 
	m_childEventManager.emplace_back(&m_sceneGraphManager);

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(INIT);
	}

	try {
		m_gameData->exec(INIT);
	}
	catch (std::exception& e) {
		LogManager::printLog("No game added!");
		LogManager::printLog(e.what());
	}

	this->setStatus(INITIALIZIED);
	LogManager::printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	if(m_graphicManager.getStatus() == INITIALIZIED)
	{
		m_timeManager.exec(UPDATE);
		
		if (m_timeManager.getStatus() == INITIALIZIED)
		{
			m_gameData->exec(UPDATE);
			m_graphicManager.exec(UPDATE);
			m_sceneGraphManager.exec(UPDATE);
			
		}
	}
	else 
	{
		this->setStatus(STANDBY);
		LogManager::printLog("CoreManager is stand-by.");
	}
}

void CoreManager::shutdown()
{
	m_gameData->exec(SHUTDOWN);
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->exec(SHUTDOWN);
	}
	LogManager::printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
