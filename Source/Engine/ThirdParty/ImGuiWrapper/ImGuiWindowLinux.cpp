#include "ImGuiWindowLinux.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWindowLinuxNS
{
}

bool ImGuiWindowLinux::Setup(ISystemConfig* systemConfig)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux Setup finished.");

	return true;
}

bool ImGuiWindowLinux::Initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been initialized.");

	return true;
}

bool ImGuiWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWindowLinux::Terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been terminated.");

	return true;
}