#include "DX12GraphicsHardwareService.h"
#include "../../Common/Array.h"
#include "DX12GraphicsHardwareService_Internal.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

using namespace Inno;

DX12GraphicsHardwareService::DX12GpuTimerQueueState* DX12GraphicsHardwareService::GetTimerState(GPUEngineType queueType)
{
	switch (queueType)
	{
	case GPUEngineType::Graphics: return &m_TimerState_Graphics;
	case GPUEngineType::Compute:  return &m_TimerState_Compute;
	case GPUEngineType::Copy:     return &m_TimerState_Copy;
	default: return nullptr;
	}
}

const DX12GraphicsHardwareService::DX12GpuTimerQueueState* DX12GraphicsHardwareService::GetTimerState(GPUEngineType queueType) const
{
	switch (queueType)
	{
	case GPUEngineType::Graphics: return &m_TimerState_Graphics;
	case GPUEngineType::Compute:  return &m_TimerState_Compute;
	case GPUEngineType::Copy:     return &m_TimerState_Copy;
	default: return nullptr;
	}
}

ComPtr<ID3D12QueryHeap> DX12GraphicsHardwareService::GetTimestampHeap(GPUEngineType queueType) const
{
	switch (queueType)
	{
	case GPUEngineType::Graphics: return m_DX12Context.m_TimestampHeap_Graphics;
	case GPUEngineType::Compute:  return m_DX12Context.m_TimestampHeap_Compute;
	case GPUEngineType::Copy:     return m_DX12Context.m_TimestampHeap_Copy;
	default: return nullptr;
	}
}

ComPtr<ID3D12Resource> DX12GraphicsHardwareService::GetTimestampReadback(GPUEngineType queueType, uint32_t frameIndex) const
{
	auto& l_buffers = (queueType == GPUEngineType::Graphics) ? m_DX12Context.m_TimestampReadback_Graphics
	                : (queueType == GPUEngineType::Compute)  ? m_DX12Context.m_TimestampReadback_Compute
	                : (queueType == GPUEngineType::Copy)     ? m_DX12Context.m_TimestampReadback_Copy
	                : m_DX12Context.m_TimestampReadback_Graphics;
	if (frameIndex >= l_buffers.size())
		return nullptr;
	return l_buffers[frameIndex];
}

uint32_t DX12GraphicsHardwareService::FindOrAllocateTimerSlot(GPUEngineType queueType, const char* name)
{
	auto* l_state = GetTimerState(queueType);
	if (l_state == nullptr || name == nullptr)
		return UINT32_MAX;

	for (size_t i = 0; i < l_state->m_Slots.size(); ++i)
	{
		if (l_state->m_Slots[i].m_Name == name)
			return static_cast<uint32_t>(i);
	}

	if (l_state->m_Slots.size() >= GPU_TIMER_MAX_NAMED_TIMERS)
	{
		Log(Warning, "GPU timer: queue=", static_cast<int32_t>(queueType),
			" capacity exhausted (max=", GPU_TIMER_MAX_NAMED_TIMERS,
			"); dropping timer for '", name, "'. Raise GPU_TIMER_MAX_NAMED_TIMERS or remove unused names.");
		return UINT32_MAX;
	}

	DX12GpuTimerSlot l_slot;
	l_slot.m_Name = name;
	l_slot.m_SlotIndex = static_cast<uint32_t>(l_state->m_Slots.size());
	l_state->m_Slots.push_back(l_slot);
	return l_slot.m_SlotIndex;
}

uint32_t DX12GraphicsHardwareService::FindTimerSlot(GPUEngineType queueType, const char* name) const
{
	auto* l_state = GetTimerState(queueType);
	if (l_state == nullptr || name == nullptr)
		return UINT32_MAX;
	for (size_t i = 0; i < l_state->m_Slots.size(); ++i)
	{
		if (l_state->m_Slots[i].m_Name == name)
			return static_cast<uint32_t>(i);
	}
	return UINT32_MAX;
}

Inno::Array<GpuTimingResult> DX12GraphicsHardwareService::GetGpuTimings() const
{
	Inno::Array<GpuTimingResult> l_all;
	const GPUEngineType l_queues[] = { GPUEngineType::Graphics, GPUEngineType::Compute, GPUEngineType::Copy };
	for (auto l_queue : l_queues)
	{
		auto* l_state = GetTimerState(l_queue);
		if (!l_state)
			continue;
		l_all.insert(l_all.end(), l_state->m_LatestTimings.begin(), l_state->m_LatestTimings.end());
	}
	return l_all;
}
