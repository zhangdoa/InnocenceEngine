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
	m_childManager.emplace_back(&m_uiManager); 
	m_childManager.emplace_back(&m_renderingManager);

	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->exec(INIT);
	}
	printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	while (m_uiManager.getWindow() != nullptr)
	{
		m_unprocessedTime = 0.0;
		m_unprocessedTime += m_timeManager.getDeltaTime();

		if (m_unprocessedTime > m_frameTime)
		{
			m_unprocessedTime -= m_frameTime;
			this->setStatus(RUNNING);		
		}
		for (size_t i = 0; i < m_childManager.size(); i++)
		{
			m_childManager[i].get()->exec(UPDATE);
		}	
	}
	this->setStatus(STANDBY);
}

void CoreManager::shutdown()
{
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[m_childManager.size() - 1 - i].get()->exec(SHUTDOWN);
	}
	printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(3));
}
