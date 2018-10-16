#include "TaskSystem.h"
#include "../../component/LogSystemSingletonComponent.h"

namespace InnoTaskSystem
{
	objectStatus m_TaskSystemStatus = objectStatus::SHUTDOWN;
}

void InnoTaskSystem::setup()
{
	m_TaskSystemStatus = objectStatus::ALIVE;
}

void InnoTaskSystem::initialize()
{
	LogSystemSingletonComponent::getInstance().m_log.push("TaskSystem has been initialized.");
}

void InnoTaskSystem::update()
{
}

void InnoTaskSystem::shutdown()
{
	m_TaskSystemStatus = objectStatus::STANDBY;
	m_TaskSystemStatus = objectStatus::SHUTDOWN;
	LogSystemSingletonComponent::getInstance().m_log.push("TaskSystem has been shutdown.");
}

objectStatus InnoTaskSystem::getStatus()
{
	return m_TaskSystemStatus;
}