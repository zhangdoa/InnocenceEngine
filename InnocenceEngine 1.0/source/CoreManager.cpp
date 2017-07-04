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
	m_childManager.emplace_back(&m_timeManager);
	m_childManager.emplace_back(&m_graphicManager); 

	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->exec(INIT);
	}

	this->setStatus(INITIALIZIED);
	printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	if(m_graphicManager.getStatus() == INITIALIZIED)
	{
		m_unprocessedTime = 0.0;
		m_unprocessedTime += m_timeManager.getDeltaTime();

		if (m_unprocessedTime > m_frameTime)
		{
			m_unprocessedTime -= m_frameTime;	
		}
		for (size_t i = 0; i < m_childManager.size(); i++)
		{
			m_childManager[i].get()->exec(UPDATE);
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
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[m_childManager.size() - 1 - i].get()->exec(SHUTDOWN);
	}
	printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
