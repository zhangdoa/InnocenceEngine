#include "TaskSystem.h"
#include "../LowLevelSystem/LogSystem.h"

namespace InnoTaskSystem
{
	objectStatus m_TaskSystemStatus = objectStatus::SHUTDOWN;
}

InnoLowLevelSystem_EXPORT void InnoTaskSystem::setup()
{
	m_TaskSystemStatus = objectStatus::ALIVE;
}

InnoLowLevelSystem_EXPORT void InnoTaskSystem::initialize()
{
	InnoLogSystem::printLog("TaskSystem has been initialized.");
}

InnoLowLevelSystem_EXPORT void InnoTaskSystem::update()
{
}

InnoLowLevelSystem_EXPORT void InnoTaskSystem::shutdown()
{
	m_TaskSystemStatus = objectStatus::STANDBY;
	m_TaskSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("TaskSystem has been shutdown.");
}

InnoLowLevelSystem_EXPORT objectStatus InnoTaskSystem::getStatus()
{
	return m_TaskSystemStatus;
}