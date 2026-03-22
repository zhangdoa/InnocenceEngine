#include "GUIService.h"
#include "../ThirdParty/ImGuiWrapper/ImGuiWrapper.h"
#include "HIDService.h"

#include "../Engine.h"
using namespace Inno;

namespace GUIServiceNS
{
	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;
}

using namespace GUIServiceNS;

bool GUIService::Setup(IServiceConfig* systemConfig)
{
	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
		};
	g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_I, true }, ButtonEvent{ EventLifeTime::OneShot, &f_toggleshowImGui });

	return 	ImGuiWrapper::Get().Setup();
}

bool GUIService::Initialize()
{
	return ImGuiWrapper::Get().Initialize();
}

bool GUIService::Update()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().Prepare();
	}

	return true;
}

bool GUIService::ExecuteCommands()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().ExecuteCommands();
	}

	return true;
}

bool GUIService::Terminate()
{
	return ImGuiWrapper::Get().Terminate();
}

ObjectStatus GUIService::GetStatus()
{
	return ObjectStatus();
}