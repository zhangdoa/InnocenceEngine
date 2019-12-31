#include "ImGuiWrapperWindowLinux.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWindowLinuxNS
{
}

bool ImGuiWrapperWindowLinux::setup()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowLinux setup finished.");

	return true;
}

bool ImGuiWrapperWindowLinux::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowLinux has been initialized.");

	return true;
}

bool ImGuiWrapperWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWrapperWindowLinux::terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowLinux has been terminated.");

	return true;
}
