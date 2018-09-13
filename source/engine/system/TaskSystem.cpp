#include "TaskSystem.h"
#include "MemorySystem.h"
#include "LogSystem.h"
#include "AssetSystem.h"
#include <iostream>

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
	InnoLogSystem::printLog("TaskSystem has been initialized.");
}

void InnoTaskSystem::update()
{
}

void InnoTaskSystem::shutdown()
{
	m_TaskSystemStatus = objectStatus::STANDBY;
	m_TaskSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("TaskSystem has been shutdown.");
}

objectStatus InnoTaskSystem::getStatus()
{
	return m_TaskSystemStatus;
}