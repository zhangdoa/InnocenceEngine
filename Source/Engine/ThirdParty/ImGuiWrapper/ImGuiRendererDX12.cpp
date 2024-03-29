#include "ImGuiRendererDX12.h"

#include "../ImGui/imgui_impl_dx12.cpp"

#include "../../RenderingServer/DX12/DX12RenderingServer.h"
#include "../../Component/DX12TextureComponent.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiRendererDX12NS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererDX12::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererDX12NS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX12 Setup finished.");

	return true;
}

bool ImGuiRendererDX12::Initialize()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_device = reinterpret_cast<ID3D12Device*>(l_renderingServer->GetDevice());
	auto l_CSUDescHeap = reinterpret_cast<ID3D12DescriptorHeap*>(l_renderingServer->GetCSUDescHeap());

	ImGui_ImplDX12_Init(l_device, l_renderingServer->GetSwapChainImageCount(),
        DXGI_FORMAT_R8G8B8A8_UNORM, l_CSUDescHeap,
        l_CSUDescHeap->GetCPUDescriptorHandleForHeapStart(),
        l_CSUDescHeap->GetGPUDescriptorHandleForHeapStart());
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX12 has been initialized.");

	return true;
}

bool ImGuiRendererDX12::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	return true;
}

bool ImGuiRendererDX12::Render()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_userPipelineOutput = reinterpret_cast<DX12TextureComponent*>(l_renderingServer->GetUserPipelineOutput());
	
	// @TODO: Finish it
    //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), l_commandList->m_DirectCommandList.Get());
	
	return true;
}

bool ImGuiRendererDX12::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	ImGuiRendererDX12NS::m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererDX12 has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererDX12::GetStatus()
{
	return ImGuiRendererDX12NS::m_ObjectStatus;
}

void ImGuiRendererDX12::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_Engine->getRenderingFrontend()->GetScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}