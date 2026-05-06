#include "DX12FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../CommandListResourceService.h"
#include "../GPUBufferResourceService.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12FrameManagementService::BeginFrame()
{
    auto l_currentFrame = m_CurrentFrame;

    // Precondition enforced here, not derived from caller ordering: the per-queue fence
    // values stored for this frame slot must be reached before the matching allocator is
    // safe to Reset. WaitOnCPU is idempotent when the fence is already past.
    m_HardwareService->WaitOnCPU(m_GraphicsSemaphoreValues[l_currentFrame], GPUEngineType::Graphics);
    m_HardwareService->WaitOnCPU(m_ComputeSemaphoreValues[l_currentFrame], GPUEngineType::Compute);
    m_HardwareService->WaitOnCPU(m_CopySemaphoreValues[l_currentFrame], GPUEngineType::Copy);

    if (FAILED(m_ctx->m_directCommandAllocators[l_currentFrame]->Reset()))
    {
        Log(Error, "DX12FrameManagementService::BeginFrame: direct command allocator Reset failed for frame ", l_currentFrame);
        return false;
    }
    if (FAILED(m_ctx->m_computeCommandAllocators[l_currentFrame]->Reset()))
    {
        Log(Error, "DX12FrameManagementService::BeginFrame: compute command allocator Reset failed for frame ", l_currentFrame);
        return false;
    }
    if (FAILED(m_ctx->m_copyCommandAllocators[l_currentFrame]->Reset()))
    {
        Log(Error, "DX12FrameManagementService::BeginFrame: copy command allocator Reset failed for frame ", l_currentFrame);
        return false;
    }

    g_Engine->Get<CommandListResourceService>()->ForEach([this](CommandListComponent* cl)
    {
        if (cl && cl->m_ObjectStatus == ObjectStatus::Activated && cl->m_CommandList)
        {
            Open(cl, cl->m_Type, nullptr);
            Close(cl, cl->m_Type);
        }
    });

    return true;
}

bool DX12FrameManagementService::PrepareRayTracing(CommandListComponent* commandList)
{
    auto l_currentFrame = m_CurrentFrame;
    auto& l_raytracingInstanceDescs = g_Engine->Get<GPUBufferResourceService>()->GetRaytracingInstanceDescs();
    auto l_instanceDescList = reinterpret_cast<DX12RaytracingInstanceDescList*>(l_raytracingInstanceDescs[l_currentFrame]);

    if (l_instanceDescList->m_Descs.size() == 0)
        return true;

    auto l_TLASBufferComponent = g_Engine->Get<GPUBufferResourceService>()->GetTLASBufferComponent();
    if (l_TLASBufferComponent->m_ObjectStatus != ObjectStatus::Activated)
    {
        Log(Warning, "TLAS buffer not activated - skipping TLAS build");
        return true;
    }

    auto l_RaytracingInstanceBufferComponent = g_Engine->Get<GPUBufferResourceService>()->GetRaytracingInstanceBufferComponent();
    auto l_mappedMemory = l_RaytracingInstanceBufferComponent->m_MappedMemories[l_currentFrame];
    g_Engine->Get<GPUBufferResourceService>()->WriteMappedMemory(l_RaytracingInstanceBufferComponent, l_mappedMemory, &l_instanceDescList->m_Descs[0], 0, l_instanceDescList->m_Descs.size());
    l_mappedMemory->m_NeedUploadToGPU = false;

    auto l_instanceBuffer = reinterpret_cast<DX12DeviceMemory*>(l_RaytracingInstanceBufferComponent->m_DeviceMemories[l_currentFrame]);
    auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

    auto instanceBarrier_UploadToDefaultHeap = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    l_commandList->ResourceBarrier(1, &instanceBarrier_UploadToDefaultHeap);

    g_Engine->Get<GPUBufferResourceService>()->UploadToGPU(commandList, l_RaytracingInstanceBufferComponent);

    auto instanceBarrierTLASBuild = CD3DX12_RESOURCE_BARRIER::Transition(
        l_instanceBuffer->m_DefaultHeapBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
    l_commandList->ResourceBarrier(1, &instanceBarrierTLASBuild);

    auto l_ScratchBufferComponent = g_Engine->Get<GPUBufferResourceService>()->GetScratchBufferComponent();
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

    g_Engine->Get<GPUBufferResourceService>()->SetTLASReady(true);

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

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
    Log(Success, "DX12FrameManagementService::ResizeImpl: ",
        l_screenResolution.x, "x", l_screenResolution.y);

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainImages.clear();

    auto l_hResult = m_swapChain->ResizeBuffers(
        m_swapChainImageCount,
        m_swapChainDesc.Width,
        m_swapChainDesc.Height,
        m_swapChainDesc.Format,
        0);

    if (FAILED(l_hResult))
    {
        Log(Error, "DX12FrameManagementService::ResizeImpl: ResizeBuffers failed, HRESULT=", static_cast<int32_t>(l_hResult));
        return false;
    }

    Log(Success, "DX12FrameManagementService::ResizeImpl: ResizeBuffers succeeded.");

    GetSwapChainImages();

    return true;
}

bool DX12FrameManagementService::WaitAllOnCPU()
{
    auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
    if (!l_semaphore)
    {
        Log(Error, "DX12FrameManagementService::WaitAllOnCPU: global semaphore is null");
        return false;
    }

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
    l_result &= waitFence(m_ctx->m_directCommandQueueFence, m_ctx->m_directCommandQueue, l_semaphore->m_DirectCommandQueueFenceEvent);
    l_result &= waitFence(m_ctx->m_computeCommandQueueFence, m_ctx->m_computeCommandQueue, l_semaphore->m_ComputeCommandQueueFenceEvent);
    l_result &= waitFence(m_ctx->m_copyCommandQueueFence, m_ctx->m_copyCommandQueue, l_semaphore->m_CopyCommandQueueFenceEvent);

    return l_result;
}
