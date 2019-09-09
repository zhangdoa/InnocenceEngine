#include "ImGuiWrapperMT.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperMTNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperMT::setup()
{
	ImGuiWrapperMTNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperMT setup finished.");

	return true;
}

bool ImGuiWrapperMT::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperMT has been initialized.");

	return true;
}

bool ImGuiWrapperMT::newFrame()
{
	return true;
}

bool ImGuiWrapperMT::render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	return true;
}

bool ImGuiWrapperMT::terminate()
{
	ImGuiWrapperMTNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperMT has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperMT::getStatus()
{
	return ImGuiWrapperMTNS::m_ObjectStatus;
}

void ImGuiWrapperMT::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_RTSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
	auto l_developmentRTSize = ImVec2((float)l_screenResolution.x / 2.0f, (float)l_screenResolution.y / 2.0f);
	auto l_shadowRTSize = ImVec2(512.0, 512.0);
	auto l_BRDFLUTSize = ImVec2(128.0, 128.0);
}

ImTextureID ImGuiWrapperMT::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID();
};
