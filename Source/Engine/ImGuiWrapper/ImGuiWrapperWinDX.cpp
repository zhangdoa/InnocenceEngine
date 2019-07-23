#include "ImGuiWrapperWinDX.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../RenderingBackend/DX11RenderingBackend/DX11OpaquePass.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11LightPass.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11FinalBlendPass.h"

#include "../RenderingBackend/DX11RenderingBackend/DX11RenderingBackendUtilities.h"

#include "../ThirdParty/ImGui/imgui_impl_win32.h"
#include "../ThirdParty/ImGui/imgui_impl_dx11.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWinDXNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperWinDX11::setup()
{
	ImGuiWrapperWinDXNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinDX setup finished.");

	return true;
}

bool ImGuiWrapperWinDX11::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	ImGui_ImplDX11_Init(DX11RenderingBackendComponent::get().m_device, DX11RenderingBackendComponent::get().m_deviceContext);
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinDX has been initialized.");

	return true;
}

bool ImGuiWrapperWinDX11::newFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	return true;
}

bool ImGuiWrapperWinDX11::render()
{
	auto l_DXRPC = DX11FinalBlendPass::getDX11RPC();
	DX11RenderingBackendComponent::get().m_deviceContext->OMSetRenderTargets((unsigned int)l_DXRPC->m_RTVs.size(), &l_DXRPC->m_RTVs[0], l_DXRPC->m_DSV);
	DX11RenderingBackendComponent::get().m_deviceContext->RSSetViewports(1, &l_DXRPC->m_viewport);
	DX11RenderingBackendComponent::get().m_deviceContext->RSSetState(l_DXRPC->m_rasterizerState);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperWinDX11::terminate()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGuiWrapperWinDXNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperWinDX has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperWinDX11::getStatus()
{
	return ImGuiWrapperWinDXNS::m_objectStatus;
}

void ImGuiWrapperWinDX11::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);

	ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Position(RGB) + Metallic(A)");
			ImGui::Image(ImTextureID(DX11OpaquePass::getDX11RPC()->m_DXTDCs[0]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Normal(RGB) + Roughness(A)");
			ImGui::Image(ImTextureID(DX11OpaquePass::getDX11RPC()->m_DXTDCs[1]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
			ImGui::Image(ImTextureID(DX11OpaquePass::getDX11RPC()->m_DXTDCs[2]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Screen Space Motion Vector(RGB) + Transparency(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Screen Space Motion Vector(RGB) + Transparency(A)");
			ImGui::Image(ImTextureID(DX11OpaquePass::getDX11RPC()->m_DXTDCs[3]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

ImTextureID ImGuiWrapperWinDX11::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID(DX11RenderingBackendNS::getDX11TextureDataComponent(iconType)->m_SRV);
}