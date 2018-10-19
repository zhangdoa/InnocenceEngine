#include "DXGuiSystem.h"
#include "../LowLevelSystem/LogSystem.h"

namespace DXGuiSystem
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::Instance::setup()
{
	m_objectStatus = objectStatus::ALIVE;
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::Instance::initialize()
{
	InnoLogSystem::printLog("DXGuiSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::Instance::update()
{
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::Instance::terminate()
{
	m_objectStatus = objectStatus::STANDBY;
	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("DXGuiSystem has been terminated.");
	return true;
}

InnoHighLevelSystem_EXPORT objectStatus DXGuiSystem::Instance::getStatus()
{
	return m_objectStatus;
}