#include "../../main/stdafx.h"
#include "TaskManager.h"


TaskManager::TaskManager()
{
}

void TaskManager::setup()
{
	m_hardwareConcurrency =	std::thread::hardware_concurrency();
	for (auto i = 0; i < m_hardwareConcurrency; ++i)
	{
		//LogManager::getInstance().printLog(i);
		m_threadPool.emplace_back(std::thread());
	}
	
}

void TaskManager::initialize()
{
}

void TaskManager::update()
{
}

void TaskManager::shutdown()
{
}


TaskManager::~TaskManager()
{
}
