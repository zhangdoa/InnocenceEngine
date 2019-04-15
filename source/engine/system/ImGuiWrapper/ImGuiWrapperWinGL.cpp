#include "ImGuiWrapperWinGL.h"
#include "../../component/WinWindowSystemComponent.h"

#include "../GLRenderingBackend/GLEnvironmentRenderPass.h"
#include "../GLRenderingBackend/GLShadowRenderPass.h"

#include "../GLRenderingBackend/GLOpaquePass.h"
#include "../GLRenderingBackend/GLSSAONoisePass.h"
#include "../GLRenderingBackend/GLSSAOBlurPass.h"
#include "../GLRenderingBackend/GLTransparentPass.h"
#include "../GLRenderingBackend/GLTerrainPass.h"

#include "../GLRenderingBackend/GLLightPass.h"

#include "../GLRenderingBackend/GLSkyPass.h"
#include "../GLRenderingBackend/GLPreTAAPass.h"
#include "../GLRenderingBackend/GLTAAPass.h"
#include "../GLRenderingBackend/GLPostTAAPass.h"
#include "../GLRenderingBackend/GLMotionBlurPass.h"
#include "../GLRenderingBackend/GLBloomExtractPass.h"
#include "../GLRenderingBackend/GLBloomBlurPass.h"
#include "../GLRenderingBackend/GLBloomMergePass.h"
#include "../GLRenderingBackend/GLBillboardPass.h"
#include "../GLRenderingBackend/GLDebuggerPass.h"
#include "../GLRenderingBackend/GLFinalBlendPass.h"

#include "../../component/GLRenderingSystemComponent.h"

#include "../../third-party/ImGui/imgui_impl_win32.h"
#include "../../third-party/ImGui/imgui_impl_opengl3.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE ImGuiWrapperWinGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

bool ImGuiWrapperWinGL::setup()
{
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL setup finished.");

	return true;
}

bool ImGuiWrapperWinGL::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	ImGui_ImplOpenGL3_Init(NULL);
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL has been initialized.");

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
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperWinGL::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperWinGL::getStatus()
{
	return ImGuiWrapperWinGLNS::m_objectStatus;
}

void ImGuiWrapperWinGL::showRenderResult()
{
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);

	ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Position(RGB) + Metallic(A)");
			ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Normal(RGB) + Roughness(A)");
			ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[1]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
			ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[2]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Screen Space Motion Vector(RGB) + Transparency(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Screen Space Motion Vector(RGB) + Transparency(A)");
			ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[3]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();

	ImGui::Begin("SSAO Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLSSAONoisePass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::Image(ImTextureID((GLuint64)GLSSAOBlurPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Terrain Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLTerrainPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Image(ImTextureID((GLuint64)GLLightPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	}
	ImGui::End();

	ImGui::Begin("Final Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("Sky Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Sky Pass");
			ImGui::Image(ImTextureID((GLuint64)GLSkyPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Pre TAA Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Pre TAA Pass");
			ImGui::Image(ImTextureID((GLuint64)GLPreTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("TAA Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("TAA Pass");
			ImGui::Image(ImTextureID((GLuint64)GLTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Post TAA Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Post TAA Pass");
			ImGui::Image(ImTextureID((GLuint64)GLPostTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Bloom Extract Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Bloom Extract Pass");
			ImGui::Image(ImTextureID((GLuint64)GLBloomExtractPass::getGLRPC(0)->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Bloom Blur Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Bloom Blur Pass");
			ImGui::Image(ImTextureID((GLuint64)GLBloomBlurPass::getGLRPC(1)->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Motion Blur Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Motion Blur Pass");
			ImGui::Image(ImTextureID((GLuint64)GLMotionBlurPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Billboard Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Billboard Pass");
			ImGui::Image(ImTextureID((GLuint64)GLBillboardPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Debugger Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass");
			ImGui::Image(ImTextureID((GLuint64)GLDebuggerPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Final Blend Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Final Blend Pass");
			ImGui::Image(ImTextureID((GLuint64)GLFinalBlendPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();

	auto l_shadowPassWindowSize = ImVec2(512.0, 512.0);
	ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("CSM Depth Buffer");
	ImGui::Image(ImTextureID((GLuint64)GLShadowRenderPass::getGLRPC(0)->m_GLTDCs[0]->m_TO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	ImGui::End();

	auto l_BRDFLUT = ImVec2(128.0, 128.0);
	ImGui::Begin("BRDF lookup table", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::BeginChild("IBL LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPass::getBRDFSplitSumLUT()->m_TO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Multi-Scattering LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPass::getBRDFMSAverageLUT()->m_TO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::Begin("Voxelization Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Voxelization Pass");
	ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPass::getVoxelVisualizationPassGLTDC()->m_TO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	ImGui::End();
}

ImTextureID ImGuiWrapperWinGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_OBJ->m_TO); break;
	case FileExplorerIconType::PNG:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_PNG->m_TO); break;
	case FileExplorerIconType::SHADER:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_SHADER->m_TO); break;
	case FileExplorerIconType::UNKNOWN:
		return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_UNKNOWN->m_TO); break;
	default:
		return nullptr; break;
	}
};