#include "ImGuiWrapperWinGL.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../ThirdParty/ImGui/imgui_impl_win32.h"
#include "../ThirdParty/ImGui/imgui_impl_opengl3.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWinGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperWinGL::setup()
{
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinGL setup finished.");

	return true;
}

bool ImGuiWrapperWinGL::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	ImGui_ImplOpenGL3_Init(NULL);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinGL has been initialized.");

	return true;
}

bool ImGuiWrapperWinGL::newFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	return true;
}

bool ImGuiWrapperWinGL::render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	//glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperWinGL::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperWinGL::getStatus()
{
	return ImGuiWrapperWinGLNS::m_objectStatus;
}

void ImGuiWrapperWinGL::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_RTSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
	auto l_developmentRTSize = ImVec2((float)l_screenResolution.x / 2.0f, (float)l_screenResolution.y / 2.0f);
	auto l_shadowRTSize = ImVec2(512.0, 512.0);
	auto l_BRDFLUTSize = ImVec2(128.0, 128.0);
}

ImTextureID ImGuiWrapperWinGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID();
};