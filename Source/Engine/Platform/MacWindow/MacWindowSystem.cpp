#include "MacWindowSystem.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace MacWindowSystemNS
{
	IWindowSurface* m_WindowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_InitConfig;
	std::vector<ButtonState> m_ButtonStates;
	std::set<WindowEventCallback*> m_WindowEventCallbacks;

	MacWindowSystemBridge* m_bridge;
}

bool MacWindowSystem::Setup(ISystemConfig* systemConfig)
{
	auto l_screenResolution = g_Engine->Get<RenderingFrontend>()->GetScreenResolution();
	bool result = MacWindowSystemNS::m_bridge->Setup(l_screenResolution.x, l_screenResolution.y);

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "MacWindowSystem Setup finished.");

	return true;
}

bool MacWindowSystem::Initialize()
{
	bool result = MacWindowSystemNS::m_bridge->Initialize();

	MacWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "MacWindowSystem has been initialized.");
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
	Log(Success, "MacWindowSystem has been terminated.");
	return true;
}

ObjectStatus MacWindowSystem::GetStatus()
{
	return MacWindowSystemNS::m_ObjectStatus;
}

IWindowSurface* MacWindowSystem::GetWindowSurface()
{
	return MacWindowSystemNS::m_WindowSurface;
}

const std::vector<ButtonState>& MacWindowSystem::GetButtonState()
{
	return MacWindowSystemNS::m_ButtonStates;
}

bool MacWindowSystem::SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	return true;
}

bool MacWindowSystem::AddEventCallback(WindowEventCallback* callback)
{
	MacWindowSystemNS::m_WindowEventCallbacks.emplace(functor);
	return true;
}

void MacWindowSystem::setBridge(MacWindowSystemBridge* bridge)
{
	MacWindowSystemNS::m_bridge = bridge;
	Log(Success, "Bridge connected at ", bridge);
}