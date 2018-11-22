#include "DXGuiSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXGuiSystemNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::setup()
{
	DXGuiSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "DXGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::terminate()
{
	DXGuiSystemNS::m_objectStatus = objectStatus::STANDBY;
	DXGuiSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "DXGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT objectStatus DXGuiSystem::getStatus()
{
	return DXGuiSystemNS::m_objectStatus;
}