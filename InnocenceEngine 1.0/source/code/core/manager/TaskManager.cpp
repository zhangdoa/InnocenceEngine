#include "../../main/stdafx.h"
#include "TaskManager.h"


TaskManager::TaskManager()
{
}

void TaskManager::setup()
{
	m_hardwareConcurrency =	std::thread::hardware_concurrency();
	this->setStatus(objectStatus::ALIVE);
}

void TaskManager::initialize()
{
	for (unsigned int i = 0; i < m_hardwareConcurrency - 1; ++i)
	{
		m_threadPool.emplace_back(std::thread(&TaskManager::threadHolder, this));
	}
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

void TaskManager::threadHolder()
{
	do 
	{
		if (m_taskQueue.size() > 0)
		{
		}
		else
		{
			//LogManager::getInstance().printLog(std::this_thread::get_id());
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	} 
	while (this->getStatus() == objectStatus::ALIVE);
}
