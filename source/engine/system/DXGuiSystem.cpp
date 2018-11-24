#include "DXGuiSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::setup()
{
	DXGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::terminate()
{
	DXGuiSystemNS::m_objectStatus = ObjectStatus::STANDBY;
	DXGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus DXGuiSystem::getStatus()
{
	return DXGuiSystemNS::m_objectStatus;
}