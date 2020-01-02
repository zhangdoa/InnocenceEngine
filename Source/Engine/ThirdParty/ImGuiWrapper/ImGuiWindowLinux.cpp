#include "ImGuiWindowLinux.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWindowLinuxNS
{
}

bool ImGuiWindowLinux::setup()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux setup finished.");

	return true;
}

bool ImGuiWindowLinux::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been initialized.");

	return true;
}

bool ImGuiWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWindowLinux::terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been terminated.");

	return true;
}