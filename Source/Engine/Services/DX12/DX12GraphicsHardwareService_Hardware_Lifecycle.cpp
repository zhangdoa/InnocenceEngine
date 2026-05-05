#include "DX12GraphicsHardwareService.h"
#include "DX12TextureResourceService.h"
#include "DX12GPUBufferResourceService.h"
#include "../TextureResourceService.h"
#include "../GPUBufferResourceService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Inno;

bool DX12GraphicsHardwareService::CreateHardwareResources()
{
    bool l_result = true;

    TryLoadRenderDocAPI();
    TryLoadPIXEventRuntime();

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    if (g_Engine->getInitConfig().enableGPUValidation)
        l_result &= CreateDebugCallback();
    else
        Log(Warning, "D3D12 debug layer disabled by default to avoid TDR from validation overhead. Pass -gpu_validation to enable.");
#endif
    l_result &= CreatePhysicalDevices();
    l_result &= CreateGlobalCommandQueues();
    l_result &= CreateGlobalCommandAllocators();
    l_result &= CreateSyncPrimitives();
    l_result &= CreateGlobalDescriptorHeaps();
    // Timer resources need m_FrameManagementService->GetSwapChainImageCount,
    // which is set by FrameManagement Setup before HardwareService Setup runs.
    l_result &= CreateGpuTimerResources();

    auto l_textureService = static_cast<DX12TextureResourceService*>(g_Engine->Get<TextureResourceService>());
    auto l_gpuBufferService = static_cast<DX12GPUBufferResourceService*>(g_Engine->Get<GPUBufferResourceService>());
    l_result &= l_textureService->CreateMipmapGenerator();
    l_result &= l_gpuBufferService->CreateRaytracingResources();

    return l_result;
}

bool DX12GraphicsHardwareService::ReleaseHardwareResources()
{
    auto l_gpuBufferService = static_cast<DX12GPUBufferResourceService*>(g_Engine->Get<GPUBufferResourceService>());
    auto l_textureService = static_cast<DX12TextureResourceService*>(g_Engine->Get<TextureResourceService>());
    l_gpuBufferService->ReleaseRaytracingResources();
    l_textureService->ReleaseMipmapGenerator();

    ReleaseGpuTimerResources();
    if (m_PIXModule)
    {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(m_PIXModule));
#endif
        m_PIXModule = nullptr;
        m_PIXBeginEventOnCommandList = nullptr;
        m_PIXEndEventOnCommandList = nullptr;
    }

    m_DX12Context.m_SamplerDescHeapAccessor.Reset();
    m_DX12Context.m_SamplerDescHeap = nullptr;

    m_DX12Context.m_DSVDescHeapAccessor.Reset();
    m_DX12Context.m_DSVDescHeap = nullptr;

    m_DX12Context.m_RTVDescHeapAccessor.Reset();
    m_DX12Context.m_RTVDescHeap = nullptr;

    m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_CSUDescHeap_ShaderNonVisible = nullptr;

    m_DX12Context.m_BindlessMeshIndex_SRV_DescHeapAccessor.Reset(); m_DX12Context.m_BindlessMeshVertex_SRV_DescHeapAccessor.Reset(); m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor.Reset(); m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor.Reset(); m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor.Reset(); m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor.Reset(); m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor.Reset(); m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor.Reset();
    m_DX12Context.m_CSUDescHeap = nullptr;

    m_DX12Context.m_directCommandQueueFence = nullptr;
    m_DX12Context.m_computeCommandQueueFence = nullptr;
    m_DX12Context.m_copyCommandQueueFence = nullptr;

    m_DX12Context.m_directCommandAllocators.clear();
    m_DX12Context.m_computeCommandAllocators.clear();
    m_DX12Context.m_copyCommandAllocators.clear();

    m_DX12Context.m_directCommandQueue = nullptr;
    m_DX12Context.m_computeCommandQueue = nullptr;
    m_DX12Context.m_copyCommandQueue = nullptr;

    try
    {
        if (m_DX12Context.m_debugInterface && m_DX12Context.m_debugCallbackCookie != 0)
        {
            ComPtr<ID3D12InfoQueue> l_pInfoQueue;
            if (SUCCEEDED(m_DX12Context.m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue))) && l_pInfoQueue)
            {
                ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
                if (SUCCEEDED(l_pInfoQueue.As(&l_pInfoQueue1)) && l_pInfoQueue1)
                {
                    l_pInfoQueue1->UnregisterMessageCallback(m_DX12Context.m_debugCallbackCookie);
                    m_DX12Context.m_debugCallbackCookie = 0;
                }
            }
        }
    }
    catch (...)
    {
        Log(Warning, "Exception during debug callback cleanup, device may already be removed.");
    }

    m_DX12Context.m_device = nullptr;
    m_DX12Context.m_adapterOutput = nullptr;
    m_DX12Context.m_adapter = nullptr;
    m_DX12Context.m_factory = nullptr;
    m_DX12Context.m_graphicsAnalysis = nullptr;
    m_DX12Context.m_debugInterface = nullptr;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    try
    {
        IDXGIDebug1* pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            pDebug->Release();
        }
    }
    catch (...)
    {
        Log(Warning, "Exception during DXGI debug report, skipping.");
    }
#endif

    return true;
}
