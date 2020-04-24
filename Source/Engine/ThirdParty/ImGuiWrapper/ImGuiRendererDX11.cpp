#include "ImGuiRendererDX11.h"
#include "../../Component/WinWindowSystemComponent.h"

#include "../ImGui/imgui_impl_dx11.cpp"

#include "../../RenderingServer/DX11/DX11RenderingServer.h"
#include "../../Component/DX11RenderPassDataComponent.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiRendererDX11NS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererDX11::setup()
{
	ImGuiRendererDX11NS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX11 setup finished.");

	return true;
}

bool ImGuiRendererDX11::initialize()
{
	auto l_renderingServer = reinterpret_cast<DX11RenderingServer*>(g_pModuleManager->getRenderingServer());
	auto l_device = reinterpret_cast<ID3D11Device*>(l_renderingServer->GetDevice());
	auto l_deviceContext = reinterpret_cast<ID3D11DeviceContext*>(l_renderingServer->GetDeviceContext());

	ImGui_ImplDX11_Init(l_device, l_deviceContext);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX11 has been initialized.");

	return true;
}

bool ImGuiRendererDX11::newFrame()
{
	ImGui_ImplDX11_NewFrame();
	return true;
}

bool ImGuiRendererDX11::render()
{
	auto l_userPipelineOutputRPDC = reinterpret_cast<DX11RenderPassDataComponent*>(g_pModuleManager->getRenderingServer()->GetUserPipelineOutput());
	auto l_renderingServer = reinterpret_cast<DX11RenderingServer*>(g_pModuleManager->getRenderingServer());
	auto l_deviceContext = reinterpret_cast<ID3D11DeviceContext*>(l_renderingServer->GetDeviceContext());

	l_deviceContext->OMSetRenderTargets(1, &l_userPipelineOutputRPDC->m_RTVs[l_userPipelineOutputRPDC->m_CurrentFrame], NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiRendererDX11::terminate()
{
	ImGui_ImplDX11_Shutdown();
	ImGuiRendererDX11NS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX11 has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererDX11::getStatus()
{
	return ImGuiRendererDX11NS::m_ObjectStatus;
}

void ImGuiRendererDX11::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}