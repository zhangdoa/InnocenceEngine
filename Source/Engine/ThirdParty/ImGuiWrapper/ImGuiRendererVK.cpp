#include "ImGuiRendererVK.h"

#include "../../Engine.h"
using namespace Inno;
;

namespace ImGuiRendererVKNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererVK::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererVKNS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiRendererVK Setup finished.");

	return true;
}

bool ImGuiRendererVK::Initialize()
{
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiRendererVK has been initialized.");

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
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "ImGuiRendererVK has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererVK::GetStatus()
{
	return ImGuiRendererVKNS::m_ObjectStatus;
}

void ImGuiRendererVK::ShowRenderResult(RenderPassType renderPassType)
{
}