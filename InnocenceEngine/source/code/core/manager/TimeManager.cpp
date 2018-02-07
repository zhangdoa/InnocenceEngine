#include "../../main/stdafx.h"
#include "TimeManager.h"


void TimeManager::setup()
{
}

void TimeManager::initialize()
{
	m_gameStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("TimeManager has been initialized.");
}

void TimeManager::update()
{
	m_updateStartTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = (std::chrono::high_resolution_clock::now() - m_updateStartTime).count();

	m_unprocessedTime += m_deltaTime;

	if (m_unprocessedTime >= m_frameTime)
	{
		m_unprocessedTime -= m_frameTime;
		setStatus(objectStatus::ALIVE);
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void TimeManager::shutdown()
{
	setStatus(objectStatus::SHUTDOWN);
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

const double TimeManager::getcurrentTime() const
{
	return (std::chrono::high_resolution_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
}
