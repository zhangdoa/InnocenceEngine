#include "DXGuiSystem.h"
#include "../LowLevelSystem/LogSystem.h"

namespace DXGuiSystem
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

void DXGuiSystem::Instance::setup()
{
}

void DXGuiSystem::Instance::initialize()
{
	InnoLogSystem::printLog("DXGuiSystem has been initialized.");
}

void DXGuiSystem::Instance::update()
{
}

void DXGuiSystem::Instance::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("DXGuiSystem has been shutdown.");
}

objectStatus DXGuiSystem::Instance::getStatus()
{
	return m_objectStatus;
}