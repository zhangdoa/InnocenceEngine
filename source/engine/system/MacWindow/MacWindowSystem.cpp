#include "MacWindowSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE MacWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	ButtonStatusMap m_buttonStatus;
}

bool MacWindowSystem::setup(void* hInstance, void* hwnd)
{
	MacWindowSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem setup finished.");

	return true;
}

bool MacWindowSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem has been initialized.");
	return true;
}

bool MacWindowSystem::update()
{
	return true;
}

bool MacWindowSystem::terminate()
{
	MacWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem has been terminated.");
	return true;
}

ObjectStatus MacWindowSystem::getStatus()
{
	return MacWindowSystemNS::m_objectStatus;
}

ButtonStatusMap MacWindowSystem::getButtonStatus()
{
	return MacWindowSystemNS::m_buttonStatus;
}

bool MacWindowSystem::sendEvent(unsigned int umsg, unsigned int WParam, int LParam)
{
	return true;
}

void MacWindowSystem::swapBuffer()
{
}
