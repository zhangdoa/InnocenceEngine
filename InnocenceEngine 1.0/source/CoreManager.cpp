#include "stdafx.h"
#include "CoreManager.h"


CoreManager::CoreManager()
{
}


CoreManager::~CoreManager()
{
}

void CoreManager::init()
{
	m_childEventManager.emplace_back(&m_timeManager);
	m_childEventManager.emplace_back(&m_graphicManager); 

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(INIT);
	}

	this->setStatus(INITIALIZIED);
	printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	if(m_graphicManager.getStatus() == INITIALIZIED)
	{
		m_timeManager.exec(UPDATE);
		
		if (m_timeManager.getStatus() == INITIALIZIED)
		{
			m_graphicManager.exec(UPDATE);
		}
	}
	else 
	{
		this->setStatus(STANDBY);
		printLog("CoreManager is stand-by.");
	}
}

void CoreManager::shutdown()
{
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->exec(SHUTDOWN);
	}
	printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
