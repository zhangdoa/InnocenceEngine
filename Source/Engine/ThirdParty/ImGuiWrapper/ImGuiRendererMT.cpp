#include "ImGuiRendererMT.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiRendererMTNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererMT::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererMTNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT Setup finished.");

	return true;
}

bool ImGuiRendererMT::Initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT has been initialized.");

	return true;
}

bool ImGuiRendererMT::NewFrame()
{
	return true;
}

bool ImGuiRendererMT::Render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	return true;
}

bool ImGuiRendererMT::Terminate()
{
	ImGuiRendererMTNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererMT::GetStatus()
{
	return ImGuiRendererMTNS::m_ObjectStatus;
}

void ImGuiRendererMT::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}