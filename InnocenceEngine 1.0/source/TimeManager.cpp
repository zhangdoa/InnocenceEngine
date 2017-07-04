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
	printLog("TimeManager has been initialized.");
}

void TimeManager::update()
{
	m_startTime = clock::now();
	m_deltaTime = (clock::now() - m_startTime).count() / 1000.0;
}

void TimeManager::shutdown()
{
	printLog("TimeManager has been shutdown.");
}