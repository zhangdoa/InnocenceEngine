#include "DX12RenderingServer.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../../Services/RenderingConfigurationService.h"

using namespace Inno;
#include "../../Engine.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"

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

    m_directCommandQueueFence.ReleaseAndGetAddressOf();
    m_computeCommandQueueFence.ReleaseAndGetAddressOf();
    m_copyCommandQueueFence.ReleaseAndGetAddressOf();

    for (size_t i = 0; i < m_GlobalCommandLists.size(); i++)
    {
        auto l_commandList = reinterpret_cast<DX12CommandList*>(m_GlobalCommandLists[i]);
        l_commandList->m_DirectCommandList.ReleaseAndGetAddressOf();
        l_commandList->m_ComputeCommandList.ReleaseAndGetAddressOf();
        l_commandList->m_CopyCommandList.ReleaseAndGetAddressOf();
    }

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandAllocators[i].ReleaseAndGetAddressOf();
        m_computeCommandAllocators[i].ReleaseAndGetAddressOf();
        m_copyCommandAllocators[i].ReleaseAndGetAddressOf();
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
    auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
    auto l_DX12TextureComp = reinterpret_cast<DX12TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);
    l_DX12TextureComp->m_DeviceMemories.resize(GetSwapChainImageCount());
    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        auto l_DX12DeviceMemory = new DX12DeviceMemory();
        l_DX12DeviceMemory->m_DefaultHeapBuffer = m_swapChainImages[i];
        l_DX12TextureComp->m_DeviceMemories[i] = l_DX12DeviceMemory;   
    }
    
    auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_DX12TextureComp->m_DeviceMemories[0]);
    l_DX12TextureComp->m_DX12TextureDesc = l_DX12DeviceMemory->m_DefaultHeapBuffer->GetDesc();
    l_DX12TextureComp->m_WriteState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    l_DX12TextureComp->m_ReadState = D3D12_RESOURCE_STATE_PRESENT;
    l_DX12TextureComp->m_CurrentState = l_DX12TextureComp->m_ReadState;
    l_DX12TextureComp->m_ObjectStatus = ObjectStatus::Activated;

    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();
    m_PreviousFrame = GetPreviousFrame();

    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12RenderingServer::BeginFrame()
{
    GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)->Reset();
    GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE)->Reset();
    GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY)->Reset();
       
    return true;
}
bool DX12RenderingServer::PresentImpl()
{
    m_swapChain->Present(0, 0);

    return true;
}

bool DX12RenderingServer::UpdateFrameIndex()
{
    m_PreviousFrame = m_CurrentFrame;
    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();
    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12RenderingServer::ResizeImpl()
{
    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainImages.clear();

    m_swapChain->ResizeBuffers(m_swapChainImageCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, 0);

    m_SwapChainRenderPassComp->m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

    if (!GetSwapChainImages())
        return false;

    return true;
}

bool DX12RenderingServer::OnOutputMergerTargetsCreated(RenderPassComponent* rhs)
{
    auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(rhs->m_OutputMergerTarget);
    if (rhs->m_RenderPassDesc.m_UseOutputMerger)
    {
        auto& l_RTVs = l_outputMergerTarget->m_RTVs;
        if (l_RTVs.size() == 0)
        {
            l_RTVs.resize(GetSwapChainImageCount());
            for (size_t i = 0; i < l_RTVs.size(); i++)
            {
                auto& l_RTV = l_RTVs[i];
                l_RTV.m_Desc = GetRTVDesc(rhs->m_RenderPassDesc.m_RenderTargetDesc);
                l_RTV.m_Handles.resize(rhs->m_RenderPassDesc.m_RenderTargetCount);
                for (size_t j = 0; j < l_RTV.m_Handles.size(); j++)
                {
                    auto l_handle = m_RTVDescHeapAccessor.GetNewHandle();
                    l_RTV.m_Handles[j] = l_handle.CPUHandle;
                }
            }
        }

        for (size_t i = 0; i < l_RTVs.size(); i++)
        {
            auto& l_RTV = l_RTVs[i];
            for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
            {
                auto l_renderTargetTexture = reinterpret_cast<DX12TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[j]);
                auto l_DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_renderTargetTexture->m_DeviceMemories[i]);

                auto l_ResourceHandle = l_DeviceMemory->m_DefaultHeapBuffer;
                m_device->CreateRenderTargetView(l_ResourceHandle.Get(), &l_RTV.m_Desc, l_RTV.m_Handles[j]);
            }

        }
    }

    if (rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
    {
        auto& l_DSVs = l_outputMergerTarget->m_DSVs;
        if (l_DSVs.size() == 0)
        {
            l_DSVs.resize(GetSwapChainImageCount());
            for (size_t i = 0; i < l_DSVs.size(); i++)
            {
                l_DSVs[i].m_Desc = GetDSVDesc(rhs->m_RenderPassDesc.m_RenderTargetDesc, rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
                l_DSVs[i].m_Handle = m_DSVDescHeapAccessor.GetNewHandle().CPUHandle;
            }
        }

        auto l_renderTargetTexture = reinterpret_cast<DX12TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
        for (size_t i = 0; i < l_DSVs.size(); i++)
        {
            auto l_DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_renderTargetTexture->m_DeviceMemories[i]);
            auto l_ResourceHandle = l_DeviceMemory->m_DefaultHeapBuffer;

            m_device->CreateDepthStencilView(l_ResourceHandle.Get(), &l_DSVs[i].m_Desc, l_DSVs[i].m_Handle);
        }
    }

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