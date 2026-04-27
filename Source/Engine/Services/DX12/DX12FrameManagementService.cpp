#include "DX12FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../CommandListResourceService.h"
#include "../GPUBufferResourceService.h"
#include "../../Engine.h"
#include "../../Platform/WinWindow/WinWindowService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/DrawCallService.h"
#include "../../Services/AssetService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

// --- Command list lifecycle ---

bool DX12FrameManagementService::Open(CommandListComponent* commandList, GPUEngineType engineType, IPipelineStateObject* pipelineStateObject)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_pipelineStateObject = reinterpret_cast<DX12PipelineStateObject*>(pipelineStateObject);
	auto l_PSO = l_pipelineStateObject ? l_pipelineStateObject->m_PSO.Get() : nullptr;
	auto l_currentFrame = GetCurrentFrame();

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
		Log(Error, "DX12FrameManagementService::Open: Reset failed, HRESULT=", l_resetResult);
		return false;
	}
	return true;
}

bool DX12FrameManagementService::Close(CommandListComponent* commandList, GPUEngineType engineType)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_closeResult = l_commandList->Close();
	if (FAILED(l_closeResult))
	{
		Log(Error, "DX12FrameManagementService::Close: Close failed, HRESULT=", l_closeResult);
		return false;
	}
	return true;
}

// --- Command recording ---

bool DX12FrameManagementService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPass)
	{
		Log(Warning, "DX12FrameManagementService::CommandListBegin: null ", (!commandList ? "commandList" : "renderPass"));
		return false;
	}

	return Open(commandList, commandList->m_Type, renderPass->m_PipelineStateObject);
}

bool DX12FrameManagementService::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindRenderPassComponent");
		return false;
	}

	auto l_dx12CommandList = DX12Helper::AsDX12CommandList(commandList);
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

bool DX12FrameManagementService::ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index)
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

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
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

	auto l_currentFrame = GetCurrentFrame();
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

bool DX12FrameManagementService::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
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
}

bool DX12FrameManagementService::TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));

	auto l_oldState = static_cast<D3D12_RESOURCE_STATES>(texture->GetCurrentState(frameIndex));
	auto l_newState = static_cast<D3D12_RESOURCE_STATES>(targetAccessibility.CanWrite() ? texture->m_WriteState : texture->m_ReadState);
	if (targetAccessibility.IsCopySource())
		l_newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (targetAccessibility.IsCopyDestination())
		l_newState = D3D12_RESOURCE_STATE_COPY_DEST;
	// CrossQueueTransition: transition to COMMON so the resource can be read by a different queue type
	// without a barrier on the receiving queue (D3D12 implicit promotion from COMMON applies).
	if (targetAccessibility.IsCrossQueue())
		l_newState = D3D12_RESOURCE_STATE_COMMON;

	// Compute command lists cannot use PIXEL_SHADER_RESOURCE state in barriers.
	// Strip it from the target state; the source state must be compute-compatible
	// (caller is responsible for doing cross-queue transitions on Graphics).
	if (commandList->m_Type == GPUEngineType::Compute)
		l_newState &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

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

bool DX12FrameManagementService::TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	uint32_t frameIndex = GetCurrentFrame();

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

bool DX12FrameManagementService::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	if (!renderPass || !commandList || !mesh)
	{
		Log(Error, "Null parameters in DrawIndexedInstanced");
		return false;
	}

	auto* l_resource = AssetService::GetMeshAsset(mesh->m_Asset);
	if (!l_resource || l_resource->m_Residency != AssetResidency::Resident)
	{
		Log(Warning, "DX12FrameManagementService::DrawIndexedInstanced: mesh asset not resident");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
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

bool DX12FrameManagementService::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DrawInstanced");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 0, nullptr);
	l_commandList->IASetIndexBuffer(nullptr);
	l_commandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12FrameManagementService::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in Dispatch");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in Dispatch for render pass ", renderPass->m_InstanceName);
		return false;
	}

	l_commandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12FrameManagementService::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DispatchRays");
		return false;
	}

	if (!g_Engine->Get<GPUBufferResourceService>()->IsTLASReady())
	{
		Log(Warning, "DX12FrameManagementService::DispatchRays: TLAS not ready, skipping for ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);

	if (l_PSO->m_RaytracingMissShaderCount == 0 || l_PSO->m_RaytracingHitGroupCount == 0)
	{
		Log(Error, "DispatchRays: PSO for ", renderPass->m_InstanceName,
			" has missShaderCount=", l_PSO->m_RaytracingMissShaderCount,
			" hitGroupCount=", l_PSO->m_RaytracingHitGroupCount,
			" — any TraceRay() in this pass will read past the shader table. Skipping dispatch (TASK-35).");
		return false;
	}

	auto l_shaderIDBufferVirtualAddress = l_PSO->m_RaytracingShaderIDBuffer->GetGPUVirtualAddress();

	// Validate the shader-ID buffer actually holds the number of records the PSO recorded
	// at creation (TASK-35). Layout: [RayGen][Miss × missCount][HitGroup × hitCount].
	D3D12_RESOURCE_DESC l_bufDesc = l_PSO->m_RaytracingShaderIDBuffer->GetDesc();
	const uint32_t l_expectedSlots = 1u + l_PSO->m_RaytracingMissShaderCount + l_PSO->m_RaytracingHitGroupCount;
	const uint64_t l_expectedBytes = static_cast<uint64_t>(l_expectedSlots) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	if (l_bufDesc.Width < l_expectedBytes)
	{
		Log(Error, "DispatchRays: shader-ID buffer too small for ", renderPass->m_InstanceName,
			" expected ", l_expectedBytes, " bytes (", l_expectedSlots, " slots)",
			" got ", l_bufDesc.Width, " bytes. Skipping dispatch (TASK-35).");
		return false;
	}

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

	dispatchDesc.RayGenerationShaderRecord.StartAddress = l_shaderIDBufferVirtualAddress;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.MissShaderTable.StartAddress = l_shaderIDBufferVirtualAddress + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.SizeInBytes = static_cast<uint64_t>(l_PSO->m_RaytracingMissShaderCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	const uint64_t hitGroupOffset = static_cast<uint64_t>(1u + l_PSO->m_RaytracingMissShaderCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.HitGroupTable.StartAddress = l_shaderIDBufferVirtualAddress + hitGroupOffset;
	dispatchDesc.HitGroupTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.HitGroupTable.SizeInBytes = static_cast<uint64_t>(l_PSO->m_RaytracingHitGroupCount) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	dispatchDesc.Width = dimensionX;
	dispatchDesc.Height = dimensionY;
	dispatchDesc.Depth = dimensionZ;

	l_commandList->DispatchRays(&dispatchDesc);

	return true;
}

bool DX12FrameManagementService::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	if (!renderPass || !commandList || !indirectDrawCommand)
	{
		Log(Error, "Null parameters in ExecuteIndirect");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(indirectDrawCommand->m_DeviceMemories[GetCurrentFrame()]);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

	auto l_modelCount = (uint32_t)g_Engine->Get<DrawCallService>()->GetGPUModelData().size();
	uint32_t l_bufferCapacity = static_cast<uint32_t>(indirectDrawCommand->m_ElementCount);
	UINT maxDrawCommandCount = l_modelCount < l_bufferCapacity ? l_modelCount : l_bufferCapacity;

	if (maxDrawCommandCount == 0)
	{
		Log(Warning, "DX12FrameManagementService::ExecuteIndirect: zero draw commands for ", renderPass->m_InstanceName);
		return false;
	}

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadWrite, Accessibility::ReadOnly);

	l_commandList->ExecuteIndirect(l_PSO->m_IndirectCommandSignature.Get(), maxDrawCommandCount, l_deviceMemory->m_DefaultHeapBuffer.Get(), 0, nullptr, 0);

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadOnly, Accessibility::ReadWrite);

	return true;
}

void DX12FrameManagementService::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
	if (!renderPass || !commandList)
	{
		Log(Warning, "DX12FrameManagementService::PushRootConstants: null ", (!renderPass ? "renderPass" : "commandList"));
		return;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
}

bool DX12FrameManagementService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in CommandListEnd");
		return false;
	}

	// TASK-155 F2 cross-queue exit: when the producer pass declares its render targets
	// will next be read by a different queue type, drop them to COMMON at the end of the
	// producer CL (record-order = execute-order on a single queue, so the engine-side
	// m_CurrentState mutation matches what the GPU will see). Consumers on the receiving
	// queue then implicitly promote from COMMON without recording a barrier of their own.
	if (renderPass->m_RenderPassDesc.m_PostCLState == CrossQueueExit::ToCommon
		&& renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		ChangeRenderTargetStates(renderPass, commandList, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	}

	if (!Close(commandList, renderPass->m_RenderPassDesc.m_GPUEngineType))
	{
		Log(Error, "Failed to close command list for render pass ", renderPass->m_InstanceName);
		return false;
	}

	return true;
}

// --- Private command recording helpers ---

bool DX12FrameManagementService::BindComputeResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (!commandList)
	{
		Log(Error, "CommandList is null in BindComputeResource");
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in BindComputeResource");
		return false;
	}

	auto l_currentFrame = GetCurrentFrame();

	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
		{
			Log(Warning, "DX12FrameManagementService::BindComputeResource: null buffer at root param ", rootParameterIndex);
			return false;
		}

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
			if (resourceBindingLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_mappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
				l_commandList->SetComputeRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if (resourceBindingLayoutDesc.m_ResourceAccessibility.CanWrite())
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
			if (resourceBindingLayoutDesc.m_ResourceAccessibility.CanWrite())
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

bool DX12FrameManagementService::BindGraphicsResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
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

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "CommandList is null in BindGraphicsResource");
		return false;
	}

	auto l_currentFrame = GetCurrentFrame();

	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
		{
			Log(Warning, "DX12FrameManagementService::BindGraphicsResource: null buffer at root param ", rootParameterIndex);
			return false;
		}

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

bool DX12FrameManagementService::SetDescriptorHeaps(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (!l_commandList)
	{
		Log(Error, "Command list is null in SetDescriptorHeaps for render pass ", renderPass->m_InstanceName);
		return false;
	}

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get(), m_ctx->m_SamplerDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(2, l_heaps);

	return true;
}

bool DX12FrameManagementService::SetRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (!commandList || !commandList->m_CommandList)
	{
		Log(Error, "Command list is null in SetRenderTargets for render pass ", renderPass->m_InstanceName);
		return false;
	}

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	auto l_currentFrame = GetCurrentFrame();
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

bool DX12FrameManagementService::PreparePipeline(RenderPassComponent* renderPass, CommandListComponent* commandList, DX12PipelineStateObject* PSO)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

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

bool DX12FrameManagementService::ChangeRenderTargetStates(RenderPassComponent* renderPass, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
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

// --- Swap chain ---

bool DX12FrameManagementService::CreateSwapChainResources()
{
    if (!g_Engine->getInitConfig().isOffscreen)
    {
        return CreateSwapChain();
    }
    return true;
}

bool DX12FrameManagementService::CreateSwapChain()
{
    m_swapChainDesc.BufferCount = m_swapChainImageCount;

    auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

    m_swapChainDesc.Width = (UINT)l_screenResolution.x;
    m_swapChainDesc.Height = (UINT)l_screenResolution.y;

    m_swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

    m_swapChainDesc.SampleDesc.Count = 1;
    m_swapChainDesc.SampleDesc.Quality = 0;

    // @TODO: finish this feature
    m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    m_swapChainDesc.Flags = 0;

    m_swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    auto l_windowService = g_Engine->getWindowService();
    auto l_winWindowService = dynamic_cast<WinWindowService*>(l_windowService);
    if (!l_winWindowService)
    {
        Log(Error, "CreateSwapChain: Window service is not a WinWindowService! Can't create swap chain for HWND.");
        return false;
    }

    IDXGISwapChain1* l_swapChain1;
    auto l_hResult = m_ctx->m_factory->CreateSwapChainForHwnd(
        m_ctx->m_directCommandQueue.Get(),
        l_winWindowService->GetWindowHandle(),
        &m_swapChainDesc,
        nullptr,
        nullptr,
        &l_swapChain1);

    l_hResult = l_swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
    l_swapChain1->Release();

    if (FAILED(l_hResult))
    {
        Log(Error, "Can't create swap chain!");
        m_ObjectStatus = ObjectStatus::Suspended;
        return false;
    }

    Log(Success, "Swap chain has been created.");

    return true;
}

// --- Frame lifecycle ---

bool DX12FrameManagementService::GetSwapChainImages()
{
    Log(Verbose, "GetSwapChainImages: Called with offscreen=", g_Engine->getInitConfig().isOffscreen);

    if (g_Engine->getInitConfig().isOffscreen)
    {
        Log(Verbose, "GetSwapChainImages: Skipping in offscreen mode");
        return true;
    }

    if (!m_swapChain)
    {
        Log(Error, "GetSwapChainImages: m_swapChain is null! This should not happen in windowed mode.");
        return false;
    }

    m_swapChainImages.resize(m_swapChainImageCount);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        auto l_HResult = m_swapChain->GetBuffer((uint32_t)i, IID_PPV_ARGS(&m_swapChainImages[i]));
        if (FAILED(l_HResult))
        {
            auto l_drr = m_ctx->m_device ? m_ctx->m_device->GetDeviceRemovedReason() : S_OK;
            Log(Error, "Can't get pointer of swap chain image ", i,
                " HRESULT=", static_cast<int32_t>(l_HResult),
                " DeviceRemovedReason=", static_cast<int32_t>(l_drr));
            return false;
        }
        m_swapChainImages[i]->SetName((L"SwapChainBackBuffer_" + std::to_wstring(i)).c_str());
    }

    return true;
}

bool DX12FrameManagementService::AssignSwapChainImages()
{
    if (g_Engine->getInitConfig().isOffscreen)
    {
        return true;
    }

    if (!m_SwapChainRenderPassComp)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: m_SwapChainRenderPassComp is null");
        return false;
    }

    auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
    if (!l_outputMergerTarget)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: OutputMergerTarget is null");
        return false;
    }

    auto l_textureComp = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);
    if (!l_textureComp)
    {
        Log(Warning, "DX12FrameManagementService::AssignSwapChainImages: swap chain color output texture is null");
        return false;
    }

    l_textureComp->m_GPUResources.resize(m_swapChainImageCount);
    l_textureComp->m_CurrentState.resize(m_swapChainImageCount, D3D12_RESOURCE_STATE_PRESENT);

    for (size_t i = 0; i < m_swapChainImageCount; i++)
    {
        l_textureComp->m_GPUResources[i] = m_swapChainImages[i].Get();
    }

    auto l_textureDesc = m_swapChainImages[0]->GetDesc();
    l_textureComp->m_TextureDesc.Width = l_textureDesc.Width;
    l_textureComp->m_TextureDesc.Height = l_textureDesc.Height;
    l_textureComp->m_TextureDesc.IsMultiBuffer = true;
    l_textureComp->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_PRESENT);
    l_textureComp->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_RENDER_TARGET);
    l_textureComp->m_ObjectStatus = ObjectStatus::Activated;

    m_CurrentFrame = m_swapChain->GetCurrentBackBufferIndex();

    m_SwapChainRenderPassComp->m_CurrentFrame = m_CurrentFrame;

    return true;
}

bool DX12FrameManagementService::ReleaseSwapChainImages()
{
    return true;
}

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
