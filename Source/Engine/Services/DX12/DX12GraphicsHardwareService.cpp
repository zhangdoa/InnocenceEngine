#include "DX12GraphicsHardwareService.h"
#include "../FrameManagementService.h"
#include "../GraphicsResourceService.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/DrawCallService.h"
#include "../../Services/AssetService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

// --- Sync primitives ---

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
		if (!m_ctx->m_directCommandQueue || !m_ctx->m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueue or DirectCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_ctx->m_directCommandQueue->Signal(m_ctx->m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_ctx->m_computeCommandQueue || !m_ctx->m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueue or ComputeCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		m_ctx->m_computeCommandQueue->Signal(m_ctx->m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_ctx->m_copyCommandQueue || !m_ctx->m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueue or CopyCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_copyCommandFinishedSemaphore = l_globalSemaphore->m_CopyCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_CopyCommandQueueSemaphore = l_copyCommandFinishedSemaphore;

		m_ctx->m_copyCommandQueue->Signal(m_ctx->m_copyCommandQueueFence.Get(), l_copyCommandFinishedSemaphore);
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
		commandQueue = m_ctx->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	else if (queueType == GPUEngineType::Compute)
		commandQueue = m_ctx->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	else if (queueType == GPUEngineType::Copy)
		commandQueue = m_ctx->GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY).Get();

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_ctx->m_directCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_ctx->m_computeCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_ComputeCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Copy)
	{
		fence = m_ctx->m_copyCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_CopyCommandQueueSemaphore;
	}

	if (commandQueue && fence)
		commandQueue->Wait(fence, semaphoreValue);

	return true;
}

bool DX12GraphicsHardwareService::Execute(CommandListComponent* commandList, GPUEngineType queueType)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	ID3D12CommandList* l_commandListToExecute[] = { l_commandList };

	if (queueType == GPUEngineType::Graphics)
		m_ctx->m_directCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Compute)
		m_ctx->m_computeCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Copy)
		m_ctx->m_copyCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);

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
		if (!m_ctx->m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		if (m_ctx->m_directCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_ctx->m_directCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000);
			if (waitResult == WAIT_TIMEOUT)
			{
				Log(Error, "DirectCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue,
					", Completed value: ", m_ctx->m_directCommandQueueFence->GetCompletedValue());
				return false;
			}
			else if (waitResult != WAIT_OBJECT_0)
			{
				Log(Error, "DirectCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
				return false;
			}
		}
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_ctx->m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		if (m_ctx->m_computeCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_ctx->m_computeCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000);
			if (waitResult == WAIT_TIMEOUT)
			{
				Log(Error, "ComputeCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue,
					", Completed value: ", m_ctx->m_computeCommandQueueFence->GetCompletedValue());
				return false;
			}
			else if (waitResult != WAIT_OBJECT_0)
			{
				Log(Error, "ComputeCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
				return false;
			}
		}
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_ctx->m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		if (m_ctx->m_copyCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			m_ctx->m_copyCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000);
			if (waitResult == WAIT_TIMEOUT)
			{
				Log(Error, "CopyCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue,
					", Completed value: ", m_ctx->m_copyCommandQueueFence->GetCompletedValue());
				return false;
			}
			else if (waitResult != WAIT_OBJECT_0)
			{
				Log(Error, "CopyCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
				return false;
			}
		}
	}

	return true;
}

// --- Command list lifecycle ---

bool DX12GraphicsHardwareService::Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_pipelineStateObject = reinterpret_cast<DX12PipelineStateObject*>(pipelineStateObject);
	auto l_PSO = l_pipelineStateObject ? l_pipelineStateObject->m_PSO.Get() : nullptr;
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	ID3D12CommandAllocator* allocator = nullptr;
	switch (commandList->m_Type)
	{
	case GPUEngineType::Graphics:
		allocator = m_ctx->m_directCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Compute:
		allocator = m_ctx->m_computeCommandAllocators[l_currentFrame].Get();
		break;
	case GPUEngineType::Copy:
		allocator = m_ctx->m_copyCommandAllocators[l_currentFrame].Get();
		break;
	default:
		Log(Error, "Invalid command list type for Open operation");
		return false;
	}

	auto l_resetResult = l_commandList->Reset(allocator, l_PSO);
	if (FAILED(l_resetResult))
	{
		Log(Error, "DX12GraphicsHardwareService::Open: Reset failed, HRESULT=", l_resetResult);
		return false;
	}
	return true;
}

bool DX12GraphicsHardwareService::Close(CommandListComponent* commandList, GPUEngineType engineType)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_closeResult = l_commandList->Close();
	if (FAILED(l_closeResult))
	{
		Log(Error, "DX12GraphicsHardwareService::Close: Close failed, HRESULT=", l_closeResult);
		return false;
	}
	return true;
}

// --- Command recording ---

bool DX12GraphicsHardwareService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPass)
		return false;

	return Open(commandList, commandList->m_Type, renderPass->m_PipelineStateObject);
}

bool DX12GraphicsHardwareService::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindRenderPassComponent");
		return false;
	}

	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_dx12CommandList)
	{
		Log(Error, "Invalid DX12 command list in BindRenderPassComponent");
		return false;
	}

	ChangeRenderTargetStates(renderPass, commandList, Accessibility::ReadOnly, Accessibility::WriteOnly);
	SetDescriptorHeaps(renderPass, commandList);
	SetRenderTargets(renderPass, commandList);

	if (renderPass->m_PipelineStateObject)
	{
		auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
		PreparePipeline(renderPass, commandList, l_PSO);
	}

	if (renderPass->m_CustomCommandsFunc)
		renderPass->m_CustomCommandsFunc(commandList);

	return true;
}

bool DX12GraphicsHardwareService::ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in ClearRenderTargets");
		return false;
	}

	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (renderPass->m_RenderPassDesc.m_RenderTargetCount == 0)
		return true;

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "Invalid DX12 command list in ClearRenderTargets");
		return false;
	}

	auto f_clearRenderTargetAsUAV = [&](DX12DeviceMemory* l_RT)
		{
			if (renderPass->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
			{
				l_commandList->ClearUnorderedAccessViewUint(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle },
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(),
					(UINT*)renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0, nullptr);
			}
			else
			{
				l_commandList->ClearUnorderedAccessViewFloat(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle },
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(),
					renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
					0, nullptr);
			}
		};

	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);
	if (!l_outputMergerTarget)
	{
		Log(Error, "Output merger target is null in ClearRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		if (l_currentFrame >= l_outputMergerTarget->m_RTVs.size())
		{
			Log(Error, "Invalid frame index ", l_currentFrame, " for RTVs in render pass ", renderPass->m_InstanceName);
			return false;
		}

		auto& l_RTV = l_outputMergerTarget->m_RTVs[l_currentFrame];
		if (l_RTV.m_Handles.empty())
		{
			Log(Error, "RTV handles are empty for render pass ", renderPass->m_InstanceName);
			return false;
		}

		if (index != -1 && index < renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			if (index >= l_RTV.m_Handles.size())
			{
				Log(Error, "Invalid RTV handle index ", index, " for render pass ", renderPass->m_InstanceName);
				return false;
			}
			if (l_RTV.m_Handles[index].ptr == 0)
			{
				Log(Error, "Null RTV handle at index ", index, " for render pass ", renderPass->m_InstanceName);
				return false;
			}
			l_commandList->ClearRenderTargetView(l_RTV.m_Handles[index], renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
		}
		else
		{
			for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				if (i >= l_RTV.m_Handles.size())
				{
					Log(Error, "Invalid RTV handle index ", i, " for render pass ", renderPass->m_InstanceName);
					return false;
				}
				if (l_RTV.m_Handles[i].ptr == 0)
				{
					Log(Error, "Null RTV handle at index ", i, " for render pass ", renderPass->m_InstanceName);
					return false;
				}
				l_commandList->ClearRenderTargetView(l_RTV.m_Handles[i], renderPass->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
		}
	}
	else
	{
		if (index != -1 && index < renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(l_outputMergerTarget->m_ColorOutputs[index]->m_GPUResources[l_currentFrame]));
		}
		else
		{
			for (auto i : l_outputMergerTarget->m_ColorOutputs)
			{
				f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(i->m_GPUResources[l_currentFrame]));
			}
		}
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			l_flag |= D3D12_CLEAR_FLAG_STENCIL;

		auto l_DSV = l_outputMergerTarget->m_DSVs[l_currentFrame];
		l_commandList->ClearDepthStencilView(l_DSV.m_Handle, l_flag, 1.0f, 0x00, 0, nullptr);
	}

	return true;
}

bool DX12GraphicsHardwareService::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindGPUResource");
		return false;
	}

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		return BindComputeResource(commandList, resourceBindingLayoutDescIndex, renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);
	else
		return BindGraphicsResource(commandList, resourceBindingLayoutDescIndex, renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);

	return false;
}

bool DX12GraphicsHardwareService::UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return false;
}

bool DX12GraphicsHardwareService::TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));

	auto l_oldState = static_cast<D3D12_RESOURCE_STATES>(texture->GetCurrentState(frameIndex));
	auto l_newState = static_cast<D3D12_RESOURCE_STATES>(targetAccessibility.CanWrite() ? texture->m_WriteState : texture->m_ReadState);
	if (targetAccessibility.IsCopySource())
		l_newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (targetAccessibility.IsCopyDestination())
		l_newState = D3D12_RESOURCE_STATE_COPY_DEST;

	if (l_oldState != l_newState)
	{
		auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			l_oldState,
			l_newState);

		l_commandList->ResourceBarrier(1, &l_transition);

		texture->SetCurrentState(frameIndex, static_cast<uint32_t>(l_newState));
	}

	return true;
}

bool DX12GraphicsHardwareService::TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[frameIndex]);

	auto l_oldState = static_cast<D3D12_RESOURCE_STATES>(gpuBuffer->GetCurrentState(frameIndex));
	auto l_newState = static_cast<D3D12_RESOURCE_STATES>(targetAccessibility.CanWrite() ? gpuBuffer->m_WriteState : gpuBuffer->m_ReadState);
	if (targetAccessibility.IsCopySource())
		l_newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (targetAccessibility.IsCopyDestination())
		l_newState = D3D12_RESOURCE_STATE_COPY_DEST;

	if (l_oldState != l_newState)
	{
		auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			l_deviceMemory->m_DefaultHeapBuffer.Get(),
			l_oldState,
			l_newState);

		l_commandList->ResourceBarrier(1, &l_transition);

		gpuBuffer->SetCurrentState(frameIndex, l_newState);
	}

	return true;
}

bool DX12GraphicsHardwareService::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	if (!renderPass || !commandList || !mesh)
	{
		Log(Error, "Null parameters in DrawIndexedInstanced");
		return false;
	}

	auto* l_resource = AssetService::GetMeshAsset(mesh->m_Asset);
	if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
		return false;

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = l_resource->m_VertexBufferView.m_BufferLocation;
	vbv.StrideInBytes = l_resource->m_VertexBufferView.m_StrideInBytes;
	vbv.SizeInBytes = l_resource->m_VertexBufferView.m_SizeInBytes;

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = l_resource->m_IndexBufferView.m_BufferLocation;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = l_resource->m_IndexBufferView.m_SizeInBytes;

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 1, &vbv);
	l_commandList->IASetIndexBuffer(&ibv);
	l_commandList->DrawIndexedInstanced(l_resource->GetIndexCount(), (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12GraphicsHardwareService::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DrawInstanced");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 0, nullptr);
	l_commandList->IASetIndexBuffer(nullptr);
	l_commandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12GraphicsHardwareService::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in Dispatch");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in Dispatch for render pass ", renderPass->m_InstanceName);
		return false;
	}

	l_commandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12GraphicsHardwareService::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DispatchRays");
		return false;
	}

	if (!g_Engine->Get<GraphicsResourceService>()->IsTLASReady())
		return false;

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	auto l_shaderIDBufferVirtualAddress = l_PSO->m_RaytracingShaderIDBuffer->GetGPUVirtualAddress();

	D3D12_RESOURCE_DESC l_bufDesc = l_PSO->m_RaytracingShaderIDBuffer->GetDesc();
	const bool hasShadowMiss = (l_bufDesc.Width >= 4 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	dispatchDesc.RayGenerationShaderRecord.StartAddress = l_shaderIDBufferVirtualAddress;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.MissShaderTable.StartAddress = l_shaderIDBufferVirtualAddress + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.SizeInBytes = hasShadowMiss
		? 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
		: D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	const uint64_t hitGroupOffset = hasShadowMiss
		? 3 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
		: 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.HitGroupTable.StartAddress = l_shaderIDBufferVirtualAddress + hitGroupOffset;
	dispatchDesc.HitGroupTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.HitGroupTable.SizeInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.Width = dimensionX;
	dispatchDesc.Height = dimensionY;
	dispatchDesc.Depth = dimensionZ;

	l_commandList->DispatchRays(&dispatchDesc);

	return true;
}

bool DX12GraphicsHardwareService::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	if (!renderPass || !commandList || !indirectDrawCommand)
	{
		Log(Error, "Null parameters in ExecuteIndirect");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(indirectDrawCommand->m_DeviceMemories[g_Engine->Get<FrameManagementService>()->GetCurrentFrame()]);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

	auto l_modelCount = (uint32_t)g_Engine->Get<DrawCallService>()->GetGPUModelData().size();
	UINT maxDrawCommandCount = static_cast<UINT>(l_modelCount);

	if (maxDrawCommandCount == 0)
		return false;

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadWrite, Accessibility::ReadOnly);

	l_commandList->ExecuteIndirect(l_PSO->m_IndirectCommandSignature.Get(), maxDrawCommandCount, l_deviceMemory->m_DefaultHeapBuffer.Get(), 0, nullptr, 0);

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadOnly, Accessibility::ReadWrite);

	return true;
}

void DX12GraphicsHardwareService::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
	if (!renderPass || !commandList)
		return;

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
}

bool DX12GraphicsHardwareService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in CommandListEnd");
		return false;
	}

	if (!Close(commandList, renderPass->m_RenderPassDesc.m_GPUEngineType))
	{
		Log(Error, "Failed to close command list for render pass ", renderPass->m_InstanceName);
		return false;
	}

	return true;
}

// --- Debug/capture ---

bool DX12GraphicsHardwareService::BeginCapture()
{
	if (m_ctx->m_graphicsAnalysis != nullptr)
	{
		m_ctx->m_graphicsAnalysis->BeginCapture();
		return true;
	}

	return false;
}

bool DX12GraphicsHardwareService::EndCapture()
{
	if (m_ctx->m_graphicsAnalysis != nullptr)
	{
		m_ctx->m_graphicsAnalysis->EndCapture();
		return true;
	}

	return false;
}

bool DX12GraphicsHardwareService::HasGPUError() const
{
	return m_ctx->m_GPUErrorDetected.load();
}

// --- DX12-specific public accessors ---

ComPtr<ID3D12Device8> DX12GraphicsHardwareService::GetDevice()
{
	return m_ctx->m_device.Get();
}

ComPtr<ID3D12CommandAllocator> DX12GraphicsHardwareService::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	return m_ctx->GetGlobalCommandAllocator(commandListType, l_currentFrame);
}

ComPtr<ID3D12CommandQueue> DX12GraphicsHardwareService::GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_ctx->GetGlobalCommandQueue(commandListType);
}

DX12DescriptorHeapAccessor& DX12GraphicsHardwareService::GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility,
	Accessibility resourceAccessibility, TextureUsage textureUsage, bool isShaderVisible)
{
	return m_ctx->GetDescriptorHeapAccessor(type, bindingAccessibility, resourceAccessibility, textureUsage, isShaderVisible);
}

// --- Private helpers ---

bool DX12GraphicsHardwareService::BindComputeResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (!commandList)
	{
		Log(Error, "CommandList is null in BindComputeResource");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in BindComputeResource");
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
		{
			Log(Error, "Attempt to bind inactivated GPU buffer ", l_buffer->m_InstanceName);
			return false;
		}

		auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_buffer->m_MappedMemories[l_currentFrame]);
		auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_buffer->m_DeviceMemories[l_currentFrame]);
		auto& l_SRV = l_deviceMemory->m_SRV;
		auto& l_UAV = l_deviceMemory->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_mappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
				l_commandList->SetComputeRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				if (l_buffer->m_Usage == GPUBufferUsage::TLAS)
				{
					auto l_GPUVirtualAddress = l_deviceMemory->m_DefaultHeapBuffer->GetGPUVirtualAddress();
					l_commandList->SetComputeRootShaderResourceView(rootParameterIndex, l_GPUVirtualAddress);
					return true;
				}
				else
				{
					l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_SRV.Handle.m_GPUHandle });
					return true;
				}
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_UAV.Handle.m_GPUHandle });
				return true;
			}
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::Sample)
		{
			auto& l_textureDescHeapAccessor = m_ctx->GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
				, resourceBindingLayoutDesc.m_ResourceAccessibility, TextureUsage::Sample);
			l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_textureDescHeapAccessor.GetFirstHandle().m_GPUHandle });
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ComputeOnly)
		{
			auto l_image = reinterpret_cast<TextureComponent*>(resource);
			if (l_image->m_ObjectStatus != ObjectStatus::Activated)
			{
				Log(Error, "Attempt to bind inactivated texture ", l_image->m_InstanceName);
				return false;
			}

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? l_currentFrame : 0;

			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
			{
				l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_WriteHandles[l_handleIndex].m_GPUHandle });
			}
			else
			{
				l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_ReadHandles[l_handleIndex].m_GPUHandle });
			}
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[l_currentFrame].m_GPUHandle;
		l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
	}

	assert(false);
	return false;
}

bool DX12GraphicsHardwareService::BindGraphicsResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (!commandList)
	{
		Log(Error, "CommandList is null in BindGraphicsResource");
		return false;
	}

	if (!commandList->m_CommandList)
	{
		Log(Error, "CommandList is null in BindGraphicsResource");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in BindGraphicsResource");
		return false;
	}

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
		{
			Log(Error, "Attempt to bind inactivated GPU buffer ", l_buffer->m_InstanceName);
			return false;
		}

		auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_buffer->m_MappedMemories[l_currentFrame]);
		auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_buffer->m_DeviceMemories[l_currentFrame]);
		auto& l_SRV = l_deviceMemory->m_SRV;
		auto& l_UAV = l_deviceMemory->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_mappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
				l_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_SRV.Handle.m_GPUHandle });
				return true;
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_UAV.Handle.m_GPUHandle });
				return true;
			}
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::Sample)
		{
			auto& l_textureDescHeapAccessor = m_ctx->GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
				, resourceBindingLayoutDesc.m_ResourceAccessibility, TextureUsage::Sample);
			l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_textureDescHeapAccessor.GetFirstHandle().m_GPUHandle });
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ComputeOnly)
		{
			auto l_image = reinterpret_cast<TextureComponent*>(resource);
			if (l_image->m_ObjectStatus != ObjectStatus::Activated)
			{
				Log(Error, "Attempt to bind inactivated texture ", l_image->m_InstanceName);
				return false;
			}

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? l_currentFrame : 0;
			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
			{
				l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_WriteHandles[l_handleIndex].m_GPUHandle });
			}
			else
			{
				l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_ReadHandles[l_handleIndex].m_GPUHandle });
			}
		}
		return true;
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[l_currentFrame].m_GPUHandle;
		l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
		return true;
	}

	assert(false);
	return false;
}

bool DX12GraphicsHardwareService::SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_commandList)
	{
		Log(Error, "Command list is null in SetDescriptorHeaps for render pass ", renderPass->m_InstanceName);
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get(), m_ctx->m_SamplerDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12GraphicsHardwareService::SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (!commandList || !commandList->m_CommandList)
	{
		Log(Error, "Command list is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);

	if (!l_outputMergerTarget)
	{
		Log(Error, "Output merger target is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* l_DSV = NULL;
	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		l_DSV = &l_outputMergerTarget->m_DSVs[l_currentFrame].m_Handle;

	D3D12_CPU_DESCRIPTOR_HANDLE* l_RTVs = NULL;
	uint32_t l_RTCount = 0;
	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		if (renderPass->m_RenderPassDesc.m_RenderTargetCount)
		{
			l_RTVs = &l_outputMergerTarget->m_RTVs[l_currentFrame].m_Handles[0];
			l_RTCount = (uint32_t)renderPass->m_RenderPassDesc.m_RenderTargetCount;
		}
	}

	l_commandList->OMSetRenderTargets(l_RTCount, l_RTVs, FALSE, l_DSV);
	return true;
}

bool DX12GraphicsHardwareService::PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (!l_commandList)
	{
		Log(Error, "Command list is null in PreparePipeline for render pass ", renderPass->m_InstanceName);
		return false;
	}

	if (PSO->m_PSO)
		l_commandList->SetPipelineState(PSO->m_PSO.Get());

	if (PSO->m_RaytracingPSO)
		l_commandList->SetPipelineState1(PSO->m_RaytracingPSO.Get());

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (PSO->m_RootSignature)
			l_commandList->SetGraphicsRootSignature(PSO->m_RootSignature.Get());

		if (PSO->m_PSO)
		{
			l_commandList->RSSetViewports(1, &PSO->m_Viewport);
			l_commandList->RSSetScissorRects(1, &PSO->m_Scissor);
			if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
			{
				l_commandList->OMSetStencilRef(renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference);
			}
		}
	}
	else
	{
		if (PSO->m_RootSignature)
			l_commandList->SetComputeRootSignature(PSO->m_RootSignature.Get());
	}

	return true;
}

bool DX12GraphicsHardwareService::ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;

	for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
	{
		auto l_renderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[i]);
		TryToTransitState(l_renderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_depthStencilRenderTarget = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
		TryToTransitState(l_depthStencilRenderTarget, commandList, sourceAccessibility, targetAccessibility);
	}

	return true;
}
