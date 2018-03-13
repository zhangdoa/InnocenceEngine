#include "TaskManager.h"

void TaskManager::setup()
{
	m_hardwareConcurrency =	std::thread::hardware_concurrency();
	m_objectStatus = objectStatus::ALIVE;
}

void TaskManager::initialize()
{
	//for (unsigned int i = 0; i < m_hardwareConcurrency; ++i)
	//{
	//	m_threadPool.emplace_back(std::thread(&TaskManager::m_threadHolder, this));
	//}

	g_pLogManager->printLog("TaskManager has been initialized.");
}

void TaskManager::update()
{

}

void TaskManager::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;
	for (auto& i : m_threadPool)
	{
		if (i.joinable())
		{
			i.join();
		}
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogManager->printLog("TaskManager has been shutdown.");
}

const objectStatus & TaskManager::getStatus() const
{
	return m_objectStatus;
}

void TaskManager::addTask(std::function<void()>& task)
{
	m_mtx.lock();
	m_taskQueue.emplace_back(&task);
	m_mtx.unlock();
}

void TaskManager::m_threadHolder()
{
	do 
	{
		//if (m_taskQueue.size() > 0)
		//{
		//	(*m_taskQueue[0])();
		//}
	} 
	while (m_objectStatus == objectStatus::ALIVE);
}
