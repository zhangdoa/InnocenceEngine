#include "MacWindowSystem.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE MacWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	ButtonStatusMap m_buttonStatus;

  MacWindowSystemBridge* m_bridge;
}

bool MacWindowSystem::setup(void* hInstance, void* hwnd)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
  bool result = MacWindowSystemNS::m_bridge->setup(l_screenResolution.x, l_screenResolution.y);

	MacWindowSystemNS::m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem setup finished.");

	return true;
}

bool MacWindowSystem::initialize()
{
	bool result = MacWindowSystemNS::m_bridge->initialize();

	MacWindowSystemNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem has been initialized.");
	return true;
}

bool MacWindowSystem::update()
{
	bool result = MacWindowSystemNS::m_bridge->update();
	return true;
}

bool MacWindowSystem::terminate()
{
	bool result = MacWindowSystemNS::m_bridge->terminate();
	MacWindowSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem has been terminated.");
	return true;
}

ObjectStatus MacWindowSystem::getStatus()
{
	return MacWindowSystemNS::m_objectStatus;
}

IWindowSurface * MacWindowSystem::getWindowSurface()
{
	// @TODO: return a windows surface wrapper instance
	return nullptr;
}

ButtonStatusMap MacWindowSystem::getButtonStatus()
{
	return MacWindowSystemNS::m_buttonStatus;
}

bool MacWindowSystem::sendEvent(unsigned int umsg, unsigned int WParam, int LParam)
{
	return true;
}

void MacWindowSystem::setBridge(MacWindowSystemBridge* bridge)
{
	MacWindowSystemNS::m_bridge = bridge;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MacWindowSystem: Bridge connected at " + InnoUtility::pointerToString(bridge));
}
