#include "DX12RenderingServer.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../../Services/RenderingConfigurationService.h"

using namespace Inno;
#include "../../Engine.h"

#include "DX12Helper_Common.h"

using namespace DX12Helper;

bool DX12RenderingServer::CreateHardwareResources()
{
    bool l_result = true;

#ifdef INNO_DEBUG
    l_result &= CreateDebugCallback();
#endif
    l_result &= CreatePhysicalDevices();
    l_result &= CreateGlobalCommandQueues();
    l_result &= CreateGlobalCommandAllocators();
    l_result &= CreateSyncPrimitives();
    l_result &= CreateGlobalDescriptorHeaps();
    l_result &= CreateMipmapGenerator();
    l_result &= CreateSwapChain();

    return l_result;
}

bool DX12RenderingServer::ReleaseHardwareResources()
{
    // @TODO: Release accessors first

    m_SamplerDescHeap.ReleaseAndGetAddressOf();
    m_DSVDescHeap.ReleaseAndGetAddressOf();
    m_RTVDescHeap.ReleaseAndGetAddressOf();
    m_CSUDescHeap_ShaderNonVisible.ReleaseAndGetAddressOf();
    m_CSUDescHeap.ReleaseAndGetAddressOf();

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandQueueFence[i].ReleaseAndGetAddressOf();
        m_computeCommandQueueFence[i].ReleaseAndGetAddressOf();
        m_copyCommandQueueFence[i].ReleaseAndGetAddressOf();
    }

    for (size_t i = 0; i < m_GlobalCommandLists.size(); i++)
    {
        m_GlobalCommandLists[i]->m_DirectCommandList.ReleaseAndGetAddressOf();
        m_GlobalCommandLists[i]->m_ComputeCommandList.ReleaseAndGetAddressOf();
        m_GlobalCommandLists[i]->m_CopyCommandList.ReleaseAndGetAddressOf();
    }

    m_copyCommandAllocator.ReleaseAndGetAddressOf();

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandAllocators[i].ReleaseAndGetAddressOf();
        m_computeCommandAllocators[i].ReleaseAndGetAddressOf();
    }

    m_directCommandQueue.ReleaseAndGetAddressOf();
    m_computeCommandQueue.ReleaseAndGetAddressOf();
    m_copyCommandQueue.ReleaseAndGetAddressOf();

    m_swapChain.ReleaseAndGetAddressOf();
    m_device.ReleaseAndGetAddressOf();
    m_adapterOutput.ReleaseAndGetAddressOf();
    m_adapter.ReleaseAndGetAddressOf();
    m_factory.ReleaseAndGetAddressOf();
    m_graphicsAnalysis.ReleaseAndGetAddressOf();
    m_debugInterface.ReleaseAndGetAddressOf();

#ifdef INNO_DEBUG
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        pDebug->Release();
    }
#endif

    return true;
}

bool DX12RenderingServer::GetSwapChainImages()
{
    m_swapChainImages.resize(m_swapChainImageCount);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        auto l_HResult = m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&m_swapChainImages[i]));
        if (FAILED(l_HResult))
        {
            Log(Error, "Can't get pointer of swap chain image ", i, "!");
            return false;
        }
        m_swapChainImages[i]->SetName((L"SwapChainBackBuffer_" + std::to_wstring(i)).c_str());
    }

    return true;
}

bool DX12RenderingServer::AssignSwapChainImages()
{
    m_SwapChainRenderPassComp->m_RenderTargets.resize(m_swapChainImageCount);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        auto l_DX12TextureComp = reinterpret_cast<DX12TextureComponent*>(m_SwapChainRenderPassComp->m_RenderTargets[i].m_Texture);

        l_DX12TextureComp->m_DefaultHeapBuffer = m_swapChainImages[i];
        l_DX12TextureComp->m_DX12TextureDesc = l_DX12TextureComp->m_DefaultHeapBuffer->GetDesc();
        l_DX12TextureComp->m_WriteState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        l_DX12TextureComp->m_ReadState = D3D12_RESOURCE_STATE_PRESENT;
        l_DX12TextureComp->m_CurrentState = l_DX12TextureComp->m_ReadState;

        l_DX12TextureComp->m_ObjectStatus = ObjectStatus::Activated;
    }

    return true;
}

bool DX12RenderingServer::PresentImpl()
{
    m_swapChain->Present(0, 0);

    return true;
}

bool DX12RenderingServer::PostPresent()
{
	m_SwapChainRenderPassComp->m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

	// GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)->Reset();
	// GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE)->Reset();
	// GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY)->Reset();

    return true;
}

bool DX12RenderingServer::ResizeImpl()
{
    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainImages.clear();
	
	WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Graphics);
	WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Compute);
    for (int i = 0; i < m_swapChainImageCount; i++)
    {
        UINT64 l_directCommandFinishedSemaphore = ++m_directCommandQueueSemaphore[i];
        GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->Signal(m_directCommandQueueFence[i].Get(), l_directCommandFinishedSemaphore);
        if (m_directCommandQueueFence[i]->GetCompletedValue() < l_directCommandFinishedSemaphore)
        {
            m_directCommandQueueFence[i]->SetEventOnCompletion(l_directCommandFinishedSemaphore, m_directCommandQueueFenceEvent[i]);
            WaitForSingleObject(m_directCommandQueueFenceEvent[i], INFINITE);
        }
    }

    m_swapChain->ResizeBuffers(m_swapChainImageCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, 0);

    if (!GetSwapChainImages())
        return false;

    return true;
}

bool DX12RenderingServer::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* rhs)
{
	if (!rhs->m_RenderPassDesc.m_Resizable)
		return true;

	rhs->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	rhs->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	ReserveRenderTargets(rhs);
	CreateRenderTargets(rhs);

	// @TODO: Move this to the base class.
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	if (rhs->m_RenderPassDesc.m_UseOutputMerger)
	{
		for (size_t i = 0; i < rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_RenderTargets[i].m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &l_rhs->m_RTVDesc, l_rhs->m_RTVDescCPUHandles[i]);
		}
	}

	if (rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		if (rhs->m_DepthStencilRenderTarget.m_Texture != nullptr)
		{
			auto l_ResourceHandle = reinterpret_cast<DX12TextureComponent*>(rhs->m_DepthStencilRenderTarget.m_Texture)->m_DefaultHeapBuffer;
			m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &l_rhs->m_DSVDesc, l_rhs->m_DSVDescCPUHandle);
		}
	}

	rhs->m_PipelineStateObject = AddPipelineStateObject();

	CreatePSO(l_rhs);

	if (rhs->m_OnResize)
		rhs->m_OnResize();

	return true;
}

bool DX12RenderingServer::BeginCapture()
{
	if (m_graphicsAnalysis != nullptr)
	{
		m_graphicsAnalysis->BeginCapture();
		return true;
	}

	return false;
}

bool DX12RenderingServer::EndCapture()
{
	if (m_graphicsAnalysis != nullptr)
	{
		m_graphicsAnalysis->EndCapture();
		return true;
	}

	return false;
}