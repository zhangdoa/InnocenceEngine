#include "DX12RenderingServer.h"

#include "../../Common/LogServiceSpecialization.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../../Services/RenderingConfigurationService.h"

using namespace Inno;
#include "../../Engine.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Texture.h"
#include "DX12Helper_Pipeline.h"

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

bool DX12RenderingServer::ReleaseSwapChainImages()
{  
    return true;
}

bool DX12RenderingServer::CreatePipelineStateObject(RenderPassComponent* rhs)
{	
	bool l_result = true;
	auto RenderPassComp = reinterpret_cast<RenderPassComponent*>(rhs);
	l_result &= CreateRootSignature(RenderPassComp);

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	if (RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		GenerateDepthStencilStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
		GenerateBlendStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, l_PSO);
		GenerateRasterizerStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
		GenerateViewportStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);

		if (RenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
		{
			l_PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

			auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
			auto l_RTV = l_DX12OutputMergerTarget->m_RTVs[0];
			for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				l_PSO->m_GraphicsPSODesc.RTVFormats[i] = l_RTV.m_Desc.Format;
			}
		}

		if (rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
			auto l_DSV = l_DX12OutputMergerTarget->m_DSVs[0];
			l_PSO->m_GraphicsPSODesc.DSVFormat = l_DSV.m_Desc.Format;
			l_PSO->m_GraphicsPSODesc.DepthStencilState = l_PSO->m_DepthStencilDesc;
		}

		l_PSO->m_GraphicsPSODesc.RasterizerState = l_PSO->m_RasterizerDesc;
		l_PSO->m_GraphicsPSODesc.BlendState = l_PSO->m_BlendDesc;
		l_PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
		l_PSO->m_GraphicsPSODesc.PrimitiveTopologyType = l_PSO->m_PrimitiveTopologyType;
		l_PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;
		if (!l_PSO->m_RootSignature.Get())
		{
			Log(Verbose, "Skipping creating DX12 PSO for ", RenderPassComp->m_InstanceName);
			return true;
		}

		l_PSO->m_GraphicsPSODesc.pRootSignature = l_PSO->m_RootSignature.Get();

		CreateInputLayout(l_PSO);
		CreateShaderPrograms(RenderPassComp);

		auto l_HResult = m_device->CreateGraphicsPipelineState(&l_PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));
		if (FAILED(l_HResult))
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create Graphics PSO.");
			return false;
		}
	}
	else
	{
		CreateShaderPrograms(RenderPassComp);

		l_PSO->m_ComputePSODesc.pRootSignature = l_PSO->m_RootSignature.Get();
		auto l_HResult = m_device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create Compute PSO.");
			return false;
		}
	}

#ifdef INNO_DEBUG
	SetObjectName(RenderPassComp, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG

	Log(Verbose, RenderPassComp->m_InstanceName, " PSO has been created.");

	return true;
}

bool DX12RenderingServer::CreateCommandList(ICommandList* commandList, size_t swapChainImageIndex, const std::wstring& name)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	l_commandList->m_DirectCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCommandAllocators[swapChainImageIndex].Get(), (name + L"_DirectCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	l_commandList->m_ComputeCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[swapChainImageIndex].Get(), (name + L"_ComputeCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());
	l_commandList->m_CopyCommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_copyCommandAllocators[swapChainImageIndex].Get(), (name + L"_CopyCommandList_" + std::to_wstring(swapChainImageIndex)).c_str());

	l_commandList->m_DirectCommandList->Close();
	l_commandList->m_ComputeCommandList->Close();
	l_commandList->m_CopyCommandList->Close();

	return true;
}

bool DX12RenderingServer::CreateFenceEvents(RenderPassComponent* rhs)
{
	bool result = true;
	auto RenderPassComp = reinterpret_cast<RenderPassComponent*>(rhs);
	for (size_t i = 0; i < RenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(RenderPassComp->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create fence event for compute CommandQueue.");
			result = false;
		}

		l_semaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_CopyCommandQueueFenceEvent == NULL)
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't create fence event for copy CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, RenderPassComp->m_InstanceName, " Fence events have been created.");
	}

	return result;
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

bool DX12RenderingServer::EndFrame()
{
    m_PreviousFrame = m_CurrentFrame;
    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();
    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12RenderingServer::ResizeImpl()
{
    Log(Verbose, "Resizing the swap chain.");
   
    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainImages.clear();

    auto l_semaphoreValue = m_directCommandQueueFence->GetCompletedValue();
    auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

    Log(Verbose, "The current frame is ", m_CurrentFrame, " and the previous frame is ", m_PreviousFrame);
    m_swapChain->ResizeBuffers(m_swapChainImageCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, 0);

    m_SwapChainRenderPassComp->m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

    if (!GetSwapChainImages())
        return false;

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