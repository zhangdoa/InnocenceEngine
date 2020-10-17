#include "ImGuiWindowMac.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiWindowMacNS
{
}

bool ImGuiWindowMac::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac Setup finished.");

	return true;
}

bool ImGuiWindowMac::Initialize()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been initialized.");

	return true;
}

bool ImGuiWindowMac::newFrame()
{
	return true;
}

bool ImGuiWindowMac::Terminate()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowMac has been terminated.");

	return true;
}