#include "DXGuiSystem.h"
#include "../LowLevelSystem/LogSystem.h"

InnoHighLevelSystem_EXPORT bool DXGuiSystem::setup()
{
	m_objectStatus = objectStatus::ALIVE;
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::initialize()
{
	InnoLogSystem::printLog("DXGuiSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::update()
{
	return true;
}

InnoHighLevelSystem_EXPORT bool DXGuiSystem::terminate()
{
	m_objectStatus = objectStatus::STANDBY;
	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("DXGuiSystem has been terminated.");
	return true;
}

InnoHighLevelSystem_EXPORT objectStatus DXGuiSystem::getStatus()
{
	return m_objectStatus;
}