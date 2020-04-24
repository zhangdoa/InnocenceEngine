#include "ImGuiRendererGL.h"

#include "../ImGui/imgui_impl_opengl3.cpp"
#include "../../Component/GLRenderPassDataComponent.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiRendererGLNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererGL::setup()
{
	ImGuiRendererGLNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL setup finished.");

	return true;
}

bool ImGuiRendererGL::initialize()
{
	ImGui_ImplOpenGL3_Init(NULL);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL has been initialized.");

	return true;
}

bool ImGuiRendererGL::newFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	return true;
}

bool ImGuiRendererGL::render()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_userPipelineOutputRPDC = reinterpret_cast<GLRenderPassDataComponent*>(g_pModuleManager->getRenderingServer()->GetUserPipelineOutput());

	glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_userPipelineOutputRPDC->m_FBO);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiRendererGL::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGuiRendererGLNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererGL::getStatus()
{
	return ImGuiRendererGLNS::m_ObjectStatus;
}

void ImGuiRendererGL::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_RTSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
	auto l_developmentRTSize = ImVec2((float)l_screenResolution.x / 2.0f, (float)l_screenResolution.y / 2.0f);
	auto l_shadowRTSize = ImVec2(512.0, 512.0);
	auto l_BRDFLUTSize = ImVec2(128.0, 128.0);
}