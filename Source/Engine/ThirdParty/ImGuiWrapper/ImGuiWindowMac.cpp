#include "ImGuiWindowMac.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWindowMacNS
{
}

bool ImGuiWindowMac::Setup(ISystemConfig* systemConfig)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac Setup finished.");

	return true;
}

bool ImGuiWindowMac::Initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been initialized.");

	return true;
}

bool ImGuiWindowMac::newFrame()
{
	return true;
}

bool ImGuiWindowMac::Terminate()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been terminated.");

	return true;
}