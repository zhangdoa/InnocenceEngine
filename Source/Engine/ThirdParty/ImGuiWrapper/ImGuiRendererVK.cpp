#include "ImGuiRendererVK.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiRendererVKNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererVK::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererVKNS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK Setup finished.");

	return true;
}

bool ImGuiRendererVK::Initialize()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK has been initialized.");

	return true;
}

bool ImGuiRendererVK::NewFrame()
{
	return true;
}

bool ImGuiRendererVK::Render()
{
	return true;
}

bool ImGuiRendererVK::Terminate()
{
	ImGuiRendererVKNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererVK::GetStatus()
{
	return ImGuiRendererVKNS::m_ObjectStatus;
}

void ImGuiRendererVK::ShowRenderResult(RenderPassType renderPassType)
{
}