#include "GUISystem.h"
#include "../ThirdParty/ImGuiWrapper/ImGuiWrapper.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace GUISystemNS
{
	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;
}

using namespace GUISystemNS;

bool InnoGUISystem::Setup(ISystemConfig* systemConfig)
{
	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
	};
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_I, true }, ButtonEvent{ EventLifeTime::OneShot, &f_toggleshowImGui });

	return 	ImGuiWrapper::Get().Setup();
}

bool InnoGUISystem::Initialize()
{
	return ImGuiWrapper::Get().Initialize();
}

bool InnoGUISystem::Update()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().Update();
	}

	return true;
}

bool InnoGUISystem::Render()
{
	if (m_showImGui)
	{
		ImGuiWrapper::Get().Render();
	}
	return true;
}

bool InnoGUISystem::Terminate()
{
	return ImGuiWrapper::Get().Terminate();
}

ObjectStatus InnoGUISystem::GetStatus()
{
	return ObjectStatus();
}