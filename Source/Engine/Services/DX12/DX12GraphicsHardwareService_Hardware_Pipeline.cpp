#include "DX12GraphicsHardwareService.h"
#include "../FrameManagementService.h"
#include "../RenderPassResourceService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Inno;

bool DX12GraphicsHardwareService::CreateGlobalCommandQueues()
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

bool DX12GraphicsHardwareService::CreateGlobalCommandAllocators()
{
    auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
    m_DX12Context.m_directCommandAllocators.resize(l_swapChainImageCount);
    m_DX12Context.m_computeCommandAllocators.resize(l_swapChainImageCount);
    m_DX12Context.m_copyCommandAllocators.resize(l_swapChainImageCount);
    for (size_t i = 0; i < l_swapChainImageCount; i++)
    {
        m_DX12Context.m_directCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_computeCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_copyCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, (L"CopyCommandAllocator_" + std::to_wstring(i)).c_str());
    }

    Log(Success, "Global CommandAllocators have been created.");

    return true;
}

bool DX12GraphicsHardwareService::CreateSyncPrimitives()
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
#endif

    Log(Verbose, "Fences for global CommandQueues have been created.");

    auto l_GlobalSemaphore = static_cast<DX12Semaphore*>(g_Engine->Get<RenderPassResourceService>()->AddSemaphore());
    l_GlobalSemaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    m_FrameManagementService->SetGlobalSemaphore(l_GlobalSemaphore);

    Log(Verbose, "Fence events for global CommandQueues have been created.");

    return true;
}
