#include "stdafx.h"
#include "TimeManager.h"


TimeManager::TimeManager()
{
}


TimeManager::~TimeManager()
{
}

void TimeManager::init()
{
	m_gameStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	setStatus(INITIALIZIED);
	printLog("TimeManager has been initialized.");
}

void TimeManager::update()
{
	m_updateStartTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = (std::chrono::high_resolution_clock::now() - m_updateStartTime).count() / 1000.0;

	m_unprocessedTime += getDeltaTime();

	if (m_unprocessedTime > m_frameTime)
	{
		m_unprocessedTime -= m_frameTime;
		setStatus(INITIALIZIED);
	}
	else 
	{
		setStatus(STANDBY);
	}
}

void TimeManager::shutdown()
{
	setStatus(UNINITIALIZIED);
	printLog("TimeManager has been shutdown.");
}

const double TimeManager::getGameStartTime()
{
	return m_gameStartTime;
}

const double TimeManager::getDeltaTime()
{
	return m_deltaTime;
}
