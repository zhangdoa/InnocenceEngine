#include "ImGuiRendererDX12.h"

#include "../ImGui/imgui_impl_dx12.cpp"

#include "../../RenderingServer/DX12/DX12RenderingServer.h"
#include "../../RenderingServer/DX12/DX12Helper_Common.h"

#include "../../Interface/IRenderPass.h"

#include "../../Common/LogService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"

#include "../../Engine.h"
using namespace Inno;

namespace ImGuiRendererDX12NS
{
	class ImGuiRenderPass : public IRenderPass
	{
	public:
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;
		bool PrepareCommandList(IRenderingContext* renderingContext) override;

		RenderPassComponent* GetRenderPassComp();

	private:
		bool RenderTargetsReservationFunc();
		bool RenderTargetsCreationFunc();

		RenderPassComponent* m_RenderPassComp = nullptr;
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	};

	ImGuiRenderPass* m_RenderPass;
}

using namespace ImGuiRendererDX12NS;
bool ImGuiRenderPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("ImGuiRenderPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Graphics;
	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	l_RenderPassDesc.m_RenderTargetsReservationFunc = std::bind(&ImGuiRenderPass::RenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&ImGuiRenderPass::RenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_UseOutputMerger = true;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// No resource binding layout descriptors needed for ImGui.

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool ImGuiRenderPass::Initialize()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	// The actual rendering is called by the rendering server
	m_RenderPassComp->m_CustomCommandsFunc = [&](ICommandList* cmdList)
		{
			auto l_renderingServer = g_Engine->getRenderingServer();
			auto l_swapChainRenderPassComp = l_renderingServer->GetSwapChainRenderPassComponent();
			auto l_currentFrame =l_renderingServer->GetCurrentFrame();

			auto dx12CmdList = reinterpret_cast<DX12CommandList*>(cmdList);
			auto directCommandList = dx12CmdList->m_DirectCommandList.Get();

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), directCommandList);

			// Change the swap chain image state to read-only.
			l_renderingServer->TryToTransitState(l_swapChainRenderPassComp->m_OutputMergerTargets[l_currentFrame]->m_RenderTargets[0].m_Texture, dx12CmdList, Accessibility::ReadOnly);
		};

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool ImGuiRenderPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus ImGuiRenderPass::GetStatus()
{
	return m_ObjectStatus;
}

bool ImGuiRenderPass::PrepareCommandList(IRenderingContext* /*renderingContext*/)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* ImGuiRenderPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool ImGuiRenderPass::RenderTargetsReservationFunc()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());	
	m_RenderPassComp->m_OutputMergerTargets.resize(l_renderingServer->GetSwapChainImageCount());

	for (size_t i = 0; i < m_RenderPassComp->m_OutputMergerTargets.size(); i++)
	{
		if (m_RenderPassComp->m_OutputMergerTargets[i] == nullptr)
			l_renderingServer->Add(m_RenderPassComp->m_OutputMergerTargets[i]);

		auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTargets[i];
		l_outputMergerTarget->m_RenderTargets.resize(m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);
	}

	return true;
}

bool ImGuiRenderPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_swapChainRenderPassComp = reinterpret_cast<RenderPassComponent*>(l_renderingServer->GetSwapChainRenderPassComponent());

	for (size_t i = 0; i < m_RenderPassComp->m_OutputMergerTargets.size(); i++)
	{
		auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTargets[i];
		for (size_t j = 0; j < l_outputMergerTarget->m_RenderTargets.size(); j++)
			l_outputMergerTarget->m_RenderTargets[j].m_Texture = l_swapChainRenderPassComp->m_OutputMergerTargets[i]->m_RenderTargets[j].m_Texture;
	}

	return true;
}

bool ImGuiRendererDX12::Setup(ISystemConfig* systemConfig)
{
	m_RenderPass = new ImGuiRenderPass();
	m_RenderPass->Setup(nullptr);

	Log(Success, "ImGuiRendererDX12 Setup finished.");

	return true;
}

bool ImGuiRendererDX12::Initialize()
{
	auto l_renderingServer = reinterpret_cast<DX12RenderingServer*>(g_Engine->getRenderingServer());
	auto l_device = l_renderingServer->GetDevice().Get();
	auto& l_descHeapAccessor = l_renderingServer->GetDescriptorHeapAccessor(GPUResourceType::Image, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
	auto l_newHandle = l_descHeapAccessor.GetNewHandle();
	auto l_swapChainCount = l_renderingServer->GetSwapChainImageCount();

	ImGui_ImplDX12_Init(l_device, l_swapChainCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, l_descHeapAccessor.GetHeap().Get(),
		l_newHandle.CPUHandle, l_newHandle.GPUHandle);

	m_RenderPass->Initialize();

	Log(Success, "ImGuiRendererDX12 has been initialized.");

	return true;
}

bool ImGuiRendererDX12::NewFrame()
{
	ImGui_ImplDX12_NewFrame();

	return true;
}

bool ImGuiRendererDX12::Prepare()
{
	m_RenderPass->PrepareCommandList(nullptr);

	return true;
}

bool ImGuiRendererDX12::ExecuteCommands()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_swapChainRenderPassComp = l_renderingServer->GetSwapChainRenderPassComponent();

	// Let the swap chain rendering finish.
	l_renderingServer->WaitOnGPU(l_swapChainRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	l_renderingServer->ExecuteCommandList(m_RenderPass->GetRenderPassComp(), GPUEngineType::Graphics);
	l_renderingServer->SignalOnGPU(m_RenderPass->GetRenderPassComp(), GPUEngineType::Graphics);

	// Let the ImGui rendering finish.
	l_renderingServer->WaitOnGPU(m_RenderPass->GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
	return true;
}

bool ImGuiRendererDX12::Terminate()
{
	ImGui_ImplDX12_Shutdown();
	m_RenderPass->Terminate();

	delete m_RenderPass;

	Log(Success, "ImGuiRendererDX12 has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererDX12::GetStatus()
{
	return m_RenderPass->GetStatus();
}

void ImGuiRendererDX12::ShowRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}