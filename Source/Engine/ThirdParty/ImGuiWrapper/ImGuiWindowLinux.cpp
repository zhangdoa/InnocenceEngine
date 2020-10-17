#include "ImGuiWindowLinux.h"

#include "../ModuleManager/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiWindowLinuxNS
{
}

bool ImGuiWindowLinux::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux Setup finished.");

	return true;
}

bool ImGuiWindowLinux::Initialize()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been initialized.");

	return true;
}

bool ImGuiWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWindowLinux::Terminate()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowLinux has been terminated.");

	return true;
}