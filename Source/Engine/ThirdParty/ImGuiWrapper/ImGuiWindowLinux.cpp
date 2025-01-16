#include "ImGuiWindowLinux.h"

#include "../ModuleManager/Engine.h"
using namespace Inno;
;

namespace ImGuiWindowLinuxNS
{
}

bool ImGuiWindowLinux::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowLinux Setup finished.");

	return true;
}

bool ImGuiWindowLinux::Initialize()
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowLinux has been initialized.");

	return true;
}

bool ImGuiWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWindowLinux::Terminate()
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowLinux has been terminated.");

	return true;
}