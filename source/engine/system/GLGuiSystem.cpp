#include "GLGuiSystem.h"
#include "../component/GLWindowSystemComponent.h"
#include "../component/GLEnvironmentRenderPassComponent.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GLTerrainRenderPassComponent.h"
#include "../component/GLLightRenderPassComponent.h"
#include "../component/GLFinalRenderPassComponent.h"
#include "../component/GLRenderingSystemComponent.h"

#include "ImGuiWrapper.h"
#include "../third-party/ImGui/imgui_impl_glfw.h"
#include "../third-party/ImGui/imgui_impl_opengl3.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	void showRenderResult();
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType);

	std::function<void()> f_ShowRenderPassResult;
	std::function<ImTextureID(const FileExplorerIconType)> f_GetFileExplorerIconTextureID;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::setup()
{
#ifndef INNO_PLATFORM_MAC
	GLGuiSystemNS::f_ShowRenderPassResult = GLGuiSystemNS::showRenderResult;
	GLGuiSystemNS::f_GetFileExplorerIconTextureID = GLGuiSystemNS::getFileExplorerIconTextureID;

	ImGuiWrapper::get().setup();

	ImGuiWrapper::get().addShowRenderPassResultCallback(&GLGuiSystemNS::f_ShowRenderPassResult);
	ImGuiWrapper::get().addGetFileExplorerIconTextureIDCallback(&GLGuiSystemNS::f_GetFileExplorerIconTextureID);
#endif // !INNO_PLATFORM_MAC

	GLGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::initialize()
{
	ImGui_ImplGlfw_InitForOpenGL(GLWindowSystemComponent::get().m_window, true);
	ImGui_ImplOpenGL3_Init(NULL);

#ifndef INNO_PLATFORM_MAC
	ImGuiWrapper::get().initialize();
#endif // !INNO_PLATFORM_MAC

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::update()
{
#ifndef INNO_PLATFORM_MAC
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGuiWrapper::get().update();

	// Rendering
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif // !INNO_PLATFORM_MAC

	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::terminate()
{
	GLGuiSystemNS::m_objectStatus = ObjectStatus::STANDBY;

#ifndef INNO_PLATFORM_MAC
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGuiWrapper::get().terminate();
#endif // !INNO_PLATFORM_MAC

	GLGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus GLGuiSystem::getStatus()
{
	return GLGuiSystemNS::m_objectStatus;
}

void GLGuiSystemNS::showRenderResult()
{
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);

	ImGui::Begin("Early-Z Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_earlyZPass_GLRPC->m_GLTDCs[0]), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Position(RGB) + Metallic(A)");
			ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Normal(RGB) + Roughness(A)");
			ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[1]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
			ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[2]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Screen Space Motion Vector(RGB) + Transparency(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Screen Space Motion Vector(RGB) + Transparency(A)");
			ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();

	ImGui::Begin("SSAO Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_noiseGLTDC->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Transparent Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::BeginChild("Albedo (RGB) + transparency factor (A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Text("Albedo (RGB) + transparency factor (A)");
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();

		ImGui::BeginChild("Transmittance factor (RGB) + blend mask (A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Text("Transmittance factor (RGB) + blend mask (A)");
		ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[1]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::Begin("Terrain Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLTerrainRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLLightRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Final Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("Sky Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Sky Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Pre TAA Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Pre TAA Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("TAA Ping Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("TAA Ping Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("TAA Sharpen Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("TAA Sharpen Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Bloom Extract Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Bloom Extract Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Bloom Blur Ping Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Bloom Blur Ping Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Motion Blur Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Motion Blur Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Billboard Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Billboard Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Debugger Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Final Blend Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Final Blend Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();

	auto l_shadowPassWindowSize = ImVec2(512.0, 512.0);
	ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Depth Buffer");
	ImGui::Image(ImTextureID((GLuint64)GLShadowRenderPassComponent::get().m_DirLight_GLRPC->m_GLTDCs[0]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	ImGui::End();

	auto l_BRDFLUT = ImVec2(128.0, 128.0);
	ImGui::Begin("BRDF lookup table", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::BeginChild("IBL LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC->m_TAO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Multi-Scattering LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC->m_TAO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
	}
	ImGui::End();
}

ImTextureID GLGuiSystemNS::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_OBJ->m_TAO); break;
	case FileExplorerIconType::PNG:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_PNG->m_TAO); break;
	case FileExplorerIconType::SHADER:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_SHADER->m_TAO); break;
	case FileExplorerIconType::UNKNOWN:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_UNKNOWN->m_TAO); break;
	default:
		return nullptr; break;
	}
};