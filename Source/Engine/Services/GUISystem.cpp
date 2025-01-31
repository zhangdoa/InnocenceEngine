#include "GUISystem.h"
#include "../ThirdParty/ImGuiWrapper/ImGuiWrapper.h"
#include "HIDService.h"

#include "../Engine.h"
using namespace Inno;

namespace GUISystemNS
{
	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;
}

using namespace GUISystemNS;

bool GUISystem::Setup(ISystemConfig* systemConfig)
{
	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
		};
	g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_I, true }, ButtonEvent{ EventLifeTime::OneShot, &f_toggleshowImGui });

	return 	ImGuiWrapper::Get().Setup();
}

bool GUISystem::Initialize()
{
	return ImGuiWrapper::Get().Initialize();
}

bool GUISystem::Update()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().Prepare();
	}

	return true;
}

bool GUISystem::ExecuteCommands()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().ExecuteCommands();
	}

	return true;
}

bool GUISystem::Terminate()
{
	return ImGuiWrapper::Get().Terminate();
}

ObjectStatus GUISystem::GetStatus()
{
	return ObjectStatus();
}