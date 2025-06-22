#include "DX12RenderingServer.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../../Services/RenderingConfigurationService.h"

using namespace Inno;
#include "../../Engine.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

using namespace DX12Helper;

#ifdef _WIN32
// Helper function to capture and format callstack
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

// Static callback function to capture D3D12 debug messages
static void CALLBACK D3D12DebugMessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    // Convert D3D12 severity to engine log level
    switch (Severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR:
        {
#ifdef _WIN32
            std::string callstackInfo = CaptureCallstack(2, 15);  // Skip callback frames, capture up to 15 frames
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

bool DX12RenderingServer::CreateDebugCallback()
{
    ID3D12Debug* l_debugInterface;

    auto l_HResult = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't get DirectX 12 debug interface!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    l_HResult = l_debugInterface->QueryInterface(IID_PPV_ARGS(&m_debugInterface));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't query DirectX 12 debug interface!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    m_debugInterface->EnableDebugLayer();
    //m_debugInterface->SetEnableGPUBasedValidation(true);
    //m_debugInterface->SetEnableSynchronizedCommandQueueValidation(true);
   
    Log(Success, "Debug layer and GPU based validation has been enabled.");

    l_HResult = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_graphicsAnalysis));
    if (SUCCEEDED(l_HResult))
    {
        Log(Success, "PIX attached.");
    }

    return true;
}

bool DX12RenderingServer::CreatePhysicalDevices()
{
    // Create a DirectX graphics interface factory.
    UINT l_DXGIFlag = 0;
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    l_DXGIFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif // INNO_DEBUG

    auto l_HResult = CreateDXGIFactory2(l_DXGIFlag, IID_PPV_ARGS(&m_factory));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create DXGI factory!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    Log(Success, "DXGI factory has been created.");

    // Choose a dedicated adapter
    IDXGIAdapter1* l_adapter;
    UINT adapterIndex = 0;
    l_HResult = m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&l_adapter));
    if (FAILED(l_HResult))
    {
        Log(Warning, "Can't find a high-performance GPU.");
        l_HResult = m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&l_adapter));
        if (FAILED(l_HResult))
        {
            Log(Error, "Can't find any capable GPU!");
            m_ObjectStatus = ObjectStatus::Suspended;
            return false;
        }
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the
    // actual device yet.
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

    m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter);

    DXGI_ADAPTER_DESC3 l_adapter_desc;
    m_adapter->GetDesc3(&l_adapter_desc);
    std::wstring l_descL = std::wstring(l_adapter_desc.Description);

    int length = WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string l_desc(length, 0);
    WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, l_desc.data(), length, nullptr, nullptr);

    Log(Success, "Adapter for: ", l_desc.c_str(), " has been created.");

    // Enumerate the primary adapter output (monitor).
    IDXGIOutput* l_adapterOutput;
    l_HResult = m_adapter->EnumOutputs(0, &l_adapterOutput);
    if (FAILED(l_HResult))
    {
        Log(Warning, "the primary output of the adapter is not connected.");
        // @TODO: Find a way to enumerate until we get the actual monitor
    }
    else
    {
        l_HResult = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&m_adapterOutput));
    }

    uint32_t l_numModes;
    uint64_t l_stringLength;

    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
    l_HResult = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, NULL);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    // Create a list to hold all the possible display modes for this monitor/video card combination.
    std::vector<DXGI_MODE_DESC1> l_displayModeList(l_numModes);

    // Now fill the display mode list structures.
    l_HResult = m_adapterOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &l_numModes, &l_displayModeList[0]);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't fill the display mode list structures!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    // Now go through all the display modes and find the one that matches the screen width and height.
    // When a match is found store the numerator and denominator of the refresh rate for that monitor.
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

    // Get the adapter (video card) description.
    l_HResult = m_adapter->GetDesc(&m_adapterDesc);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't get the video card adapter description!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    // Store the dedicated video card memory in megabytes.
    m_videoCardMemory = (int32_t)(m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    // Convert the name of the video card to a character array and store it.
    if (wcstombs_s(&l_stringLength, m_videoCardDescription, 128, m_adapterDesc.Description, 128) != 0)
    {
        Log(Error, "can't convert the name of the video card to a character array!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    auto featureLevel = D3D_FEATURE_LEVEL_12_2;

    // Create the Direct3D 12 device.
    l_HResult = D3D12CreateDevice(m_adapter.Get(), featureLevel, IID_PPV_ARGS(&m_device));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create a DirectX 12.2 device. The default video card does not support DirectX 12.2!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(m_device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS,
        &options,
        sizeof(options))))
    {
        if (!options.TypedUAVLoadAdditionalFormats)
            Log(Warning, "TypedUAVLoadAdditionalFormats is not supported, can't generate mipmap for sRGB textures.");
    }

    Log(Success, "D3D device has been created.");

    // Set debug report severity
    ComPtr<ID3D12InfoQueue> l_pInfoQueue;
    l_HResult = m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

    if (SUCCEEDED(l_HResult) && l_pInfoQueue)
    {
        if (SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE))
            && SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE))
            //&& SUCCEEDED(l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE))
            )
        {
            Log(Success, "Debug report severity has been set.");
            
            // Register message callback to capture debug messages (requires ID3D12InfoQueue1)
            ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
            l_HResult = l_pInfoQueue.As(&l_pInfoQueue1);
            if (SUCCEEDED(l_HResult) && l_pInfoQueue1)
            {
                l_HResult = l_pInfoQueue1->RegisterMessageCallback(
                    D3D12DebugMessageCallback,
                    D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                    nullptr,
                    &m_debugCallbackCookie);
                    
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

bool DX12RenderingServer::CreateGlobalCommandQueues()
{
    // Set up the description of the command queues.
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

    m_directCommandQueue = CreateCommandQueue(&l_graphicCommandQueueDesc, L"DirectCommandQueue");
    m_computeCommandQueue = CreateCommandQueue(&l_computeCommandQueueDesc, L"ComputeCommandQueue");
    m_copyCommandQueue = CreateCommandQueue(&l_copyCommandQueueDesc, L"CopyCommandQueue");

    Log(Success, "Global CommandQueues have been created.");

    return true;
}

bool DX12RenderingServer::CreateGlobalCommandAllocators()
{
    m_directCommandAllocators.resize(m_swapChainImageCount);
    m_computeCommandAllocators.resize(m_swapChainImageCount);
    m_copyCommandAllocators.resize(m_swapChainImageCount);    
    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
        m_computeCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
        m_copyCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, (L"CopyCommandAllocator_" + std::to_wstring(i)).c_str());
    }

    Log(Success, "Global CommandAllocators have been created.");

    return true;
}

bool DX12RenderingServer::CreateSyncPrimitives()
{
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_directCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for direct CommandQueue!");
        return false;
    }
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for compute CommandQueue!");
        return false;
    }
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_copyCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for copy CommandQueue!");
        return false;
    }
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    m_directCommandQueueFence->SetName(L"DirectCommandQueueFence");
    m_computeCommandQueueFence->SetName(L"ComputeCommandQueueFence");
    m_copyCommandQueueFence->SetName(L"CopyCommandQueueFence");
#endif // INNO_DEBUG

    Log(Verbose, "Fences for global CommandQueues have been created.");

    auto l_GlobalSemaphore = static_cast<DX12Semaphore*>(AddSemaphore());
    l_GlobalSemaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    m_GlobalSemaphore = l_GlobalSemaphore;
    
    Log(Verbose, "Fence events for global CommandQueues have been created.");
    
    return true;
}

bool DX12RenderingServer::CreateGlobalDescriptorHeaps()
{
    auto l_renderingCapacity = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

    uint32_t l_maxGPUBuffers = l_renderingCapacity.maxBuffers;
    uint32_t l_maxMaterialTextures = l_renderingCapacity.maxTextures;
    uint32_t l_maxRenderTargetTextures = 2048;

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers * 3 + l_maxMaterialTextures * 2 + l_maxRenderTargetTextures * 2;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, true);
        m_CSUDescHeap = CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderVisible");
        auto l_incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;
        auto l_RenderTargetTextureDescriptorSectionSize = l_maxRenderTargetTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferCBVHandle = {};
        DescriptorHandle l_firstGPUBufferSRVHandle = {};
        DescriptorHandle l_firstGPUBufferUAVHandle = {};

        {
            l_firstGPUBufferCBVHandle.m_CPUHandle = m_CSUDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            l_firstGPUBufferCBVHandle.m_GPUHandle = m_CSUDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

            m_GPUBuffer_CBV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferCBVHandle, true, L"GPUBuffer_CBV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferCBVHandle;
            D3D12_CONSTANT_BUFFER_VIEW_DESC l_CBVDesc = {};
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_device->CreateConstantBufferView(nullptr, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferSRVHandle.m_CPUHandle = l_firstGPUBufferCBVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferSRVHandle.m_GPUHandle = l_firstGPUBufferCBVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_GPUBuffer_SRV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferSRVHandle, true, L"GPUBuffer_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            l_SRVDesc.Format = DXGI_FORMAT_R32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }            
        }

        {
            l_firstGPUBufferUAVHandle.m_CPUHandle = l_firstGPUBufferSRVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferUAVHandle.m_GPUHandle = l_firstGPUBufferSRVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_GPUBuffer_UAV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, true, L"GPUBuffer_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_SINT;            
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }                
        }

        DescriptorHandle l_firstMaterialTextureSRVHandle = {};
        DescriptorHandle l_firstMaterialTextureUAVHandle = {};
        {
            l_firstMaterialTextureSRVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstMaterialTextureSRVHandle.m_GPUHandle = l_firstGPUBufferUAVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_MaterialTexture_SRV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureSRVHandle, true, L"MaterialTexture_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstMaterialTextureUAVHandle.m_CPUHandle = l_firstMaterialTextureSRVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstMaterialTextureUAVHandle.m_GPUHandle = l_firstMaterialTextureSRVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_MaterialTexture_UAV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureUAVHandle, true, L"MaterialTexture_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstRenderTargetTextureSRVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureUAVHandle = {};
        {
            l_firstRenderTargetTextureSRVHandle.m_CPUHandle = l_firstMaterialTextureUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstRenderTargetTextureSRVHandle.m_GPUHandle = l_firstMaterialTextureUAVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_RenderTarget_SRV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureSRVHandle, true, L"RenderTarget_SRV_DescHeapAccessor");
            
            auto l_currentHandle = l_firstRenderTargetTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;            
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstRenderTargetTextureUAVHandle.m_CPUHandle = l_firstRenderTargetTextureSRVHandle.m_CPUHandle + l_RenderTargetTextureDescriptorSectionSize;
            l_firstRenderTargetTextureUAVHandle.m_GPUHandle = l_firstRenderTargetTextureSRVHandle.m_GPUHandle + l_RenderTargetTextureDescriptorSectionSize;

            m_RenderTarget_UAV_DescHeapAccessor = CreateDescriptorHeapAccessor(m_CSUDescHeap.Get(), l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureUAVHandle, true, L"RenderTarget_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;            
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }
    }

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers + l_maxMaterialTextures + l_maxRenderTargetTextures;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, false);
        m_CSUDescHeap_ShaderNonVisible = CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderNonVisible");

        auto l_incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferUAVHandle = {};
        DescriptorHandle l_firstMaterialTextureBufferUAVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureBufferUAVHandle = {};

        l_firstGPUBufferUAVHandle.m_CPUHandle = m_CSUDescHeap_ShaderNonVisible->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstMaterialTextureBufferUAVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
        l_firstRenderTargetTextureBufferUAVHandle.m_CPUHandle = l_firstMaterialTextureBufferUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;

        {
            m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible = CreateDescriptorHeapAccessor(m_CSUDescHeap_ShaderNonVisible.Get(), l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, false, L"GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_UINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible = CreateDescriptorHeapAccessor(m_CSUDescHeap_ShaderNonVisible.Get(), l_Desc, l_maxMaterialTextures
                , l_incrementSize, l_firstMaterialTextureBufferUAVHandle, false, L"MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstMaterialTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible = CreateDescriptorHeapAccessor(m_CSUDescHeap_ShaderNonVisible.Get(), l_Desc, l_maxRenderTargetTextures
                , l_incrementSize, l_firstRenderTargetTextureBufferUAVHandle, false, L"RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstRenderTargetTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }
    }

    {
        uint32_t l_maxSamplerCount = 128;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, l_maxSamplerCount, true);
        m_SamplerDescHeap = CreateDescriptorHeap(l_Desc, L"GlobalSamplerDescHeap");

        DescriptorHandle l_firstSamplerHandle = {};
        l_firstSamplerHandle.m_CPUHandle = m_SamplerDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstSamplerHandle.m_GPUHandle = m_SamplerDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

        m_SamplerDescHeapAccessor = CreateDescriptorHeapAccessor(m_SamplerDescHeap.Get(), l_Desc, l_maxSamplerCount, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER), l_firstSamplerHandle, true, L"SamplerDescHeapAccessor");
    }

    {
        uint32_t l_maxRTVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, l_maxRTVCount, false);
        m_RTVDescHeap = CreateDescriptorHeap(l_Desc, L"GlobalRTVDescHeap");

        DescriptorHandle l_firstRTVHandle = {};
        l_firstRTVHandle.m_CPUHandle = m_RTVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_RTVDescHeapAccessor = CreateDescriptorHeapAccessor(m_RTVDescHeap.Get(), l_Desc, l_maxRTVCount, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), l_firstRTVHandle, false, L"RTVDescHeapAccessor");
    }

    {
        uint32_t l_maxDSVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, l_maxDSVCount, false);
        m_DSVDescHeap = CreateDescriptorHeap(l_Desc, L"GlobalDSVDescHeap");

        DescriptorHandle l_firstDSVHandle = {};
        l_firstDSVHandle.m_CPUHandle = m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DSVDescHeapAccessor = CreateDescriptorHeapAccessor(m_DSVDescHeap.Get(), l_Desc, l_maxDSVCount, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), l_firstDSVHandle, false, L"DSVDescHeapAccessor");
    }

    return true;
}

bool DX12RenderingServer::CreateMipmapGenerator()
{
    {
        CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
        uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
        rootParameters[0].InitAsConstants(3, 0);
        rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
        rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

        ID3DBlob* signature;
        ID3DBlob* error;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_3DMipmapRootSignature));

        ShaderFilePath l_3DPath = "mipmapGenerator3D.comp/";

        D3D12_COMPUTE_PIPELINE_STATE_DESC l_3DPSODesc = {};
        l_3DPSODesc.pRootSignature = m_3DMipmapRootSignature;

#ifdef USE_DXIL
        std::vector<uint8_t> l_3DmipmapComputeShader;
        LoadShaderFile(l_3DmipmapComputeShader, l_3DPath);
        l_3DPSODesc.CS = { l_3DmipmapComputeShader.data(), l_3DmipmapComputeShader.size() };
#else
        ID3DBlob* l_3DmipmapComputeShader;
        LoadShaderFile(&l_3DmipmapComputeShader, ShaderStage::Compute, l_3DPath);
        l_3DPSODesc.CS = { reinterpret_cast<UINT8*>(l_3DmipmapComputeShader->GetBufferPointer()), l_3DmipmapComputeShader->GetBufferSize() };
#endif
        m_device->CreateComputePipelineState(&l_3DPSODesc, IID_PPV_ARGS(&m_3DMipmapPSO));

        Log(Success, "Mipmap generator for 3D texture has been created.");
    }
    {
        CD3DX12_DESCRIPTOR_RANGE uavRanges[2];
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        uavRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);  // Source UAV
        uavRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);  // Destination UAV
        rootParameters[0].InitAsConstants(2, 0);
        rootParameters[1].InitAsDescriptorTable(1, &uavRanges[0]);
        rootParameters[2].InitAsDescriptorTable(1, &uavRanges[1]);

        ID3DBlob* signature;
        ID3DBlob* error;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_2DMipmapRootSignature));

        ShaderFilePath l_2DPath = "mipmapGenerator2D.comp/";
        D3D12_COMPUTE_PIPELINE_STATE_DESC l_2DPSODesc = {};
        l_2DPSODesc.pRootSignature = m_2DMipmapRootSignature;

#ifdef USE_DXIL
        std::vector<uint8_t> l_2DmipmapComputeShader;
        LoadShaderFile(l_2DmipmapComputeShader, l_2DPath);
        l_2DPSODesc.CS = { l_2DmipmapComputeShader.data(), l_2DmipmapComputeShader.size() };
#else
        ID3DBlob* l_2DmipmapComputeShader;
        LoadShaderFile(&l_2DmipmapComputeShader, ShaderStage::Compute, l_2DPath);
        l_2DPSODesc.CS = { reinterpret_cast<UINT8*>(l_2DmipmapComputeShader->GetBufferPointer()), l_2DmipmapComputeShader->GetBufferSize() };
#endif
        m_device->CreateComputePipelineState(&l_2DPSODesc, IID_PPV_ARGS(&m_2DMipmapPSO));

        Log(Success, "Mipmap generator for 2D texture has been created.");
    }

    return true;
}

bool DX12RenderingServer::CreateSwapChain()
{
    // Set the swap chain to use multi-buffering.
    m_swapChainDesc.BufferCount = m_swapChainImageCount;

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    // Set the width and height of the back buffer.
    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    // Set regular 32-bit surface for the back buffer.
    m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Set the usage of the back buffer.
    m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

    // Turn multisampling off.
    m_swapChainDesc.SampleDesc.Count = 1;
    m_swapChainDesc.SampleDesc.Quality = 0;

    // Set to full screen or windowed mode.
    // @TODO: finish this feature

    m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // Don't set the advanced flags.
    m_swapChainDesc.Flags = 0;

    m_swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    // Finally create the swap chain
    IDXGISwapChain1* l_swapChain1;
    auto l_hResult = m_factory->CreateSwapChainForHwnd(
        m_directCommandQueue.Get(),
        reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle(),
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
