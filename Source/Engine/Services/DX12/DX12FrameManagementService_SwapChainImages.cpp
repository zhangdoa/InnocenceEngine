#include "DX12FrameManagementService.h"
#include "../../Component/TextureComponent.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12FrameManagementService::GetSwapChainImages()
{
    Log(Verbose, "GetSwapChainImages: Called with offscreen=", g_Engine->getInitConfig().isOffscreen);

    if (g_Engine->getInitConfig().isOffscreen)
    {
        Log(Verbose, "GetSwapChainImages: Skipping in offscreen mode");
        return true;
    }

    if (!m_swapChain)
    {
        Log(Error, "GetSwapChainImages: m_swapChain is null! This should not happen in windowed mode.");
        return false;
    }

    m_swapChainImages.resize(m_swapChainImageCount);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        auto l_HResult = m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&m_swapChainImages[i]));
        if (FAILED(l_HResult))
        {
            auto l_drr = m_ctx->m_device ? m_ctx->m_device->GetDeviceRemovedReason() : S_OK;
            Log(Error, "Can't get pointer of swap chain image ", i,
                " HRESULT=", static_cast<int32_t>(l_HResult),
                " DeviceRemovedReason=", static_cast<int32_t>(l_drr));
            return false;
        }
        m_swapChainImages[i]->SetName((L"SwapChainBackBuffer_" + std::to_wstring(i)).c_str());
    }

    return true;
}

bool DX12FrameManagementService::AssignSwapChainImages()
{
    if (g_Engine->getInitConfig().isOffscreen)
    {
        return true;
    }

    if (!m_SwapChainRenderPassComp)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: m_SwapChainRenderPassComp is null");
        return false;
    }

    auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
    if (!l_outputMergerTarget)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: OutputMergerTarget is null");
        return false;
    }

    auto l_textureComp = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);
    if (!l_textureComp)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: swap chain color output texture is null");
        return false;
    }

    l_textureComp->m_GPUResources.resize(m_swapChainImageCount);
    l_textureComp->m_CurrentState.resize(m_swapChainImageCount, D3D12_RESOURCE_STATE_PRESENT);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        l_textureComp->m_GPUResources[i] = m_swapChainImages[i].Get();
    }

    auto l_textureDesc = m_swapChainImages[0]->GetDesc();
    l_textureComp->m_TextureDesc.Width = l_textureDesc.Width;
    l_textureComp->m_TextureDesc.Height = l_textureDesc.Height;
    l_textureComp->m_TextureDesc.IsMultiBuffer = true;
    l_textureComp->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_PRESENT);
    l_textureComp->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_RENDER_TARGET);
    l_textureComp->m_ObjectStatus = ObjectStatus::Activated;

    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12FrameManagementService::ReleaseSwapChainImages()
{
    return true;
}
