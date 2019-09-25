#include "ImGuiWrapperGL.h"

#include "../ThirdParty/ImGui/imgui_impl_opengl3.cpp"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperGLNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperGL::setup()
{
	ImGuiWrapperGLNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperGL setup finished.");

	return true;
}

bool ImGuiWrapperGL::initialize()
{
	ImGui_ImplOpenGL3_Init(NULL);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperGL has been initialized.");

	return true;
}

bool ImGuiWrapperGL::newFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	return true;
}

bool ImGuiWrapperGL::render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	//glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperGL::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGuiWrapperGLNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperGL::getStatus()
{
	return ImGuiWrapperGLNS::m_ObjectStatus;
}

void ImGuiWrapperGL::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_RTSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
	auto l_developmentRTSize = ImVec2((float)l_screenResolution.x / 2.0f, (float)l_screenResolution.y / 2.0f);
	auto l_shadowRTSize = ImVec2(512.0, 512.0);
	auto l_BRDFLUTSize = ImVec2(128.0, 128.0);
}