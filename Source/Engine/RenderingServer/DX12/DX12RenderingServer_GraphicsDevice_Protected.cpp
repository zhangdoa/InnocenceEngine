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

    m_TLASBufferComponent = AddGPUBufferComponent("TLASBuffer/");
    m_TLASBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
    // @TODO: The number of elements should be calculated based on the number of ray tracing instances.
    m_TLASBufferComponent->m_Usage = GPUBufferUsage::TLAS;
    m_TLASBufferComponent->m_ElementCount = 256; 

    InitializeImpl(m_TLASBufferComponent);

    m_ScratchBufferComponent = AddGPUBufferComponent("ScratchBuffer/");
    m_ScratchBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
    m_ScratchBufferComponent->m_Usage = GPUBufferUsage::ScratchBuffer;
    m_ScratchBufferComponent->m_ElementCount = 256; 

    InitializeImpl(m_ScratchBufferComponent);

    m_RaytracingInstanceBufferComponent = AddGPUBufferComponent("RaytracingInstanceBuffer/");
    m_RaytracingInstanceBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
    m_RaytracingInstanceBufferComponent->m_ElementCount = 256;
    m_RaytracingInstanceBufferComponent->m_ElementSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    InitializeImpl(m_RaytracingInstanceBufferComponent);

    m_RaytracingInstanceDescs.resize(GetSwapChainImageCount());
    for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
    {
        auto l_descList = new DX12RaytracingInstanceDescList();
        l_descList->m_Descs.reserve(m_RaytracingInstanceBufferComponent->m_ElementCount);
        m_RaytracingInstanceDescs[i] = l_descList;
    }

    return l_result;
}

bool DX12RenderingServer::ReleaseHardwareResources()
{
    Delete(m_RaytracingInstanceBufferComponent);
    Delete(m_ScratchBufferComponent);
    Delete(m_TLASBufferComponent);

    m_3DMipmapPSO->Release();
    m_2DMipmapPSO->Release();
    m_3DMipmapRootSignature->Release();
    m_2DMipmapRootSignature->Release();

    m_SamplerDescHeapAccessor.m_Heap = nullptr;
    m_SamplerDescHeap = nullptr;

    m_DSVDescHeapAccessor.m_Heap = nullptr;
    m_DSVDescHeap = nullptr;

    m_RTVDescHeapAccessor.m_Heap = nullptr;
    m_RTVDescHeap = nullptr;

    m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible.m_Heap = nullptr;
    m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible.m_Heap = nullptr;
    m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible.m_Heap = nullptr;
    m_CSUDescHeap_ShaderNonVisible = nullptr;

    m_RenderTarget_UAV_DescHeapAccessor.m_Heap = nullptr;
    m_MaterialTexture_UAV_DescHeapAccessor.m_Heap = nullptr;
    m_GPUBuffer_UAV_DescHeapAccessor.m_Heap = nullptr;
    m_RenderTarget_SRV_DescHeapAccessor.m_Heap = nullptr;
    m_MaterialTexture_SRV_DescHeapAccessor.m_Heap = nullptr;
    m_GPUBuffer_SRV_DescHeapAccessor.m_Heap = nullptr;
    m_GPUBuffer_CBV_DescHeapAccessor.m_Heap = nullptr;
    m_CSUDescHeap = nullptr;

    m_directCommandQueueFence = nullptr;
    m_computeCommandQueueFence = nullptr;
    m_copyCommandQueueFence = nullptr;

    for (size_t i = 0; i < m_GlobalCommandLists.size(); i++)
    {
        auto l_commandList = reinterpret_cast<DX12CommandList*>(m_GlobalCommandLists[i]);
        Delete(l_commandList);
    }

    m_GlobalCommandLists.clear();

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        m_directCommandAllocators[i]->Release();
        m_computeCommandAllocators[i]->Release();
        m_copyCommandAllocators[i]->Release();
    }
    m_directCommandAllocators.clear();
    m_computeCommandAllocators.clear();
    m_copyCommandAllocators.clear();

    m_directCommandQueue = nullptr;
    m_computeCommandQueue = nullptr;
    m_copyCommandQueue = nullptr;

    m_swapChain = nullptr;

    m_device = nullptr;

    m_adapterOutput = nullptr;

    m_adapter = nullptr;

    m_factory = nullptr;
    
    m_graphicsAnalysis = nullptr;

    m_debugInterface = nullptr;
    
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
    l_result &= CreateRootSignature(rhs);

    auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(rhs->m_PipelineStateObject);
    if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
    {
        if (rhs->m_RenderPassDesc.m_UseOutputMerger)
        {
            l_result &= CreateGraphicsPipelineStateObject(rhs, l_PSO);
        }
    }
    else if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute && !rhs->m_RenderPassDesc.m_UseRaytracing)
    {
        LoadComputeShaders(rhs);

        l_PSO->m_ComputePSODesc.pRootSignature = l_PSO->m_RootSignature.Get();
        auto l_HResult = m_device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

        if (FAILED(l_HResult))
        {
            Log(Error, rhs->m_InstanceName, " Can't create Compute PSO.");
            return false;
        }
    }

    if (l_PSO->m_PSO)
    {
#ifdef INNO_DEBUG
        SetObjectName(rhs, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG
        Log(Verbose, rhs->m_InstanceName, " PSO has been created.");
    }

    if (rhs->m_RenderPassDesc.m_UseRaytracing)
    {
        l_result &= CreateRaytracingPipelineStateObject(rhs, l_PSO);
    }

    return l_result;
}

bool DX12RenderingServer::CreateGraphicsPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
    GenerateDepthStencilStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, PSO);
    GenerateBlendStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, PSO);
    GenerateRasterizerStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, PSO);
    GenerateViewportStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, PSO);

    PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

    auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
    auto l_RTV = l_DX12OutputMergerTarget->m_RTVs[0];
    for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
    {
        PSO->m_GraphicsPSODesc.RTVFormats[i] = l_RTV.m_Desc.Format;
    }

    if (RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
    {
        auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
        auto l_DSV = l_DX12OutputMergerTarget->m_DSVs[0];
        PSO->m_GraphicsPSODesc.DSVFormat = l_DSV.m_Desc.Format;
        PSO->m_GraphicsPSODesc.DepthStencilState = PSO->m_DepthStencilDesc;
    }

    PSO->m_GraphicsPSODesc.RasterizerState = PSO->m_RasterizerDesc;
    PSO->m_GraphicsPSODesc.BlendState = PSO->m_BlendDesc;
    PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
    PSO->m_GraphicsPSODesc.PrimitiveTopologyType = PSO->m_PrimitiveTopologyType;
    PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;
    if (!PSO->m_RootSignature.Get())
    {
        Log(Verbose, "Skipping creating Graphics PSO for ", RenderPassComp->m_InstanceName);
        return true;
    }

    PSO->m_GraphicsPSODesc.pRootSignature = PSO->m_RootSignature.Get();

    CreateInputLayout(PSO);
    LoadGraphicsShaders(RenderPassComp);

    auto l_HResult = m_device->CreateGraphicsPipelineState(&PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&PSO->m_PSO));
    if (FAILED(l_HResult))
    {
        Log(Error, RenderPassComp->m_InstanceName, " Can't create Graphics PSO.");
        return false;
    }

    return true;
}

bool DX12RenderingServer::CreateRaytracingPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
    auto l_DX12SPC = reinterpret_cast<DX12ShaderProgramComponent*>(RenderPassComp->m_ShaderProgram);
    
    if (!PSO->m_RootSignature)
    {
        Log(Error, RenderPassComp->m_InstanceName, " Global root signature is null!");
        return false;
    }

    LoadRaytracingShaders(RenderPassComp);

    D3D12_DXIL_LIBRARY_DESC rayGenLib = {};
    rayGenLib.DXILLibrary.pShaderBytecode = &l_DX12SPC->m_RayGenBuffer[0];
    rayGenLib.DXILLibrary.BytecodeLength = l_DX12SPC->m_RayGenBuffer.size();

    D3D12_DXIL_LIBRARY_DESC closestHitLib = {};
    closestHitLib.DXILLibrary.pShaderBytecode = &l_DX12SPC->m_ClosestHitBuffer[0];
    closestHitLib.DXILLibrary.BytecodeLength = l_DX12SPC->m_ClosestHitBuffer.size();

    D3D12_DXIL_LIBRARY_DESC anyHitLib = {};
    anyHitLib.DXILLibrary.pShaderBytecode = &l_DX12SPC->m_AnyHitBuffer[0];
    anyHitLib.DXILLibrary.BytecodeLength = l_DX12SPC->m_AnyHitBuffer.size();

    D3D12_DXIL_LIBRARY_DESC missLib = {};
    missLib.DXILLibrary.pShaderBytecode = &l_DX12SPC->m_MissBuffer[0];
    missLib.DXILLibrary.BytecodeLength = l_DX12SPC->m_MissBuffer.size();

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = L"HitGroup";  // Name for your hit group.
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
    hitGroupDesc.AnyHitShaderImport = L"AnyHitShader";
    hitGroupDesc.IntersectionShaderImport = nullptr; // For triangle geometry.

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = 32;  // Adjust to your needs.
    shaderConfig.MaxAttributeSizeInBytes = 8; // For example, 2 floats for barycentrics.

    D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { PSO->m_RootSignature.Get() };

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
    pipelineCfg.MaxTraceRecursionDepth = 1; // Ray generation, no recursion.

    D3D12_STATE_SUBOBJECT subobjects[8] = {};
    subobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[0].pDesc = &rayGenLib;

    subobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[1].pDesc = &closestHitLib;

    subobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[2].pDesc = &anyHitLib;

    subobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[3].pDesc = &missLib;

    subobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    subobjects[4].pDesc = &hitGroupDesc;

    subobjects[5].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    subobjects[5].pDesc = &shaderConfig;

    subobjects[6].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    subobjects[6].pDesc = &globalSig;

    subobjects[7].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    subobjects[7].pDesc = &pipelineCfg;

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = ARRAYSIZE(subobjects);
    stateObjectDesc.pSubobjects = subobjects;

    // Create the state object.
    HRESULT l_HResult = m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&PSO->m_RaytracingPSO));
    if (FAILED(l_HResult))
    {
        Log(Error, RenderPassComp->m_InstanceName, " Can't create Raytracing PSO.");
        return false;
    }

#ifdef INNO_DEBUG
    SetObjectName(RenderPassComp, PSO->m_RaytracingPSO, "RaytracingPSO");
#endif // INNO_DEBUG

    Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing PSO has been created.");

    auto l_shaderIDBufferSize = 3 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    auto l_shaderIDBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(l_shaderIDBufferSize);
    PSO->m_RaytracingShaderIDBuffer = CreateUploadHeapBuffer(&l_shaderIDBufferDesc);

    ID3D12StateObjectProperties* props;
    PSO->m_RaytracingPSO->QueryInterface(&props);

    void* data;
    auto writeId = [&](const wchar_t* name) {
        void* id = props->GetShaderIdentifier(name);
        memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        data = static_cast<char*>(data) +
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        };

    PSO->m_RaytracingShaderIDBuffer->Map(0, nullptr, &data);
    writeId(L"RayGenShader");
    writeId(L"MissShader");
    writeId(L"HitGroup");
    PSO->m_RaytracingShaderIDBuffer->Unmap(0, nullptr);
    props->Release();

    Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing shader IDs have been written.");

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

bool DX12RenderingServer::PrepareRayTracing(ICommandList* commandList)
{
    auto l_currentFrame = GetCurrentFrame();
    auto l_instanceDescList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[l_currentFrame]);
    
    // Early exit if no instances or TLAS not ready
    if (l_instanceDescList->m_Descs.size() == 0)
        return true;
    
    if (m_TLASBufferComponent->m_ObjectStatus != ObjectStatus::Activated)
    {
        Log(Warning, "TLAS buffer not activated - skipping TLAS build");
        return true;
    }
    
    // Upload instance data to GPU
    auto l_mappedMemory = m_RaytracingInstanceBufferComponent->m_MappedMemories[l_currentFrame];
    WriteMappedMemory(m_RaytracingInstanceBufferComponent, l_mappedMemory, &l_instanceDescList->m_Descs[0], 0, l_instanceDescList->m_Descs.size());
    l_mappedMemory->m_NeedUploadToGPU = false;
    UploadToGPU(commandList, m_RaytracingInstanceBufferComponent);
    
    // Get buffer references
    auto l_TLASBuffer = reinterpret_cast<DX12DeviceMemory*>(m_TLASBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_scratchBuffer = reinterpret_cast<DX12DeviceMemory*>(m_ScratchBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_instanceBuffer = reinterpret_cast<DX12DeviceMemory*>(m_RaytracingInstanceBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
    
    // Transition instance buffer for reading
    CD3DX12_RESOURCE_BARRIER instanceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
    l_commandList->m_DirectCommandList->ResourceBarrier(1, &instanceBarrier);
    
    // Setup TLAS build description - always full rebuild for stability
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
    tlasDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasDesc.Inputs.NumDescs = l_instanceDescList->m_Descs.size();
    tlasDesc.Inputs.InstanceDescs = l_instanceBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();
    tlasDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    tlasDesc.SourceAccelerationStructureData = 0; // Full rebuild
    tlasDesc.DestAccelerationStructureData = l_TLASBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData = l_scratchBuffer->m_DefaultHeapBuffer->GetGPUVirtualAddress();
    
    // Build the TLAS
    l_commandList->m_DirectCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    
    // Add UAV barrier for TLAS completion
    CD3DX12_RESOURCE_BARRIER tlasBarrier = CD3DX12_RESOURCE_BARRIER::UAV(l_TLASBuffer->m_DefaultHeapBuffer.Get());
    l_commandList->m_DirectCommandList->ResourceBarrier(1, &tlasBarrier);
    
    // Transition instance buffer back to writable state
    CD3DX12_RESOURCE_BARRIER instanceBarrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    l_commandList->m_DirectCommandList->ResourceBarrier(1, &instanceBarrierBack);
    
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
    Log(Verbose, "Resizing the swap chain...");

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

bool DX12RenderingServer::OnSceneLoadingStart()
{
    for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
    {
        auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
        l_descList->m_Descs.clear();
    }

    m_RegisteredCollisionComponents.clear();
    
    Log(Verbose, "Raytracing instance descriptions have been cleared.");

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