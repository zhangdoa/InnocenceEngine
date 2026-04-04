#include "ImGuiRendererDX12.h"

#include "../ImGui/imgui_impl_dx12.cpp"

#include "../../Services/DX12/DX12GraphicsService.h"
#include "../../Services/DX12/DX12Helper_Common.h"

#include "../../Interface/IRenderPass.h"
#include "../../Services/GraphicsHardwareService.h"
#include "../../Services/GraphicsResourceService.h"
#include "../../Services/FrameManagementService.h"

#include "../../Common/LogService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Services/RenderingConfigurationService.h"

#include "../../Engine.h"
using namespace Inno;

namespace ImGuiRendererDX12NS
{
	class ImGuiRenderPass : public IRenderPass
	{
	public:
		bool Setup(IServiceConfig* systemConfig) override;
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
bool ImGuiRenderPass::Setup(IServiceConfig* systemConfig)
{
	auto l_graphicsService = reinterpret_cast<DX12GraphicsService*>(g_Engine->getGraphicsService());
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	m_RenderPassComp = l_rsService->AddRenderPassComponent("ImGuiRenderPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Graphics;
	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&ImGuiRenderPass::RenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&ImGuiRenderPass::RenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_UseOutputMerger = true;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// No resource binding layout descriptors needed for ImGui.
	
	m_CommandListComp_Graphics = l_rsService->AddCommandListComponent("ImGuiRenderPass/Graphics");
	
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool ImGuiRenderPass::Initialize()
{
	auto l_graphicsService = reinterpret_cast<DX12GraphicsService*>(g_Engine->getGraphicsService());
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	l_rsService->Initialize(m_RenderPassComp);

	// The actual rendering is called by the rendering server
	m_RenderPassComp->m_CustomCommandsFunc = [&](CommandListComponent* cmdList)
		{
			// Skip all rendering in offscreen mode
			if (g_Engine->getInitConfig().isOffscreen)
			{
				return;
			}

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
			auto l_swapChainRenderPassComp = l_fmService->GetSwapChainRenderPassComponent();
			auto l_currentFrame = l_fmService->GetCurrentFrame();

			auto dx12CmdList = cmdList;
			auto commandList = reinterpret_cast<ID3D12GraphicsCommandList*>(dx12CmdList->m_CommandList);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			// Change the swap chain image state to read-only (skip in offscreen mode).
			if (!g_Engine->getInitConfig().isOffscreen && l_swapChainRenderPassComp && 
				l_swapChainRenderPassComp->m_OutputMergerTarget && 
				!l_swapChainRenderPassComp->m_OutputMergerTarget->m_ColorOutputs.empty())
			{
				l_hwService->TryToTransitState(l_swapChainRenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0], dx12CmdList, Accessibility::WriteOnly, Accessibility::ReadOnly);
			}
		};

	l_rsService->Initialize(m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool ImGuiRenderPass::Terminate()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	l_rsService->Delete(m_RenderPassComp);
	l_rsService->Delete(m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus ImGuiRenderPass::GetStatus()
{
	return m_ObjectStatus;
}

bool ImGuiRenderPass::PrepareCommandList(IRenderingContext* /*renderingContext*/)
{
	// Skip command list preparation in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		return true;
	}

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_hwService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	return true;
}

RenderPassComponent* ImGuiRenderPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool ImGuiRenderPass::RenderTargetsReservationFunc()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	if (m_RenderPassComp->m_OutputMergerTarget == nullptr)
		l_rsService->Add(m_RenderPassComp->m_OutputMergerTarget);

	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_ColorOutputs.resize(m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);

	return true;
}

bool ImGuiRenderPass::RenderTargetsCreationFunc()
{
	auto l_graphicsService = reinterpret_cast<DX12GraphicsService*>(g_Engine->getGraphicsService());
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_swapChainRenderPassComp = reinterpret_cast<RenderPassComponent*>(l_fmService->GetSwapChainRenderPassComponent());
	
	// Skip render target creation in offscreen mode or if swap chain is not available
	if (g_Engine->getInitConfig().isOffscreen || !l_swapChainRenderPassComp || 
		!l_swapChainRenderPassComp->m_OutputMergerTarget)
	{
		Log(Verbose, "ImGuiRenderPass: Skipping render target creation in offscreen mode or invalid swap chain");
		return true;
	}
	
	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
		l_outputMergerTarget->m_ColorOutputs[j] = l_swapChainRenderPassComp->m_OutputMergerTarget->m_ColorOutputs[j];

	return true;
}

bool ImGuiRendererDX12::Setup(IServiceConfig* systemConfig)
{
	m_RenderPass = new ImGuiRenderPass();
	m_RenderPass->Setup(nullptr);

	Log(Success, "ImGuiRendererDX12 Setup finished.");

	return true;
}

bool ImGuiRendererDX12::Initialize()
{
	// Skip initialization in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		Log(Verbose, "ImGuiRendererDX12: Skipping initialization in offscreen mode");
		return true;
	}

	auto l_graphicsService = reinterpret_cast<DX12GraphicsService*>(g_Engine->getGraphicsService());
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_device = l_graphicsService->GetDevice().Get();
	auto& l_descHeapAccessor = l_graphicsService->GetDescriptorHeapAccessor(GPUResourceType::Image, Accessibility::ReadOnly, Accessibility::ReadWrite, TextureUsage::ColorAttachment);
	auto l_newHandle = l_descHeapAccessor.GetNewHandle();
	auto l_swapChainCount = l_fmService->GetSwapChainImageCount();

	ImGui_ImplDX12_Init(l_device, l_swapChainCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, l_descHeapAccessor.GetHeap().Get(),
	 	D3D12_CPU_DESCRIPTOR_HANDLE{ l_newHandle.m_CPUHandle }, D3D12_GPU_DESCRIPTOR_HANDLE{ l_newHandle.m_GPUHandle });

	m_RenderPass->Initialize();

	Log(Success, "ImGuiRendererDX12 has been initialized.");

	return true;
}

bool ImGuiRendererDX12::NewFrame()
{
	// Skip frame processing in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		return true;
	}

	ImGui_ImplDX12_NewFrame();

	return true;
}

bool ImGuiRendererDX12::Prepare()
{
	// Skip preparation in offscreen mode
	if (g_Engine->getInitConfig().isOffscreen)
	{
		return true;
	}

	m_RenderPass->PrepareCommandList(nullptr);

	return true;
}

bool ImGuiRendererDX12::ExecuteCommands()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_swapChainRenderPassComp = l_fmService->GetSwapChainRenderPassComponent();

	// Skip ImGui execution in offscreen mode or if swap chain is not available
	if (g_Engine->getInitConfig().isOffscreen || !l_swapChainRenderPassComp)
	{
		Log(Verbose, "ImGuiRendererDX12: Skipping command execution in offscreen mode or invalid swap chain");
		return true;
	}

	// Let the swap chain rendering finish.
	l_hwService->WaitOnGPU(l_swapChainRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	if (m_RenderPass->PrepareCommandList(nullptr))
	{
		auto l_commandList = m_RenderPass->GetCommandListComp(GPUEngineType::Graphics);
		if (l_commandList) {
			l_hwService->Execute(l_commandList, GPUEngineType::Graphics);
		}
	}
	l_hwService->SignalOnGPU(m_RenderPass->GetRenderPassComp(), GPUEngineType::Graphics);

	// Let the ImGui rendering finish.
	l_hwService->WaitOnGPU(m_RenderPass->GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
	return true;
}

bool ImGuiRendererDX12::Terminate()
{
	// Only shutdown ImGui if it was initialized (not in offscreen mode)
	if (!g_Engine->getInitConfig().isOffscreen)
	{
		ImGui_ImplDX12_Shutdown();
	}
	
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