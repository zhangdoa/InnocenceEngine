#include "ImGuiWindowMac.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWindowMacNS
{
}

bool ImGuiWindowMac::setup()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac setup finished.");

	return true;
}

bool ImGuiWindowMac::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been initialized.");

	return true;
}

bool ImGuiWindowMac::newFrame()
{
	return true;
}

bool ImGuiWindowMac::terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been terminated.");

	return true;
}
