#include "DX12GraphicsHardwareService.h"
#include "DX12GraphicsHardwareService_Internal.h"
#include "DX12Helper_Common.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::BeginGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType)
{
	auto l_heap = GetTimestampHeap(queueType);
	if (l_heap == nullptr)
		return false;

	auto l_commandList = AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	if (name == nullptr || name[0] == '\0')
	{
		Log(Warning, "BeginGpuTimer: empty/null name on queue=", static_cast<int32_t>(queueType), "; skipping.");
		return false;
	}

	auto l_slotIndex = FindOrAllocateTimerSlot(queueType, name);
	if (l_slotIndex == UINT32_MAX)
		return false;

	auto* l_state = GetTimerState(queueType);
	auto& l_slot = l_state->m_Slots[l_slotIndex];

	if (l_slot.m_BeginRecorded)
	{
		Log(Warning, "BeginGpuTimer: nested Begin without matching End for '", name,
			"' on queue=", static_cast<int32_t>(queueType), "; previous Begin will be overwritten (timing for this frame may be wrong).");
	}

	// D3D12 uses EndQuery for both ends of a TIMESTAMP query — the API name is
	// historical; semantically each EndQuery records "the GPU reached this point".
	l_commandList->EndQuery(l_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, l_slot.m_SlotIndex * GPU_TIMER_QUERIES_PER_TIMER);
	l_slot.m_BeginRecorded = true;
	l_slot.m_EndRecordedThisFrame = false;
	return true;
}

bool DX12GraphicsHardwareService::EndGpuTimer(CommandListComponent* commandList, const char* name, GPUEngineType queueType)
{
	auto l_heap = GetTimestampHeap(queueType);
	if (l_heap == nullptr)
		return false;

	auto l_commandList = AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	if (name == nullptr || name[0] == '\0')
	{
		Log(Warning, "EndGpuTimer: empty/null name on queue=", static_cast<int32_t>(queueType), "; skipping.");
		return false;
	}

	auto l_slotIndex = FindTimerSlot(queueType, name);
	if (l_slotIndex == UINT32_MAX)
	{
		Log(Warning, "EndGpuTimer: no matching Begin for '", name,
			"' on queue=", static_cast<int32_t>(queueType), "; ignoring.");
		return false;
	}

	auto* l_state = GetTimerState(queueType);
	auto& l_slot = l_state->m_Slots[l_slotIndex];

	if (!l_slot.m_BeginRecorded)
	{
		Log(Warning, "EndGpuTimer: End without matching Begin for '", name,
			"' on queue=", static_cast<int32_t>(queueType), "; ignoring.");
		return false;
	}

	l_commandList->EndQuery(l_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, l_slot.m_SlotIndex * GPU_TIMER_QUERIES_PER_TIMER + 1);
	l_slot.m_BeginRecorded = false;
	l_slot.m_EndRecordedThisFrame = true;
	if (static_cast<int32_t>(l_slot.m_SlotIndex) > l_state->m_MaxEverFullyRecordedSlot)
		l_state->m_MaxEverFullyRecordedSlot = static_cast<int32_t>(l_slot.m_SlotIndex);
	return true;
}
