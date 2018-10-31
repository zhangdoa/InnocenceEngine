#include "TaskSystem.h"
#include "../LowLevelSystem/LogSystem.h"

namespace InnoTaskSystem
{
	objectStatus m_TaskSystemStatus = objectStatus::SHUTDOWN;
}

InnoLowLevelSystem_EXPORT bool InnoTaskSystem::setup()
{
	m_TaskSystemStatus = objectStatus::ALIVE;
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoTaskSystem::initialize()
{
	InnoLogSystem::printLog("TaskSystem has been initialized.");
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoTaskSystem::update()
{
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoTaskSystem::terminate()
{
	m_TaskSystemStatus = objectStatus::STANDBY;
	m_TaskSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("TaskSystem has been terminated.");
	return true;
}

InnoLowLevelSystem_EXPORT objectStatus InnoTaskSystem::getStatus()
{
	return m_TaskSystemStatus;
}