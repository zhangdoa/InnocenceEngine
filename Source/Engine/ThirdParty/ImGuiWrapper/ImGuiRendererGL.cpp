#include "ImGuiRendererGL.h"

#include "../../Component/GLRenderPassComponent.h"
#include "../ImGui/imgui_impl_opengl3.cpp"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiRendererGLNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererGL::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererGLNS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL Setup finished.");

	return true;
}

bool ImGuiRendererGL::Initialize()
{
	ImGui_ImplOpenGL3_Init(NULL);
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL has been initialized.");

	return true;
}

bool ImGuiRendererGL::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	return true;
}

bool ImGuiRendererGL::Render()
{
	auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_userPipelineOutputRenderPassComp = reinterpret_cast<GLRenderPassComponent*>(g_Engine->getRenderingServer()->GetUserPipelineOutput());

	glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_userPipelineOutputRenderPassComp->m_FBO);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiRendererGL::Terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGuiRendererGLNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererGL has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererGL::GetStatus()
{
	return ImGuiRendererGLNS::m_ObjectStatus;
}

void ImGuiRendererGL::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_RTSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
	auto l_developmentRTSize = ImVec2((float)l_screenResolution.x / 2.0f, (float)l_screenResolution.y / 2.0f);
	auto l_shadowRTSize = ImVec2(512.0, 512.0);
	auto l_BRDFLUTSize = ImVec2(128.0, 128.0);
}