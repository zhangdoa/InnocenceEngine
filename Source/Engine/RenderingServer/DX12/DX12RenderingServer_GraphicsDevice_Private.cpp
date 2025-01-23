#include "DX12RenderingServer.h"

#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../../Services/RenderingConfigurationService.h"

using namespace Inno;
#include "../../Engine.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"

using namespace DX12Helper;

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
    // m_debugInterface->SetEnableGPUBasedValidation(true);

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
#ifdef INNO_DEBUG
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

    // Release the display mode list.
    // displayModeList.clear();

    // Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
    // Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
    auto featureLevel = D3D_FEATURE_LEVEL_12_1;

    // Create the Direct3D 12 device.
    l_HResult = D3D12CreateDevice(m_adapter.Get(), featureLevel, IID_PPV_ARGS(&m_device));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create a DirectX 12.1 device. The default video card does not support DirectX 12.1!");
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

    l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    // l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

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
    m_copyCommandAllocator = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, L"CopyCommandAllocator");

    m_directCommandAllocators.resize(m_swapChainImageCount);
    m_computeCommandAllocators.resize(m_swapChainImageCount);
    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
        m_computeCommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
    }

    Log(Success, "Global CommandAllocators have been created.");

    m_GlobalCommandLists.resize(m_swapChainImageCount);
    for (size_t i = 0; i < m_GlobalCommandLists.size(); i++)
    {
        m_GlobalCommandLists[i] = static_cast<DX12CommandList*>(AddCommandList());
        CreateCommandList(m_GlobalCommandLists[i], i, L"GPUBufferCommandList");
    }

    Log(Success, "Global CommandLists have been created.");

    return true;
}

bool DX12RenderingServer::CreateSyncPrimitives()
{
    m_directCommandQueueFence.resize(m_swapChainImageCount);
    m_computeCommandQueueFence.resize(m_swapChainImageCount);
    m_copyCommandQueueFence.resize(m_swapChainImageCount);
    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_directCommandQueueFence[i]))))
        {
            Log(Error, "Can't create Fence for direct CommandQueue!");
            return false;
        }
        if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeCommandQueueFence[i]))))
        {
            Log(Error, "Can't create Fence for compute CommandQueue!");
            return false;
        }
        if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_copyCommandQueueFence[i]))))
        {
            Log(Error, "Can't create Fence for copy CommandQueue!");
            return false;
        }
#ifdef INNO_DEBUG
        m_directCommandQueueFence[i]->SetName((L"DirectCommandQueueFence_" + std::to_wstring(i)).c_str());
        m_computeCommandQueueFence[i]->SetName((L"ComputeCommandQueueFence_" + std::to_wstring(i)).c_str());
        m_copyCommandQueueFence[i]->SetName((L"CopyCommandQueueFence_" + std::to_wstring(i)).c_str());
#endif // INNO_DEBUG
    }
    Log(Verbose, "Fences for global CommandQueues have been created.");

    m_directCommandQueueSemaphore.resize(m_swapChainImageCount, std::atomic<uint64_t>(0));
    m_computeCommandQueueSemaphore.resize(m_swapChainImageCount, std::atomic<uint64_t>(0));
    m_copyCommandQueueSemaphore.resize(m_swapChainImageCount, std::atomic<uint64_t>(0));

    m_directCommandQueueFence.resize(m_swapChainImageCount);
    m_computeCommandQueueFence.resize(m_swapChainImageCount);
    m_copyCommandQueueFence.resize(m_swapChainImageCount);

    m_directCommandQueueFenceEvent.resize(m_swapChainImageCount, std::atomic<HANDLE>(nullptr));
    m_computeCommandQueueFenceEvent.resize(m_swapChainImageCount, std::atomic<HANDLE>(nullptr));
    
    return true;
}

bool DX12RenderingServer::CreateGlobalDescriptorHeaps()
{
    m_CSUDescHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxDescriptorCount, L"GlobalCSUDescHeap_ShaderVisible");
    m_ShaderNonVisibleCSUDescHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxDescriptorCount, L"GlobalCSUDescHeap_ShaderNonVisible", false);
    m_SamplerDescHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128, L"GlobalSamplerDescHeap");
    m_RTVDescHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256, L"GlobalRTVDescHeap", false);
    m_DSVDescHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256, L"GlobalDSVDescHeap", false);

    return true;
}

bool DX12RenderingServer::CreateMipmapGenerator()
{
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    {
        CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
        srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
        rootParameters[0].InitAsConstants(3, 0);
        rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
        rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

        ID3DBlob* signature;
        ID3DBlob* error;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_3DMipmapRootSignature));

        ShaderFilePath l_3DPath = "mipmapGenerator3D.comp/";

        D3D12_COMPUTE_PIPELINE_STATE_DESC l_3DPSODesc = {};
        l_3DPSODesc.pRootSignature = m_3DMipmapRootSignature;

#ifdef USE_DXIL
        std::vector<char> l_3DmipmapComputeShader;
        LoadShaderFile(l_3DmipmapComputeShader, l_3DPath);
        l_3DPSODesc.CS = { &l_3DmipmapComputeShader[0], l_3DmipmapComputeShader.size() };
#else
        ID3DBlob* l_3DmipmapComputeShader;
        LoadShaderFile(&l_3DmipmapComputeShader, ShaderStage::Compute, l_3DPath);
        l_3DPSODesc.CS = { reinterpret_cast<UINT8*>(l_3DmipmapComputeShader->GetBufferPointer()), l_3DmipmapComputeShader->GetBufferSize() };
#endif
        m_device->CreateComputePipelineState(&l_3DPSODesc, IID_PPV_ARGS(&m_3DMipmapPSO));

        Log(Success, "Mipmap generator for 3D texture has been created.");
    }
    {
        CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
        srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
        rootParameters[0].InitAsConstants(2, 0);
        rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
        rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

        ID3DBlob* signature;
        ID3DBlob* error;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_2DMipmapRootSignature));

        ShaderFilePath l_2DPath = "mipmapGenerator2D.comp/";
        D3D12_COMPUTE_PIPELINE_STATE_DESC l_2DPSODesc = {};
        l_2DPSODesc.pRootSignature = m_2DMipmapRootSignature;

#ifdef USE_DXIL
        std::vector<char> l_2DmipmapComputeShader;
        LoadShaderFile(l_2DmipmapComputeShader, l_2DPath);
        l_2DPSODesc.CS = { &l_2DmipmapComputeShader[0], l_2DmipmapComputeShader.size() };
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
    // Set the swap chain to use double buffering.
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

    // Discard the back buffer contents after presenting.
    m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

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

    if (FAILED(l_hResult))
    {
        Log(Error, "Can't create swap chain!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    Log(Success, "Swap chain has been created.");

    return true;
}
