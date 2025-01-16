#include "ImGuiWindowLinux.h"

#include "../ModuleManager/Engine.h"
using namespace Inno;
;

namespace ImGuiWindowLinuxNS
{
}

bool ImGuiWindowLinux::Setup(ISystemConfig* systemConfig)
{
	Log(Success, "ImGuiWindowLinux Setup finished.");

	return true;
}

bool ImGuiWindowLinux::Initialize()
{
	Log(Success, "ImGuiWindowLinux has been initialized.");

	return true;
}

bool ImGuiWindowLinux::newFrame()
{
	return true;
}

bool ImGuiWindowLinux::Terminate()
{
	Log(Success, "ImGuiWindowLinux has been terminated.");

	return true;
}