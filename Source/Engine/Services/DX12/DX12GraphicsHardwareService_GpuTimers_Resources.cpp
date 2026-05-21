#include "DX12GraphicsHardwareService.h"
#include "DX12GraphicsHardwareService_Internal.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::CreateGpuTimerResources()
{
	if (m_DX12Context.m_device == nullptr)
	{
		Log(Error, "CreateGpuTimerResources: device is null.");
		return false;
	}
	if (m_FrameManagementService == nullptr)
	{
		Log(Error, "CreateGpuTimerResources: FrameManagementService not wired.");
		return false;
	}

	auto l_swapChainCount = m_FrameManagementService->GetSwapChainImageCount();
	if (l_swapChainCount == 0)
	{
		Log(Error, "CreateGpuTimerResources: swap-chain image count is 0.");
		return false;
	}

	auto l_createForQueue = [this, l_swapChainCount](
		GPUEngineType queueType,
		D3D12_QUERY_HEAP_TYPE heapType,
		D3D12_COMMAND_LIST_TYPE cmdListType,
		ComPtr<ID3D12QueryHeap>& outHeap,
		std::vector<ComPtr<ID3D12Resource>>& outReadback,
		std::vector<ComPtr<ID3D12CommandAllocator>>& outAllocators,
		std::vector<ComPtr<ID3D12GraphicsCommandList7>>& outLists,
		const wchar_t* heapName,
		const char* readbackName,
		const wchar_t* allocatorNameStem,
		const wchar_t* listNameStem)
	{
		D3D12_QUERY_HEAP_DESC l_heapDesc = {};
		l_heapDesc.Type = heapType;
		l_heapDesc.Count = GPU_TIMER_TOTAL_QUERIES_PER_QUEUE;
		l_heapDesc.NodeMask = 0;
		auto l_HResult = m_DX12Context.m_device->CreateQueryHeap(&l_heapDesc, IID_PPV_ARGS(&outHeap));
		if (FAILED(l_HResult))
		{
			LogD3D12CreateFailure(m_DX12Context.m_device.Get(), "QueryHeap (timestamp)", heapName, l_HResult);
			return false;
		}
		outHeap->SetName(heapName);

		outReadback.clear();
		outReadback.resize(l_swapChainCount);
		outAllocators.clear();
		outAllocators.resize(l_swapChainCount);
		outLists.clear();
		outLists.resize(l_swapChainCount);
		for (uint32_t i = 0; i < l_swapChainCount; ++i)
		{
			outReadback[i] = m_DX12Context.CreateReadBackHeapBuffer(GPU_TIMER_READBACK_BYTES_PER_QUEUE, readbackName);
			if (outReadback[i] == nullptr)
				return false;
			outAllocators[i] = m_DX12Context.CreateCommandAllocator(cmdListType, (std::wstring(allocatorNameStem) + std::to_wstring(i)).c_str());
			if (outAllocators[i] == nullptr)
				return false;
			outLists[i] = m_DX12Context.CreateCommandList(cmdListType, outAllocators[i], (std::wstring(listNameStem) + std::to_wstring(i)).c_str());
			if (outLists[i] == nullptr)
				return false;
			// CreateCommandList leaves the list in the recording state; close it so
			// the first ResolveGpuTimers Reset call sees the expected state.
			outLists[i]->Close();
		}
		auto* l_state = GetTimerState(queueType);
		if (l_state)
		{
			l_state->m_Slots.reserve(GPU_TIMER_MAX_NAMED_TIMERS);
			l_state->m_CommandListType = cmdListType;
		}
		return true;
	};

	bool l_ok = true;
	l_ok &= l_createForQueue(GPUEngineType::Graphics, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_DX12Context.m_TimestampHeap_Graphics, m_DX12Context.m_TimestampReadback_Graphics,
		m_DX12Context.m_TimestampResolveAllocators_Graphics, m_DX12Context.m_TimestampResolveLists_Graphics,
		L"GpuTimer_TimestampHeap_Graphics", "GpuTimer_TimestampReadback_Graphics",
		L"GpuTimer_ResolveAllocator_Graphics_", L"GpuTimer_ResolveList_Graphics_");
	l_ok &= l_createForQueue(GPUEngineType::Compute, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_DX12Context.m_TimestampHeap_Compute, m_DX12Context.m_TimestampReadback_Compute,
		m_DX12Context.m_TimestampResolveAllocators_Compute, m_DX12Context.m_TimestampResolveLists_Compute,
		L"GpuTimer_TimestampHeap_Compute", "GpuTimer_TimestampReadback_Compute",
		L"GpuTimer_ResolveAllocator_Compute_", L"GpuTimer_ResolveList_Compute_");
	// Copy queues require D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP; submitting
	// a copy-queue EndQuery into the regular TIMESTAMP heap fails GBV.
	l_ok &= l_createForQueue(GPUEngineType::Copy, D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP, D3D12_COMMAND_LIST_TYPE_COPY,
		m_DX12Context.m_TimestampHeap_Copy, m_DX12Context.m_TimestampReadback_Copy,
		m_DX12Context.m_TimestampResolveAllocators_Copy, m_DX12Context.m_TimestampResolveLists_Copy,
		L"GpuTimer_TimestampHeap_Copy", "GpuTimer_TimestampReadback_Copy",
		L"GpuTimer_ResolveAllocator_Copy_", L"GpuTimer_ResolveList_Copy_");

	if (l_ok)
		Log(Success, "GPU timer resources created (per-queue heap=", GPU_TIMER_TOTAL_QUERIES_PER_QUEUE,
			" queries, readback per-frame=", GPU_TIMER_READBACK_BYTES_PER_QUEUE, " B, frames=", l_swapChainCount, ").");
	else
		Log(Warning, "GPU timer resources partial-create failed; timer API will return false on the affected queue(s).");

	return l_ok;
}

void DX12GraphicsHardwareService::ReleaseGpuTimerResources()
{
	m_DX12Context.m_TimestampResolveLists_Graphics.clear();
	m_DX12Context.m_TimestampResolveLists_Compute.clear();
	m_DX12Context.m_TimestampResolveLists_Copy.clear();
	m_DX12Context.m_TimestampResolveAllocators_Graphics.clear();
	m_DX12Context.m_TimestampResolveAllocators_Compute.clear();
	m_DX12Context.m_TimestampResolveAllocators_Copy.clear();
	m_DX12Context.m_TimestampReadback_Graphics.clear();
	m_DX12Context.m_TimestampReadback_Compute.clear();
	m_DX12Context.m_TimestampReadback_Copy.clear();
	m_DX12Context.m_TimestampHeap_Graphics = nullptr;
	m_DX12Context.m_TimestampHeap_Compute = nullptr;
	m_DX12Context.m_TimestampHeap_Copy = nullptr;
	m_TimerState_Graphics.m_Slots.clear();
	m_TimerState_Graphics.m_LatestTimings.clear();
	m_TimerState_Graphics.m_MaxEverFullyRecordedSlot = -1;
	m_TimerState_Compute.m_Slots.clear();
	m_TimerState_Compute.m_LatestTimings.clear();
	m_TimerState_Compute.m_MaxEverFullyRecordedSlot = -1;
	m_TimerState_Copy.m_Slots.clear();
	m_TimerState_Copy.m_LatestTimings.clear();
	m_TimerState_Copy.m_MaxEverFullyRecordedSlot = -1;
	m_TimerResolveFrameCounter = 0;
}
