#include "ImGuiRendererMT.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiRendererMTNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererMT::setup()
{
	ImGuiRendererMTNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT setup finished.");

	return true;
}

bool ImGuiRendererMT::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT has been initialized.");

	return true;
}

bool ImGuiRendererMT::newFrame()
{
	return true;
}

bool ImGuiRendererMT::render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	return true;
}

bool ImGuiRendererMT::terminate()
{
	ImGuiRendererMTNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererMT has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererMT::getStatus()
{
	return ImGuiRendererMTNS::m_ObjectStatus;
}

void ImGuiRendererMT::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}