#include "MacWindowSystem.h"

#include "../../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace MacWindowSystemNS
{
	IWindowSurface* m_windowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
	std::vector<ButtonState> m_buttonState;
	std::set<WindowEventCallbackFunctor*> m_windowEventCallbackFunctor;

  MacWindowSystemBridge* m_bridge;
}

bool MacWindowSystem::setup(void* hInstance, void* hwnd)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
    bool result = MacWindowSystemNS::m_bridge->setup(l_screenResolution.x, l_screenResolution.y);

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem setup finished.");

	return true;
}

bool MacWindowSystem::initialize()
{
	bool result = MacWindowSystemNS::m_bridge->initialize();

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem has been initialized.");
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
	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem has been terminated.");
	return true;
}

ObjectStatus MacWindowSystem::getStatus()
{
	return MacWindowSystemNS::m_ObjectStatus;
}

IWindowSurface * MacWindowSystem::getWindowSurface()
{
	return MacWindowSystemNS::m_windowSurface;
}

const std::vector<ButtonState>& MacWindowSystem::getButtonState()
{
	return MacWindowSystemNS::m_buttonState;
}

bool MacWindowSystem::sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam)
{
	return true;
}

bool MacWindowSystem::addEventCallback(WindowEventCallbackFunctor* functor)
{
	MacWindowSystemNS::m_windowEventCallbackFunctor.emplace(functor);
	return true;
}

void MacWindowSystem::setBridge(MacWindowSystemBridge* bridge)
{
	MacWindowSystemNS::m_bridge = bridge;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem: Bridge connected at ", bridge);
}
