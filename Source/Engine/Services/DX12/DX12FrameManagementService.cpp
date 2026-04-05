#include "DX12FrameManagementService.h"
#include "DX12GraphicsResourceService.h"
#include "../GraphicsResourceService.h"
#include "../GraphicsHardwareService.h"
#include "../../Engine.h"
#include "../../Platform/WinWindow/WinWindowService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

using namespace Inno;
using namespace DX12Helper;

static std::atomic<bool> g_GPUErrorDetected{false};

#ifdef _WIN32
static std::string CaptureCallstack(UINT framesToSkip = 1, UINT maxFrames = 10)
{
    static bool symbolsInitialized = false;
    if (!symbolsInitialized)
    {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        symbolsInitialized = true;
    }

    std::string callstack = "\nCallstack:\n";

    void* stack[32];
    WORD numberOfFrames = CaptureStackBackTrace(framesToSkip, maxFrames, stack, NULL);

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (int i = 0; i < numberOfFrames; i++)
    {
        DWORD64 address = (DWORD64)(stack[i]);

        if (SymFromAddr(GetCurrentProcess(), address, 0, symbol))
        {
            callstack += "  " + std::to_string(i) + ": " + symbol->Name + " (0x" +
                        std::to_string(address) + ")\n";
        }
        else
        {
            callstack += "  " + std::to_string(i) + ": <unknown> (0x" +
                        std::to_string(address) + ")\n";
        }
    }

    free(symbol);
    return callstack;
}
#endif

static void CALLBACK D3D12DebugMessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    switch (Severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR:
        {
            g_GPUErrorDetected.store(true);
            if (pContext)
                static_cast<DX12Context*>(pContext)->m_GPUErrorDetected.store(true);
#ifdef _WIN32
            std::string callstackInfo = CaptureCallstack(2, 15);
            Log(Error, "D3D12 ERROR: ", pDescription, callstackInfo.c_str());
#else
            Log(Error, "D3D12 ERROR: ", pDescription);
#endif
        }
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        Log(Warning, "D3D12 WARNING: ", pDescription);
        break;
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
        Log(Verbose, "D3D12 INFO: ", pDescription);
        break;
    }
}

bool DX12FrameManagementService::CreateDebugCallback()
{
    ID3D12Debug* l_debugInterface;

    auto l_HResult = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't get DirectX 12 debug interface!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    l_HResult = l_debugInterface->QueryInterface(IID_PPV_ARGS(&m_DX12Context.m_debugInterface));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't query DirectX 12 debug interface!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    m_DX12Context.m_debugInterface->EnableDebugLayer();
    //m_DX12Context.m_debugInterface->SetEnableGPUBasedValidation(true);
    //m_DX12Context.m_debugInterface->SetEnableSynchronizedCommandQueueValidation(true);

    Log(Success, "Debug layer and GPU based validation has been enabled.");

    l_HResult = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DX12Context.m_graphicsAnalysis));
    if (SUCCEEDED(l_HResult))
    {
        Log(Success, "PIX attached.");
    }

    return true;
}

bool DX12FrameManagementService::CreatePhysicalDevices()
{
    UINT l_DXGIFlag = 0;
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    l_DXGIFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif // INNO_DEBUG

    auto l_HResult = CreateDXGIFactory2(l_DXGIFlag, IID_PPV_ARGS(&m_DX12Context.m_factory));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create DXGI factory!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    Log(Success, "DXGI factory has been created.");

    IDXGIAdapter1* l_adapter;
    UINT adapterIndex = 0;
    l_HResult = m_DX12Context.m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&l_adapter));
    if (FAILED(l_HResult))
    {
        Log(Warning, "Can't find a high-performance GPU.");
        l_HResult = m_DX12Context.m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&l_adapter));
        if (FAILED(l_HResult))
        {
            Log(Error, "Can't find any capable GPU!");
            m_ObjectStatus = ObjectStatus::Suspended;
            return false;
        }
    }

    if (FAILED(D3D12CreateDevice(l_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
    {
        Log(Error, "Adapter doesn't support DirectX 12!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    if (l_adapter == nullptr)
    {
        Log(Error, "Can't create a suitable video card adapter!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    m_DX12Context.m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter);

    DXGI_ADAPTER_DESC3 l_adapter_desc;
    m_DX12Context.m_adapter->GetDesc3(&l_adapter_desc);
    std::wstring l_descL = std::wstring(l_adapter_desc.Description);

    int length = WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string l_desc(length, 0);
    WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, l_desc.data(), length, nullptr, nullptr);

    Log(Success, "Adapter for: ", l_desc.c_str(), " has been created.");

    IDXGIOutput* l_adapterOutput;
    l_HResult = m_DX12Context.m_adapter->EnumOutputs(0, &l_adapterOutput);
    if (FAILED(l_HResult))
    {
        Log(Warning, "the primary output of the adapter is not connected.");
        // @TODO: Find a way to enumerate until we get the actual monitor
    }
    else
    {
        l_HResult = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&m_DX12Context.m_adapterOutput));
    }

    uint32_t l_numModes;
    uint64_t l_stringLength;

    l_HResult = m_DX12Context.m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, NULL);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    std::vector<DXGI_MODE_DESC1> l_displayModeList(l_numModes);

    l_HResult = m_DX12Context.m_adapterOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, &l_displayModeList[0]);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't fill the display mode list structures!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    for (uint32_t i = 0; i < l_numModes; i++)
    {
        if (l_displayModeList[i].Width == l_screenResolution.x &&
            l_displayModeList[i].Height == l_screenResolution.y)
        {
            m_refreshRate.x = l_displayModeList[i].RefreshRate.Numerator;
            m_refreshRate.y = l_displayModeList[i].RefreshRate.Denominator;
        }
    }

    l_HResult = m_DX12Context.m_adapter->GetDesc(&m_DX12Context.m_adapterDesc);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't get the video card adapter description!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    m_DX12Context.m_videoCardMemory = (int32_t)(m_DX12Context.m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    if (wcstombs_s(&l_stringLength, m_DX12Context.m_videoCardDescription, 128, m_DX12Context.m_adapterDesc.Description, 128) != 0)
    {
        Log(Error, "can't convert the name of the video card to a character array!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    auto featureLevel = D3D_FEATURE_LEVEL_12_2;

    l_HResult = D3D12CreateDevice(m_DX12Context.m_adapter.Get(), featureLevel, IID_PPV_ARGS(&m_DX12Context.m_device));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create a DirectX 12.2 device. The default video card does not support DirectX 12.2!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(m_DX12Context.m_device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS,
        &options,
        sizeof(options))))
    {
        if (!options.TypedUAVLoadAdditionalFormats)
            Log(Warning, "TypedUAVLoadAdditionalFormats is not supported, can't generate mipmap for sRGB textures.");
    }

    Log(Success, "D3D device has been created.");

    ComPtr<ID3D12InfoQueue> l_pInfoQueue;
    l_HResult = m_DX12Context.m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

    if (SUCCEEDED(l_HResult) && l_pInfoQueue)
    {
        if (SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE))
            && SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE))
            //&& SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE))
            )
        {
            Log(Success, "Debug report severity has been set.");

            ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
            l_HResult = l_pInfoQueue.As(&l_pInfoQueue1);
            if (SUCCEEDED(l_HResult) && l_pInfoQueue1)
            {
                l_HResult = l_pInfoQueue1->RegisterMessageCallback(
                    D3D12DebugMessageCallback,
                    D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                    &m_DX12Context,
                    &m_DX12Context.m_debugCallbackCookie);

                if (SUCCEEDED(l_HResult))
                {
                    Log(Success, "D3D12 debug message callback registered.");
                }
                else
                {
                    Log(Warning, "Failed to register D3D12 debug message callback.");
                }
            }
            else
            {
                Log(Warning, "ID3D12InfoQueue1 not available - debug message callback not supported.");
            }
        }
    }
    else
    {
        Log(Warning, "Debug info queue not available (debug layer not enabled).");
    }

    return true;
}

bool DX12FrameManagementService::CreateGlobalCommandQueues()
{
    D3D12_COMMAND_QUEUE_DESC l_graphicCommandQueueDesc = {};
    l_graphicCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    l_graphicCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_graphicCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_graphicCommandQueueDesc.NodeMask = 0;

    D3D12_COMMAND_QUEUE_DESC l_computeCommandQueueDesc = {};
    l_computeCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    l_computeCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_computeCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_computeCommandQueueDesc.NodeMask = 0;

    D3D12_COMMAND_QUEUE_DESC l_copyCommandQueueDesc = {};
    l_copyCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    l_copyCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_copyCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_copyCommandQueueDesc.NodeMask = 0;

    m_DX12Context.m_directCommandQueue = m_DX12Context.CreateCommandQueue(&l_graphicCommandQueueDesc, L"DirectCommandQueue");
    m_DX12Context.m_computeCommandQueue = m_DX12Context.CreateCommandQueue(&l_computeCommandQueueDesc, L"ComputeCommandQueue");
    m_DX12Context.m_copyCommandQueue = m_DX12Context.CreateCommandQueue(&l_copyCommandQueueDesc, L"CopyCommandQueue");

    Log(Success, "Global CommandQueues have been created.");

    return true;
}

bool DX12FrameManagementService::CreateGlobalCommandAllocators()
{
    m_DX12Context.m_directCommandAllocators.resize(m_swapChainImageCount);
    m_DX12Context.m_computeCommandAllocators.resize(m_swapChainImageCount);
    m_DX12Context.m_copyCommandAllocators.resize(m_swapChainImageCount);
    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_DX12Context.m_directCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_computeCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_copyCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, (L"CopyCommandAllocator_" + std::to_wstring(i)).c_str());
    }

    Log(Success, "Global CommandAllocators have been created.");

    return true;
}

bool DX12FrameManagementService::CreateSyncPrimitives()
{
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_directCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for direct CommandQueue!");
        return false;
    }
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_computeCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for compute CommandQueue!");
        return false;
    }
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_copyCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for copy CommandQueue!");
        return false;
    }
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    m_DX12Context.m_directCommandQueueFence->SetName(L"DirectCommandQueueFence");
    m_DX12Context.m_computeCommandQueueFence->SetName(L"ComputeCommandQueueFence");
    m_DX12Context.m_copyCommandQueueFence->SetName(L"CopyCommandQueueFence");
#endif // INNO_DEBUG

    Log(Verbose, "Fences for global CommandQueues have been created.");

    auto l_GlobalSemaphore = static_cast<DX12Semaphore*>(m_ResourceService->AddSemaphore());
    l_GlobalSemaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    m_GlobalSemaphore = l_GlobalSemaphore;

    Log(Verbose, "Fence events for global CommandQueues have been created.");

    return true;
}

bool DX12FrameManagementService::CreateGlobalDescriptorHeaps()
{
    auto l_renderingCapacity = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

    uint32_t l_maxGPUBuffers = l_renderingCapacity.maxBuffers;
    uint32_t l_maxMaterialTextures = l_renderingCapacity.maxTextures;
    uint32_t l_maxRenderTargetTextures = 2048;

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers * 3 + l_maxMaterialTextures * 2 + l_maxRenderTargetTextures * 2;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, true);
        m_DX12Context.m_CSUDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderVisible");
        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;
        auto l_RenderTargetTextureDescriptorSectionSize = l_maxRenderTargetTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferCBVHandle = {};
        DescriptorHandle l_firstGPUBufferSRVHandle = {};
        DescriptorHandle l_firstGPUBufferUAVHandle = {};

        {
            l_firstGPUBufferCBVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            l_firstGPUBufferCBVHandle.m_GPUHandle = m_DX12Context.m_CSUDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

            m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferCBVHandle, true, L"GPUBuffer_CBV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferCBVHandle;
            D3D12_CONSTANT_BUFFER_VIEW_DESC l_CBVDesc = {};
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateConstantBufferView(nullptr, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferSRVHandle.m_CPUHandle = l_firstGPUBufferCBVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferSRVHandle.m_GPUHandle = l_firstGPUBufferCBVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferSRVHandle, true, L"GPUBuffer_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            l_SRVDesc.Format = DXGI_FORMAT_R32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferUAVHandle.m_CPUHandle = l_firstGPUBufferSRVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferUAVHandle.m_GPUHandle = l_firstGPUBufferSRVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, true, L"GPUBuffer_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_SINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstMaterialTextureSRVHandle = {};
        DescriptorHandle l_firstMaterialTextureUAVHandle = {};
        {
            l_firstMaterialTextureSRVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstMaterialTextureSRVHandle.m_GPUHandle = l_firstGPUBufferUAVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureSRVHandle, true, L"MaterialTexture_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstMaterialTextureUAVHandle.m_CPUHandle = l_firstMaterialTextureSRVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstMaterialTextureUAVHandle.m_GPUHandle = l_firstMaterialTextureSRVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureUAVHandle, true, L"MaterialTexture_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstRenderTargetTextureSRVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureUAVHandle = {};
        {
            l_firstRenderTargetTextureSRVHandle.m_CPUHandle = l_firstMaterialTextureUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstRenderTargetTextureSRVHandle.m_GPUHandle = l_firstMaterialTextureUAVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureSRVHandle, true, L"RenderTarget_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstRenderTargetTextureUAVHandle.m_CPUHandle = l_firstRenderTargetTextureSRVHandle.m_CPUHandle + l_RenderTargetTextureDescriptorSectionSize;
            l_firstRenderTargetTextureUAVHandle.m_GPUHandle = l_firstRenderTargetTextureSRVHandle.m_GPUHandle + l_RenderTargetTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureUAVHandle, true, L"RenderTarget_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }
    }

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers + l_maxMaterialTextures + l_maxRenderTargetTextures;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, false);
        m_DX12Context.m_CSUDescHeap_ShaderNonVisible = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderNonVisible");

        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferUAVHandle = {};
        DescriptorHandle l_firstMaterialTextureBufferUAVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureBufferUAVHandle = {};

        l_firstGPUBufferUAVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap_ShaderNonVisible->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstMaterialTextureBufferUAVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
        l_firstRenderTargetTextureBufferUAVHandle.m_CPUHandle = l_firstMaterialTextureBufferUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;

        {
            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, false, L"GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_UINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxMaterialTextures
                , l_incrementSize, l_firstMaterialTextureBufferUAVHandle, false, L"MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstMaterialTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxRenderTargetTextures
                , l_incrementSize, l_firstRenderTargetTextureBufferUAVHandle, false, L"RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstRenderTargetTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }
    }

    {
        uint32_t l_maxSamplerCount = 128;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, l_maxSamplerCount, true);
        m_DX12Context.m_SamplerDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalSamplerDescHeap");

        DescriptorHandle l_firstSamplerHandle = {};
        l_firstSamplerHandle.m_CPUHandle = m_DX12Context.m_SamplerDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstSamplerHandle.m_GPUHandle = m_DX12Context.m_SamplerDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_SamplerDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_SamplerDescHeap, l_Desc, l_maxSamplerCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER), l_firstSamplerHandle, true, L"SamplerDescHeapAccessor");
    }

    {
        uint32_t l_maxRTVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, l_maxRTVCount, false);
        m_DX12Context.m_RTVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalRTVDescHeap");

        DescriptorHandle l_firstRTVHandle = {};
        l_firstRTVHandle.m_CPUHandle = m_DX12Context.m_RTVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_RTVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_RTVDescHeap, l_Desc, l_maxRTVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), l_firstRTVHandle, false, L"RTVDescHeapAccessor");
    }

    {
        uint32_t l_maxDSVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, l_maxDSVCount, false);
        m_DX12Context.m_DSVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalDSVDescHeap");

        DescriptorHandle l_firstDSVHandle = {};
        l_firstDSVHandle.m_CPUHandle = m_DX12Context.m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_DSVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_DSVDescHeap, l_Desc, l_maxDSVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), l_firstDSVHandle, false, L"DSVDescHeapAccessor");
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

    // @TODO: finish this feature
    m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    m_swapChainDesc.Flags = 0;

    m_swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    IDXGISwapChain1* l_swapChain1;
    auto l_hResult = m_DX12Context.m_factory->CreateSwapChainForHwnd(
        m_DX12Context.m_directCommandQueue.Get(),
        reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetWindowHandle(),
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

bool DX12FrameManagementService::CreateHardwareResources()
{
    bool l_result = true;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    l_result &= CreateDebugCallback();
#endif
    l_result &= CreatePhysicalDevices();
    l_result &= CreateGlobalCommandQueues();
    l_result &= CreateGlobalCommandAllocators();
    l_result &= CreateSyncPrimitives();
    l_result &= CreateGlobalDescriptorHeaps();

    auto l_resourceService = static_cast<DX12GraphicsResourceService*>(m_ResourceService);
    l_result &= l_resourceService->CreateMipmapGenerator();

    if (!g_Engine->getInitConfig().isOffscreen)
    {
        l_result &= CreateSwapChain();
    }

    l_result &= l_resourceService->CreateRaytracingResources();

    return l_result;
}

bool DX12FrameManagementService::ReleaseHardwareResources()
{
    auto l_resourceService = static_cast<DX12GraphicsResourceService*>(m_ResourceService);
    l_resourceService->ReleaseRaytracingResources();
    l_resourceService->ReleaseMipmapGenerator();

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

    m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor.Reset();
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

    m_swapChain = nullptr;

    if (m_DX12Context.m_debugInterface && m_DX12Context.m_debugCallbackCookie != 0)
    {
        ComPtr<ID3D12InfoQueue> l_pInfoQueue;
        HRESULT l_HResult = m_DX12Context.m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));
        if (SUCCEEDED(l_HResult) && l_pInfoQueue)
        {
            ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
            l_HResult = l_pInfoQueue.As(&l_pInfoQueue1);
            if (SUCCEEDED(l_HResult) && l_pInfoQueue1)
            {
                l_pInfoQueue1->UnregisterMessageCallback(m_DX12Context.m_debugCallbackCookie);
                Log(Verbose, "D3D12 debug message callback unregistered.");
                m_DX12Context.m_debugCallbackCookie = 0;
            }
        }
    }

    m_DX12Context.m_device = nullptr;

    m_DX12Context.m_adapterOutput = nullptr;

    m_DX12Context.m_adapter = nullptr;

    m_DX12Context.m_factory = nullptr;

    m_DX12Context.m_graphicsAnalysis = nullptr;

    m_DX12Context.m_debugInterface = nullptr;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        pDebug->Release();
    }
#endif

    return true;
}

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
            Log(Error, "Can't get pointer of swap chain image ", i, "!");
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

    auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
    auto l_textureComp = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);

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

bool DX12FrameManagementService::BeginFrame()
{
    auto l_currentFrame = m_CurrentFrame;
    if (FAILED(m_DX12Context.m_directCommandAllocators[l_currentFrame]->Reset()))
        return false;
    if (FAILED(m_DX12Context.m_computeCommandAllocators[l_currentFrame]->Reset()))
        return false;
    if (FAILED(m_DX12Context.m_copyCommandAllocators[l_currentFrame]->Reset()))
        return false;

    m_ResourceService->ForEachCommandList([this](CommandListComponent* cl)
    {
        if (cl && cl->m_ObjectStatus == ObjectStatus::Activated)
        {
            m_HardwareService->Open(cl, cl->m_Type, nullptr);
            m_HardwareService->Close(cl, cl->m_Type);
        }
    });

    return true;
}

bool DX12FrameManagementService::PrepareRayTracing(CommandListComponent* commandList)
{
    auto l_currentFrame = m_CurrentFrame;
    auto& l_raytracingInstanceDescs = m_ResourceService->GetRaytracingInstanceDescs();
    auto l_instanceDescList = reinterpret_cast<DX12RaytracingInstanceDescList*>(l_raytracingInstanceDescs[l_currentFrame]);

    if (l_instanceDescList->m_Descs.size() == 0)
        return true;

    auto l_TLASBufferComponent = m_ResourceService->GetTLASBufferComponent();
    if (l_TLASBufferComponent->m_ObjectStatus != ObjectStatus::Activated)
    {
        Log(Warning, "TLAS buffer not activated - skipping TLAS build");
        return true;
    }

    auto l_RaytracingInstanceBufferComponent = m_ResourceService->GetRaytracingInstanceBufferComponent();
    auto l_mappedMemory = l_RaytracingInstanceBufferComponent->m_MappedMemories[l_currentFrame];
    m_ResourceService->WriteMappedMemory(l_RaytracingInstanceBufferComponent, l_mappedMemory, &l_instanceDescList->m_Descs[0], 0, l_instanceDescList->m_Descs.size());
    l_mappedMemory->m_NeedUploadToGPU = false;

    auto l_instanceBuffer = reinterpret_cast<DX12DeviceMemory*>(l_RaytracingInstanceBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

    auto instanceBarrier_UploadToDefaultHeap = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    l_commandList->ResourceBarrier(1, &instanceBarrier_UploadToDefaultHeap);

    m_ResourceService->UploadToGPU(commandList, l_RaytracingInstanceBufferComponent);

    auto instanceBarrierTLASBuild = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
    l_commandList->ResourceBarrier(1, &instanceBarrierTLASBuild);

    auto l_ScratchBufferComponent = m_ResourceService->GetScratchBufferComponent();
    auto l_TLASBuffer = reinterpret_cast<DX12DeviceMemory*>(l_TLASBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_scratchBuffer = reinterpret_cast<DX12DeviceMemory*>(l_ScratchBufferComponent->m_DeviceMemories[l_currentFrame]);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
    tlasDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasDesc.Inputs.NumDescs = l_instanceDescList->m_Descs.size();
    tlasDesc.Inputs.InstanceDescs = l_instanceBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();
    tlasDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    tlasDesc.SourceAccelerationStructureData = 0;
    tlasDesc.DestAccelerationStructureData = l_TLASBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData = l_scratchBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();

    l_commandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);

    CD3DX12_RESOURCE_BARRIER tlasBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_TLASBuffer->m_DefaultHeapBuffer.Get());
    l_commandList->ResourceBarrier(1, &tlasBarrier);

    m_ResourceService->SetTLASReady(true);

    return true;
}

bool DX12FrameManagementService::PresentImpl()
{
    if (g_Engine->getInitConfig().isOffscreen)
    {
        return true;
    }

    m_swapChain->Present(0, 0);

    return true;
}

bool DX12FrameManagementService::EndFrame()
{
    if (g_Engine->getInitConfig().isOffscreen)
    {
        return true;
    }

    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();
    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12FrameManagementService::ResizeImpl()
{
    if (g_Engine->getInitConfig().isOffscreen)
    {
        return true;
    }

    // @TODO: reset m_RenderTarget_SRV_DescHeapAccessor and other render target desc heaps
    Log(Verbose, "Resizing the swap chain...");

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    auto l_semaphoreValue = m_DX12Context.m_directCommandQueueFence->GetCompletedValue();
    auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

    m_swapChainImages.clear();

    auto l_previousFrame = (m_CurrentFrame == 0 ? m_swapChainImageCount - 1 : m_CurrentFrame - 1);
    Log(Verbose, "The current frame is ", m_CurrentFrame, " and the previous frame is ", l_previousFrame);
    m_swapChain->ResizeBuffers(m_swapChainImageCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, 0);

    GetSwapChainImages();

    return true;
}

bool DX12FrameManagementService::WaitAllOnCPU()
{
    auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
    if (!l_semaphore)
        return false;

    auto waitFence = [&](ComPtr<ID3D12Fence>& fence, ComPtr<ID3D12CommandQueue>& queue, HANDLE fenceEvent) -> bool
    {
        if (!fence || !queue)
            return true;

        uint64_t l_value = fence->GetCompletedValue() + 1;
        queue->Signal(fence.Get(), l_value);

        if (fence->GetCompletedValue() < l_value)
        {
            fence->SetEventOnCompletion(l_value, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        return true;
    };

    bool l_result = true;
    l_result &= waitFence(m_DX12Context.m_directCommandQueueFence, m_DX12Context.m_directCommandQueue, l_semaphore->m_DirectCommandQueueFenceEvent);
    l_result &= waitFence(m_DX12Context.m_computeCommandQueueFence, m_DX12Context.m_computeCommandQueue, l_semaphore->m_ComputeCommandQueueFenceEvent);
    l_result &= waitFence(m_DX12Context.m_copyCommandQueueFence, m_DX12Context.m_copyCommandQueue, l_semaphore->m_CopyCommandQueueFenceEvent);

    return l_result;
}
