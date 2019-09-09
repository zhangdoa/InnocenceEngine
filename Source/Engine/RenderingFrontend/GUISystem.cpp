#include "GUISystem.h"
#include "../ImGuiWrapper/ImGuiWrapper.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace GUISystemNS
{
	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;
}

using namespace GUISystemNS;

bool InnoGUISystem::setup()
{
	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
	};
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_I, true }, ButtonEvent{ EventLifeTime::OneShot, &f_toggleshowImGui });

	return 	ImGuiWrapper::get().setup();
}

bool InnoGUISystem::initialize()
{
	return ImGuiWrapper::get().initialize();
}

bool InnoGUISystem::update()
{
	if (m_showImGui)
	{
		ImGuiWrapper::get().update();
	}

	return true;
}

bool InnoGUISystem::render()
{
	if (m_showImGui)
	{
		ImGuiWrapper::get().render();
	}
	return true;
}

bool InnoGUISystem::terminate()
{
	return ImGuiWrapper::get().terminate();
}

ObjectStatus InnoGUISystem::getStatus()
{
	return ObjectStatus();
}