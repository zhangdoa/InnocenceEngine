#include "DX12FrameManagementService.h"
#include "../../Engine.h"
#include "../../Platform/WinWindow/WinWindowService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../ConfigurationService.h"
#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12FrameManagementService::Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_pipelineStateObject = reinterpret_cast<DX12PipelineStateObject*>(pipelineStateObject);
	auto l_PSO = l_pipelineStateObject ? l_pipelineStateObject->m_PSO.Get() : nullptr;
	auto l_currentFrame = GetCurrentFrame();

	ID3D12CommandAllocator* allocator = nullptr;
	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		allocator = m_ctx->m_directCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Compute:
		allocator = m_ctx->m_computeCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Copy:
		allocator = m_ctx->m_copyCommandAllocators[l_currentFrame].Get();
		break;
	default:
		Log(Error, "Invalid command list type for Open operation");
		return false;
	}

	auto l_resetResult = l_commandList->Reset(allocator, l_PSO);
	if (FAILED(l_resetResult))
	{
		Log(Error, "DX12FrameManagementService::Open: Reset failed, HRESULT=", l_resetResult);
		return false;
	}
	return true;
}

bool DX12FrameManagementService::Close(CommandListComponent* commandList, GPUEngineType engineType)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_closeResult = l_commandList->Close();
	if (FAILED(l_closeResult))
	{
		Log(Error, "DX12FrameManagementService::Close: Close failed, HRESULT=", l_closeResult);
		return false;
	}
	return true;
}

bool DX12FrameManagementService::CreateSwapChainResources()
{
    if (!g_Engine->Get<ConfigurationService>()->IsOffscreen())
    {
        return CreateSwapChain();
    }
    return true;
}

bool DX12FrameManagementService::CreateSwapChain()
{
    m_swapChainDesc.BufferCount = m_swapChainImageCount;

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

    m_swapChainDesc.SampleDesc.Count = 1;
    m_swapChainDesc.SampleDesc.Quality = 0;

    m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    m_swapChainDesc.Flags = 0;

    m_swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    auto l_windowService = g_Engine->getWindowService();
    auto l_winWindowService = dynamic_cast<WinWindowService*>(l_windowService);
    if (!l_winWindowService)
    {
        Log(Error, "CreateSwapChain: Window service is not a WinWindowService! Can't create swap chain for HWND.");
        return false;
    }

    IDXGISwapChain1* l_swapChain1;
    auto l_hResult = m_ctx->m_factory->CreateSwapChainForHwnd(
        m_ctx->m_directCommandQueue.Get(),
        l_winWindowService->GetWindowHandle(),
        &m_swapChainDesc,
        nullptr,
        nullptr,
        &l_swapChain1);

    l_hResult = l_swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
    l_swapChain1->Release();

    if (FAILED(l_hResult))
    {
        Log(Error, "Can't create swap chain!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    Log(Success, "Swap chain has been created.");

    return true;
}
