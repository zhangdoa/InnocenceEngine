#include "DX12GraphicsHardwareService.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	if (!l_globalSemaphore)
	{
		Log(Error, "Global semaphore is null in SignalOnGPU");
		return false;
	}

	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(semaphore);
	auto l_isOnGlobalSemaphore = l_semaphore == l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_DX12Context.m_directCommandQueue || !m_DX12Context.m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueue or DirectCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_DX12Context.m_directCommandQueue->Signal(m_DX12Context.m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_DX12Context.m_computeCommandQueue || !m_DX12Context.m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueue or ComputeCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		m_DX12Context.m_computeCommandQueue->Signal(m_DX12Context.m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_DX12Context.m_copyCommandQueue || !m_DX12Context.m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueue or CopyCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_copyCommandFinishedSemaphore = l_globalSemaphore->m_CopyCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_CopyCommandQueueSemaphore = l_copyCommandFinishedSemaphore;

		m_DX12Context.m_copyCommandQueue->Signal(m_DX12Context.m_copyCommandQueueFence.Get(), l_copyCommandFinishedSemaphore);
	}

	return true;
}

bool DX12GraphicsHardwareService::WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t semaphoreValue = 0;
	auto l_semaphore = semaphore ? reinterpret_cast<DX12Semaphore*>(semaphore) : l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	else if (queueType == GPUEngineType::Compute)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	else if (queueType == GPUEngineType::Copy)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY).Get();

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_DX12Context.m_directCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_DX12Context.m_computeCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_ComputeCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Copy)
	{
		fence = m_DX12Context.m_copyCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_CopyCommandQueueSemaphore;
	}

	if (semaphoreValue == 0)
		return true;

	if (commandQueue && fence)
		commandQueue->Wait(fence, semaphoreValue);

	return true;
}

bool DX12GraphicsHardwareService::Execute(CommandListComponent* commandList, GPUEngineType queueType)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	ID3D12CommandList* l_commandListToExecute[] = { l_commandList };

	if (queueType == GPUEngineType::Graphics)
		m_DX12Context.m_directCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Compute)
		m_DX12Context.m_computeCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Copy)
		m_DX12Context.m_copyCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);

	return true;
}

uint64_t DX12GraphicsHardwareService::GetSemaphoreValue(GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());

	if (queueType == GPUEngineType::Graphics)
		return l_semaphore->m_DirectCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Compute)
		return l_semaphore->m_ComputeCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Copy)
		return l_semaphore->m_CopyCommandQueueSemaphore;

	return 0;
}

bool DX12GraphicsHardwareService::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	if (semaphoreValue == 0)
		return true;

	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	HANDLE* fenceEvent = nullptr;

	if (queueType == GPUEngineType::Graphics)
		fenceEvent = &l_semaphore->m_DirectCommandQueueFenceEvent;
	else if (queueType == GPUEngineType::Compute)
		fenceEvent = &l_semaphore->m_ComputeCommandQueueFenceEvent;
	else if (queueType == GPUEngineType::Copy)
		fenceEvent = &l_semaphore->m_CopyCommandQueueFenceEvent;

	if (!fenceEvent || *fenceEvent == nullptr)
	{
		Log(Error, "Invalid fence event handle in WaitOnCPU for queue type: ", (uint32_t)queueType);
		return false;
	}

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_DX12Context.m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("DirectCommandQueueFence", m_DX12Context.m_directCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_DX12Context.m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("ComputeCommandQueueFence", m_DX12Context.m_computeCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_DX12Context.m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("CopyCommandQueueFence", m_DX12Context.m_copyCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}

	return true;
}
