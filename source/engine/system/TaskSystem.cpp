#include "TaskSystem.h"

void TaskSystem::setup()
{
	m_threadPool = g_pMemorySystem->spawn<InnoThreadPool>();
	m_objectStatus = objectStatus::ALIVE;
}

void TaskSystem::initialize()
{
	g_pLogSystem->printLog("TaskSystem has been initialized.");
}

void TaskSystem::update()
{
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
