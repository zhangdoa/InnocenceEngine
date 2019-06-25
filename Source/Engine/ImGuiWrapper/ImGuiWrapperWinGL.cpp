#include "ImGuiWrapperWinGL.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../RenderingBackend/GLRenderingBackend/GLShadowPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLBRDFLUTPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLVXGIPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLOpaquePass.h"
#include "../RenderingBackend/GLRenderingBackend/GLSSAONoisePass.h"
#include "../RenderingBackend/GLRenderingBackend/GLSSAOBlurPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLLightCullingPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLLightPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLTerrainPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLSkyPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLPreTAAPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLTAAPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLPostTAAPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLMotionBlurPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLBloomExtractPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLGaussianBlurPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLBloomMergePass.h"
#include "../RenderingBackend/GLRenderingBackend/GLBillboardPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLDebuggerPass.h"
#include "../RenderingBackend/GLRenderingBackend/GLFinalBlendPass.h"

#include "../RenderingBackend/GLRenderingBackend/GLRenderingBackendUtilities.h"

#include "../ThirdParty/ImGui/imgui_impl_win32.h"
#include "../ThirdParty/ImGui/imgui_impl_opengl3.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE ImGuiWrapperWinGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperWinGL::setup()
{
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL setup finished.");

	return true;
}

bool ImGuiWrapperWinGL::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	ImGui_ImplOpenGL3_Init(NULL);
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL has been initialized.");

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
	glViewport(0, 0, (GLsizei)l_screenResolution.x, (GLsizei)l_screenResolution.y);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperWinGL::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGuiWrapperWinGLNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinGL has been terminated.");

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

	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("CSM Depth Buffer");
		ImGui::Image(ImTextureID((GLuint64)GLShadowPass::getGLRPC(0)->m_GLTDCs[0]->m_TO), l_shadowRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::End();
		break;
	case RenderPassType::GI:
		ImGui::Begin("BRDF lookup table", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginChild("IBL BRDF LUT", l_BRDFLUTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GLBRDFLUTPass::getBRDFSplitSumLUT()->m_TO), l_BRDFLUTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Multi-Scattering LUT", l_BRDFLUTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GLBRDFLUTPass::getBRDFMSAverageLUT()->m_TO), l_BRDFLUTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		ImGui::End();

		ImGui::Begin("Voxelization Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Voxelization Pass");
		ImGui::Image(ImTextureID((GLuint64)GLVXGIPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::End();
		break;
	case RenderPassType::Opaque:
		ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			{
				ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("World Space Position(RGB) + Metallic(A)");
				ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("World Space Normal(RGB) + Roughness(A)");
				ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[1]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
				ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[2]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Screen Space Motion Vector(RG) + UUID(B) + Transparency(A)", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Screen Space Motion Vector(RG) + UUID(B) + Transparency(A)");
				ImGui::Image(ImTextureID((GLuint64)GLOpaquePass::getGLRPC()->m_GLTDCs[3]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
		}
		ImGui::End();

		ImGui::Begin("SSAO Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginChild("SSAO Noise", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("SSAO Noise");
			ImGui::Image(ImTextureID((GLuint64)GLSSAONoisePass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("SSAO Blur", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("SSAO Blur");
			ImGui::Image(ImTextureID((GLuint64)GLSSAOBlurPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		ImGui::End();
		break;
	case RenderPassType::Light:
		ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginChild("Light Culling Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Light Culling Pass");
			ImGui::Image(ImTextureID((GLuint64)GLLightCullingPass::getHeatMap()->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Light Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Light Pass");
			ImGui::Image(ImTextureID((GLuint64)GLLightPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		ImGui::End();
		break;
	case RenderPassType::Transparent:
		break;
	case RenderPassType::Terrain:
		ImGui::Begin("Height Map", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginChild("Height Map", l_shadowRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Height Map");
			ImGui::Image(ImTextureID((GLuint64)GLTerrainPass::getHeightMap(0)->m_TO), l_shadowRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Height Map Normal", l_shadowRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Height Map Normal");
			ImGui::Image(ImTextureID((GLuint64)GLTerrainPass::getHeightMap(1)->m_TO), l_shadowRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		ImGui::End();
		break;
	case RenderPassType::PostProcessing:
		ImGui::Begin("Post-processing Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			{
				ImGui::BeginChild("Sky Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Sky Pass");
				ImGui::Image(ImTextureID((GLuint64)GLSkyPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Pre TAA Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Pre TAA Pass");
				ImGui::Image(ImTextureID((GLuint64)GLPreTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("TAA Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("TAA Pass");
				ImGui::Image(ImTextureID((GLuint64)GLTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Post TAA Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Post TAA Pass");
				ImGui::Image(ImTextureID((GLuint64)GLPostTAAPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Bloom Extract Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Bloom Extract Pass");
				ImGui::Image(ImTextureID((GLuint64)GLBloomExtractPass::getGLRPC(0)->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Bloom Blur Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Bloom Blur Pass");
				ImGui::Image(ImTextureID((GLuint64)GLGaussianBlurPass::getGLRPC(1)->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Motion Blur Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Motion Blur Pass");
				ImGui::Image(ImTextureID((GLuint64)GLMotionBlurPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Final Blend Pass", l_RTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Final Blend Pass");
				ImGui::Image(ImTextureID((GLuint64)GLFinalBlendPass::getGLRPC()->m_GLTDCs[0]->m_TO), l_RTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
		}
		ImGui::End();
		break;
	case RenderPassType::Development:
		ImGui::Begin("Development Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginChild("Debugger Pass", l_developmentRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass");
			ImGui::Image(ImTextureID((GLuint64)GLDebuggerPass::getGLRPC(0)->m_GLTDCs[0]->m_TO), l_developmentRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Debugger Pass Right View", l_developmentRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass Right View");
			ImGui::Image(ImTextureID((GLuint64)GLDebuggerPass::getGLRPC(1)->m_GLTDCs[0]->m_TO), l_developmentRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::BeginChild("Debugger Pass Top View", l_developmentRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass Top View");
			ImGui::Image(ImTextureID((GLuint64)GLDebuggerPass::getGLRPC(2)->m_GLTDCs[0]->m_TO), l_developmentRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Debugger Pass Front View", l_developmentRTSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Debugger Pass Front View");
			ImGui::Image(ImTextureID((GLuint64)GLDebuggerPass::getGLRPC(3)->m_GLTDCs[0]->m_TO), l_developmentRTSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::EndChild();
		}
		ImGui::End();
		break;
	default:
		break;
	}
}

ImTextureID ImGuiWrapperWinGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID((GLuint64)GLRenderingBackendNS::getGLTextureDataComponent(iconType)->m_TO);
};