#include "MacWindowService.h"
#include "../../Common/Array.h"
#include "../../Common/UnorderedSet.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace MacWindowServiceNS
{
	IWindowSurface* m_WindowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_InitConfig;
	Inno::Array<ButtonState> m_ButtonStates;
	Inno::UnorderedSet<WindowEventCallback*> m_WindowEventCallbacks;

	MacWindowServiceBridge* m_bridge;
}

bool MacWindowService::Setup(IServiceConfig* systemConfig)
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	bool result = MacWindowServiceNS::m_bridge->Setup(l_screenResolution.x, l_screenResolution.y);

	MacWindowServiceNS::m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "MacWindowService Setup finished.");

	return true;
}

bool MacWindowService::Initialize()
{
	bool result = MacWindowServiceNS::m_bridge->Initialize();

	MacWindowServiceNS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "MacWindowService has been initialized.");
	return true;
}

bool MacWindowService::Update()
{
	bool result = MacWindowServiceNS::m_bridge->Update();
	return true;
}

bool MacWindowService::Terminate()
{
	bool result = MacWindowServiceNS::m_bridge->Terminate();
	MacWindowServiceNS::m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "MacWindowService has been terminated.");
	return true;
}

ObjectStatus MacWindowService::GetStatus()
{
	return MacWindowServiceNS::m_ObjectStatus;
}

IWindowSurface* MacWindowService::GetWindowSurface()
{
	return MacWindowServiceNS::m_WindowSurface;
}

const Inno::Array<ButtonState>& MacWindowService::GetButtonState()
{
	return MacWindowServiceNS::m_ButtonStates;
}

bool MacWindowService::SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	return true;
}

bool MacWindowService::AddEventCallback(WindowEventCallback* callback)
{
	MacWindowServiceNS::m_WindowEventCallbacks.emplace(functor);
	return true;
}

void MacWindowService::setBridge(MacWindowServiceBridge* bridge)
{
	MacWindowServiceNS::m_bridge = bridge;
	Log(Success, "Bridge connected at ", bridge);
}