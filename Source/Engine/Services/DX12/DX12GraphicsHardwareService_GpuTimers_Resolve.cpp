#include "DX12GraphicsHardwareService.h"
#include "DX12GraphicsHardwareService_Internal.h"
#include "../FrameManagementService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

using namespace Inno;

bool DX12GraphicsHardwareService::ResolveGpuTimers()
{
	if (m_FrameManagementService == nullptr)
		return false;
	if (m_DX12Context.m_TimestampHeap_Graphics == nullptr)
		return false;

	auto l_currentFrame = m_FrameManagementService->GetCurrentFrame();
	auto l_swapChainCount = m_FrameManagementService->GetSwapChainImageCount();
	if (l_swapChainCount == 0)
		return false;

	// 1) Resolve THIS frame's queries into THIS frame's readback buffer.
	//    Uses dedicated per-frame allocator + list so it doesn't share state
	//    with the engine's pass-recording allocators. The per-frame allocator
	//    is safe to Reset because BeginFrame waited on the matching fence
	//    for this slot before we got here.
	const GPUEngineType l_queues[] = { GPUEngineType::Graphics, GPUEngineType::Compute, GPUEngineType::Copy };
	for (auto l_queue : l_queues)
	{
		auto* l_state = GetTimerState(l_queue);
		auto l_heap = GetTimestampHeap(l_queue);
		auto l_readback = GetTimestampReadback(l_queue, l_currentFrame);
		if (!l_state || !l_heap || !l_readback)
			continue;

		// Skip queues with no recorded timers this frame — avoids submitting
		// a no-op command list and the per-queue execute cost.
		bool l_anyRecorded = false;
		for (auto& l_slot : l_state->m_Slots)
		{
			if (l_slot.m_EndRecordedThisFrame)
			{
				l_anyRecorded = true;
				break;
			}
		}
		if (!l_anyRecorded)
			continue;
		// No slot has ever been fully recorded — nothing safe to resolve.
		if (l_state->m_MaxEverFullyRecordedSlot < 0)
			continue;

		auto& l_allocs = (l_queue == GPUEngineType::Graphics) ? m_DX12Context.m_TimestampResolveAllocators_Graphics
		               : (l_queue == GPUEngineType::Compute)  ? m_DX12Context.m_TimestampResolveAllocators_Compute
		               :                                         m_DX12Context.m_TimestampResolveAllocators_Copy;
		auto& l_lists = (l_queue == GPUEngineType::Graphics) ? m_DX12Context.m_TimestampResolveLists_Graphics
		              : (l_queue == GPUEngineType::Compute)  ? m_DX12Context.m_TimestampResolveLists_Compute
		              :                                         m_DX12Context.m_TimestampResolveLists_Copy;
		if (l_currentFrame >= l_allocs.size() || l_currentFrame >= l_lists.size())
			continue;

		auto l_allocator = l_allocs[l_currentFrame];
		auto l_list = l_lists[l_currentFrame];
		if (!l_allocator || !l_list)
			continue;

		if (FAILED(l_allocator->Reset()))
		{
			Log(Warning, "ResolveGpuTimers: allocator Reset failed for queue=", static_cast<int32_t>(l_queue), " frame=", l_currentFrame);
			continue;
		}
		if (FAILED(l_list->Reset(l_allocator.Get(), nullptr)))
		{
			Log(Warning, "ResolveGpuTimers: command list Reset failed for queue=", static_cast<int32_t>(l_queue), " frame=", l_currentFrame);
			continue;
		}

		// Resolve only up through the highest slot ever fully recorded.
		// D3D12 GBV rejects ResolveQueryData for queries that have never been
		// performed (TASK-140 validation discovery), so we cannot blindly
		// resolve the entire heap. Slots covered by this range whose End was
		// not recorded *this* frame still resolve cleanly because their
		// timestamp memory holds the previous successful pair.
		const uint32_t l_resolveCount = static_cast<uint32_t>(l_state->m_MaxEverFullyRecordedSlot + 1) * GPU_TIMER_QUERIES_PER_TIMER;
		l_list->ResolveQueryData(l_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
			0, l_resolveCount,
			l_readback.Get(), 0);

		l_list->Close();
		ID3D12CommandList* l_listsToExec[] = { l_list.Get() };
		auto l_queue_d3d = m_DX12Context.GetGlobalCommandQueue(l_state->m_CommandListType);
		if (l_queue_d3d)
			l_queue_d3d->ExecuteCommandLists(1, l_listsToExec);
	}

	// 2) Read back the readback buffer from N frames ago — by then the GPU
	//    has caught up and the data is safe to map without a sync stall.
	if (m_TimerResolveFrameCounter >= GPU_TIMER_READBACK_FRAME_LATENCY)
	{
		uint32_t l_readbackFrame = static_cast<uint32_t>(
			(m_TimerResolveFrameCounter - GPU_TIMER_READBACK_FRAME_LATENCY) % l_swapChainCount);

		for (auto l_queue : l_queues)
		{
			auto* l_state = GetTimerState(l_queue);
			auto l_readback = GetTimestampReadback(l_queue, l_readbackFrame);
			if (!l_state || !l_readback)
				continue;

			auto l_queue_d3d = m_DX12Context.GetGlobalCommandQueue(l_state->m_CommandListType);
			if (!l_queue_d3d)
				continue;

			UINT64 l_freq = 0;
			if (FAILED(l_queue_d3d->GetTimestampFrequency(&l_freq)) || l_freq == 0)
				continue;

			if (l_state->m_MaxEverFullyRecordedSlot < 0)
				continue;
			const SIZE_T l_readBytes = static_cast<SIZE_T>(l_state->m_MaxEverFullyRecordedSlot + 1) * GPU_TIMER_QUERIES_PER_TIMER * sizeof(UINT64);
			D3D12_RANGE l_readRange = { 0, l_readBytes };
			void* l_mapped = nullptr;
			if (FAILED(l_readback->Map(0, &l_readRange, &l_mapped)) || l_mapped == nullptr)
				continue;
			const UINT64* l_timestamps = static_cast<const UINT64*>(l_mapped);

			l_state->m_LatestTimings.clear();
			l_state->m_LatestTimings.reserve(l_state->m_Slots.size());
			for (auto& l_slot : l_state->m_Slots)
			{
				if (static_cast<int32_t>(l_slot.m_SlotIndex) > l_state->m_MaxEverFullyRecordedSlot)
					continue;
				const UINT64 l_begin = l_timestamps[l_slot.m_SlotIndex * GPU_TIMER_QUERIES_PER_TIMER];
				const UINT64 l_end   = l_timestamps[l_slot.m_SlotIndex * GPU_TIMER_QUERIES_PER_TIMER + 1];
				// Stale slot or end-before-begin (clock wrap on idle queues) — skip.
				if (l_end <= l_begin)
					continue;
				const double l_ms = static_cast<double>(l_end - l_begin) * 1000.0 / static_cast<double>(l_freq);
				GpuTimingResult l_result;
				l_result.m_Name = l_slot.m_Name;
				l_result.m_Milliseconds = l_ms;
				l_result.m_QueueType = l_queue;
				l_state->m_LatestTimings.push_back(std::move(l_result));
			}

			D3D12_RANGE l_writeRange = { 0, 0 };
			l_readback->Unmap(0, &l_writeRange);
		}
	}

	// 3) Advance frame counter and clear "recorded this frame" flags so the
	//    next frame's Begin/End run cleanly.
	for (auto l_queue : l_queues)
	{
		auto* l_state = GetTimerState(l_queue);
		if (!l_state)
			continue;
		for (auto& l_slot : l_state->m_Slots)
			l_slot.m_EndRecordedThisFrame = false;
	}
	++m_TimerResolveFrameCounter;

	// Periodic Verbose dump for validation / "is the timer infra wired" checks.
	// First dump fires the moment readback becomes live (cf. FRAME_LATENCY)
	// so short -total_frames smoke runs still get a baseline; subsequent
	// dumps respect GPU_TIMER_LOG_PERIOD_FRAMES so long runs aren't spammed.
	// Silent by default — opt in with -gpu_timer_log on the command line
	// (mirrors -gpu_validation). Per-frame, per-pass log lines drown the
	// signal at any non-default loglevel; the gate keeps the timer
	// collection live but suppresses the readout unless explicitly asked.
	if (g_Engine->getInitConfig().enableGpuTimerLog)
	{
		static constexpr uint32_t GPU_TIMER_LOG_PERIOD_FRAMES = 30;
		const bool l_firstReadbackReady = (m_TimerResolveFrameCounter == GPU_TIMER_READBACK_FRAME_LATENCY + 1);
		const bool l_periodicHit = (m_TimerResolveFrameCounter > GPU_TIMER_READBACK_FRAME_LATENCY)
			&& (m_TimerResolveFrameCounter % GPU_TIMER_LOG_PERIOD_FRAMES) == 0;
		if (l_firstReadbackReady || l_periodicHit)
		{
			auto l_timings = GetGpuTimings();
			for (auto& l_t : l_timings)
			{
				Log(Verbose, "GpuTimer[", static_cast<int32_t>(l_t.m_QueueType), "] ", l_t.m_Name.c_str(), " = ", l_t.m_Milliseconds, " ms");
			}
		}
	}

	return true;
}
