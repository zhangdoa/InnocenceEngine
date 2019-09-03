#include "ImGuiWrapperWindowMac.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWindowMacNS
{
}

bool ImGuiWrapperWindowMac::setup()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowMac setup finished.");

	return true;
}

bool ImGuiWrapperWindowMac::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowMac has been initialized.");

	return true;
}

bool ImGuiWrapperWindowMac::newFrame()
{
	return true;
}

bool ImGuiWrapperWindowMac::terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowMac has been terminated.");

	return true;
}
