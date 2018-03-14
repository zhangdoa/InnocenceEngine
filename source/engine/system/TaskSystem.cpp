#include "TaskSystem.h"

void TaskSystem::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void TaskSystem::initialize()
{
	g_pLogSystem->printLog("TaskSystem has been initialized.");
}

void TaskSystem::update()
{
	for (auto i : m_taskQueue)
	{
		
	}
}

void TaskSystem::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("TaskSystem has been shutdown.");
}

const objectStatus & TaskSystem::getStatus() const
{
	return m_objectStatus;
}

void TaskSystem::addTask(void* task)
{
	m_mtx.lock();
	m_taskQueue.emplace_back(task);
	m_mtx.unlock();
}
