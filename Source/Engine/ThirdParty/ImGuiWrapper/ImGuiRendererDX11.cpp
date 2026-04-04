#include "ImGuiRendererDX11.h"

#include "../ImGui/imgui_impl_dx11.cpp"

#include "../../Services/DX11/DX11GraphicsService.h"
#include "../../Component/DX11RenderPassComponent.h"

#include "../../Services/FrameManagementService.h"
#include "../../Engine.h"
using namespace Inno;
;

namespace ImGuiRendererDX11NS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererDX11::Setup(IServiceConfig* systemConfig)
{
	ImGuiRendererDX11NS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "ImGuiRendererDX11 Setup finished.");

	return true;
}

bool ImGuiRendererDX11::Initialize()
{
	auto l_graphicsService = reinterpret_cast<DX11GraphicsService*>(g_Engine->getGraphicsService());
	auto l_device = reinterpret_cast<ID3D11Device*>(l_graphicsService->GetDevice());
	auto l_deviceContext = reinterpret_cast<ID3D11DeviceContext*>(l_graphicsService->GetDeviceContext());

	ImGui_ImplDX11_Init(l_device, l_deviceContext);
	Log(Success, "ImGuiRendererDX11 has been initialized.");

	return true;
}

bool ImGuiRendererDX11::NewFrame()
{
	ImGui_ImplDX11_NewFrame();
	return true;
}

bool ImGuiRendererDX11::Prepare()
{
	auto l_userPipelineOutputRenderPassComp = reinterpret_cast<DX11RenderPassComponent*>(g_Engine->Get<FrameManagementService>()->GetUserPipelineOutput());
	auto l_graphicsService = reinterpret_cast<DX11GraphicsService*>(g_Engine->getGraphicsService());
	auto l_deviceContext = reinterpret_cast<ID3D11DeviceContext*>(l_graphicsService->GetDeviceContext());

	l_deviceContext->OMSetRenderTargets(1, &l_userPipelineOutputRenderPassComp->m_RTVs[l_userPipelineOutputRenderPassComp->m_CurrentFrame], NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiRendererDX11::Terminate()
{
	ImGui_ImplDX11_Shutdown();
	ImGuiRendererDX11NS::m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "ImGuiRendererDX11 has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererDX11::GetStatus()
{
	return ImGuiRendererDX11NS::m_ObjectStatus;
}

void ImGuiRendererDX11::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}