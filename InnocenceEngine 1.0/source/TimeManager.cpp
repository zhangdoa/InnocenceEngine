#include "stdafx.h"
#include "TimeManager.h"


TimeManager::TimeManager()
{
}


TimeManager::~TimeManager()
{
}

const double TimeManager::getDeltaTime()
{
	return m_deltaTime;
}

void TimeManager::init()
{
	setStatus(INITIALIZIED);
	printLog("TimeManager has been initialized.");
}

void TimeManager::update()
{
	m_startTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = (std::chrono::high_resolution_clock::now() - m_startTime).count() / 1000.0;
}

void TimeManager::shutdown()
{
	setStatus(UNINITIALIZIED);
	printLog("TimeManager has been shutdown.");
}