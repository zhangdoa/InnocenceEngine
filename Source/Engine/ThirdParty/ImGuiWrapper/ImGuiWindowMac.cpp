#include "ImGuiWindowMac.h"

#include "../../Engine.h"
using namespace Inno;
;

namespace ImGuiWindowMacNS
{
}

bool ImGuiWindowMac::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowMac Setup finished.");

	return true;
}

bool ImGuiWindowMac::Initialize()
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowMac has been initialized.");

	return true;
}

bool ImGuiWindowMac::newFrame()
{
	return true;
}

bool ImGuiWindowMac::Terminate()
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiWindowMac has been terminated.");

	return true;
}