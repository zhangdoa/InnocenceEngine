#include "../../main/stdafx.h"
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
	setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("TimeManager has been initialized.");
}

void TimeManager::update()
{
	m_updateStartTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = (std::chrono::high_resolution_clock::now() - m_updateStartTime).count() / 1000.0;

	m_unprocessedTime += getDeltaTime();

	if (m_unprocessedTime > m_frameTime)
	{
		m_unprocessedTime -= m_frameTime;
		setStatus(objectStatus::INITIALIZIED);
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void TimeManager::shutdown()
{
	setStatus(objectStatus::UNINITIALIZIED);
	LogManager::getInstance().printLog("TimeManager has been shutdown.");
}

const __time64_t TimeManager::getGameStartTime() const
{
	return m_gameStartTime;
}

const double TimeManager::getDeltaTime() const
{
	return m_deltaTime;
}
