#include "MacWindowSystem.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace MacWindowSystemNS
{
	IWindowSurface* m_windowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
	std::vector<ButtonState> m_buttonState;
	std::set<WindowEventCallbackFunctor*> m_windowEventCallbackFunctor;

	MacWindowSystemBridge* m_bridge;
}

bool MacWindowSystem::Setup(ISystemConfig* systemConfig)
{
	auto l_screenResolution = g_Engine->getRenderingFrontend()->GetScreenResolution();
	bool result = MacWindowSystemNS::m_bridge->Setup(l_screenResolution.x, l_screenResolution.y);

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Created;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem Setup finished.");

	return true;
}

bool MacWindowSystem::Initialize()
{
	bool result = MacWindowSystemNS::m_bridge->Initialize();

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem has been initialized.");
	return true;
}

bool MacWindowSystem::Update()
{
	bool result = MacWindowSystemNS::m_bridge->Update();
	return true;
}

bool MacWindowSystem::Terminate()
{
	bool result = MacWindowSystemNS::m_bridge->Terminate();
	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem has been terminated.");
	return true;
}

ObjectStatus MacWindowSystem::GetStatus()
{
	return MacWindowSystemNS::m_ObjectStatus;
}

IWindowSurface* MacWindowSystem::getWindowSurface()
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
	g_Engine->getLogSystem()->Log(LogLevel::Success, "MacWindowSystem: Bridge connected at ", bridge);
}