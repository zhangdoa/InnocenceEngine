#include "ImGuiWindowMac.h"

#include "../../Engine.h"
using namespace Inno;
;

namespace ImGuiWindowMacNS
{
}

bool ImGuiWindowMac::Setup(ISystemConfig* systemConfig)
{
	Log(Success, "ImGuiWindowMac Setup finished.");

	return true;
}

bool ImGuiWindowMac::Initialize()
{
	Log(Success, "ImGuiWindowMac has been initialized.");

	return true;
}

bool ImGuiWindowMac::newFrame()
{
	return true;
}

bool ImGuiWindowMac::Terminate()
{
	Log(Success, "ImGuiWindowMac has been terminated.");

	return true;
}