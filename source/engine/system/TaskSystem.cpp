#include "TaskSystem.h"

void TaskSystem::setup()
{
	m_hardwareConcurrency =	std::thread::hardware_concurrency();
	m_objectStatus = objectStatus::ALIVE;
}

void TaskSystem::initialize()
{
	//for (unsigned int i = 0; i < m_hardwareConcurrency; ++i)
	//{
	//	m_threadPool.emplace_back(std::thread(&TaskSystem::m_threadHolder, this));
	//}

	g_pLogSystem->printLog("TaskSystem has been initialized.");
}

void TaskSystem::update()
{

}

void TaskSystem::shutdown()
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
	g_pLogSystem->printLog("TaskSystem has been shutdown.");
}

const objectStatus & TaskSystem::getStatus() const
{
	return m_objectStatus;
}

void TaskSystem::addTask(std::function<void()>& task)
{
	m_mtx.lock();
	m_taskQueue.emplace_back(&task);
	m_mtx.unlock();
}

void TaskSystem::m_threadHolder()
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
