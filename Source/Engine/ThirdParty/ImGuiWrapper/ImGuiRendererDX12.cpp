#include "ImGuiRendererDX12.h"

#include "../ImGui/imgui_impl_dx12.cpp"

#include "../../RenderingServer/DX12/DX12RenderingServer.h"
#include "../../RenderingServer/DX12/DX12Helper.h"

#include "../../Component/DX12TextureComponent.h"

#include "../../Common/LogService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"

#include "../../Engine.h"
using namespace Inno;
;

namespace ImGuiRendererDX12NS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;
}

bool ImGuiRendererDX12::Setup(ISystemConfig* systemConfig)
{
	ImGuiRendererDX12NS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "ImGuiRendererDX12 Setup finished.");

	return true;
}

bool ImGuiRendererDX12::Initialize()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_device = reinterpret_cast<ID3D12Device*>(l_renderingServer->GetDevice());
	auto l_CSUDescHeap = reinterpret_cast<ID3D12DescriptorHeap*>(l_renderingServer->GetCSUDescHeap());
	auto l_commandAllocator = reinterpret_cast<ID3D12CommandAllocator*>(l_renderingServer->GetCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 0));

	ImGui_ImplDX12_Init(l_device, l_renderingServer->GetSwapChainImageCount(),
        DXGI_FORMAT_R8G8B8A8_UNORM, l_CSUDescHeap,
        l_CSUDescHeap->GetCPUDescriptorHandleForHeapStart(),
        l_CSUDescHeap->GetGPUDescriptorHandleForHeapStart());

	ImGuiRendererDX12NS::m_CommandList = DX12Helper::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, l_device, l_commandAllocator, L"ImGui_DirectCommandList");
	ImGuiRendererDX12NS::m_CommandList->Close();
	Log(Success, "ImGuiRendererDX12 has been initialized.");

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
	
	// @TODO: Implement as a render pass
    //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ImGuiRendererDX12NS::m_CommandList.Get());
	
	return true;
}

bool ImGuiRendererDX12::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	ImGuiRendererDX12NS::m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "ImGuiRendererDX12 has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererDX12::GetStatus()
{
	return ImGuiRendererDX12NS::m_ObjectStatus;
}

void ImGuiRendererDX12::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}