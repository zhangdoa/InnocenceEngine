#include "DX12RenderingServer.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::BindRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	ChangeRenderTargetStates(l_rhs, l_commandList, Accessibility::WriteOnly);
	SetDescriptorHeaps(l_rhs, l_commandList);
	SetRenderTargets(l_rhs, l_commandList);

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);
	PreparePipeline(l_rhs, l_commandList, l_PSO);

	if (l_rhs->m_CustomCommandsFunc)
		l_rhs->m_CustomCommandsFunc(l_commandList);

	return true;
}

bool DX12RenderingServer::ClearRenderTargets(RenderPassComponent* rhs, size_t index)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != GPUEngineType::Graphics)
		return true;

	if (rhs->m_RenderPassDesc.m_RenderTargetCount == 0)
		return true;

	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto f_clearRenderTargetAsUAV = [&](DX12DeviceMemory* l_RT)
		{
			if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle }, 
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(), 
					(UINT*)l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, NULL);
			}
			else
			{
				l_commandList->m_DirectCommandList->ClearUnorderedAccessViewFloat(
					D3D12_GPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_GPUHandle }, 
					D3D12_CPU_DESCRIPTOR_HANDLE{ l_RT->m_UAV.Handle.m_CPUHandle },
					l_RT->m_DefaultHeapBuffer.Get(), 
					l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, NULL);
			}
		};

	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(l_rhs->m_OutputMergerTarget);
	auto l_currentFrame = GetCurrentFrame();
	if (l_rhs->m_RenderPassDesc.m_UseOutputMerger)
	{
		auto l_RTV = l_outputMergerTarget->m_RTVs[l_currentFrame];
		if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
		{
			l_commandList->m_DirectCommandList->ClearRenderTargetView(l_RTV.m_Handles[index], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
		}
		else
		{
			for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				l_commandList->m_DirectCommandList->ClearRenderTargetView(l_RTV.m_Handles[i], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
		}
	}
	else
	{
		if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
		{
			//f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(l_outputMergerTarget->m_ColorOutputs[index]->m_DeviceMemories[l_currentFrame]));
		}
		else
		{
			for (auto i : l_outputMergerTarget->m_ColorOutputs)
			{
				//f_clearRenderTargetAsUAV(reinterpret_cast<DX12DeviceMemory*>(i->m_DeviceMemories[l_currentFrame]));
			}
		}
	}

	if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
	{
		auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			l_flag |= D3D12_CLEAR_FLAG_STENCIL;

		auto l_DSV = l_outputMergerTarget->m_DSVs[l_currentFrame];
		l_commandList->m_DirectCommandList->ClearDepthStencilView(l_DSV.m_Handle, l_flag, 1.0f, 0x00, 0, nullptr);
	}

	return true;
}

bool DX12RenderingServer::BindComputeResource(DX12CommandList* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_currentFrame = GetCurrentFrame();
		auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_buffer->m_MappedMemories[l_currentFrame]);
		auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_buffer->m_DeviceMemories[l_currentFrame]);
		auto& l_SRV = l_deviceMemory->m_SRV;
		auto& l_UAV = l_deviceMemory->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_mappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
				commandList->m_ComputeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				if (l_buffer->m_Usage == GPUBufferUsage::TLAS)
				{
					auto l_GPUVirtualAddress = l_deviceMemory->m_DefaultHeapBuffer->GetGPUVirtualAddress();
					commandList->m_ComputeCommandList->SetComputeRootShaderResourceView(rootParameterIndex, l_GPUVirtualAddress);
					return true;
				}
				else
				{
					commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_SRV.Handle.m_GPUHandle });
					return true;
				}
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_UAV.Handle.m_GPUHandle });
				return true;
			}
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::Sample)
		{
			auto& l_textureDescHeapAccessor = GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
				, resourceBindingLayoutDesc.m_ResourceAccessibility, TextureUsage::Sample);
			commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_textureDescHeapAccessor.GetFirstHandle().m_GPUHandle });
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment)
		{
			auto l_image = reinterpret_cast<TextureComponent*>(resource);
			if (l_image->m_ObjectStatus != ObjectStatus::Activated)
				return false;

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_WriteHandles[l_handleIndex].m_GPUHandle });
			else
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_ReadHandles[l_handleIndex].m_GPUHandle });
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[GetCurrentFrame()].m_GPUHandle;
		commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
	}

	assert(false);
	return false;
}

bool DX12RenderingServer::BindGraphicsResource(DX12CommandList* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_currentFrame = GetCurrentFrame();
		auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(l_buffer->m_MappedMemories[l_currentFrame]);
		auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(l_buffer->m_DeviceMemories[l_currentFrame]);
		auto& l_SRV = l_deviceMemory->m_SRV;
		auto& l_UAV = l_deviceMemory->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_mappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
				commandList->m_DirectCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_SRV.Handle.m_GPUHandle });
				return true;
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_UAV.Handle.m_GPUHandle });
				return true;
			}
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::Sample)
		{
			auto& l_textureDescHeapAccessor = GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
				, resourceBindingLayoutDesc.m_ResourceAccessibility, TextureUsage::Sample);
			commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_textureDescHeapAccessor.GetFirstHandle().m_GPUHandle });
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment)
		{
			auto l_image = reinterpret_cast<TextureComponent*>(resource);
			if (l_image->m_ObjectStatus != ObjectStatus::Activated)
				return false;

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;

			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_WriteHandles[l_handleIndex].m_GPUHandle });
			else
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_image->m_ReadHandles[l_handleIndex].m_GPUHandle });
		}
		return true;
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[GetCurrentFrame()].m_GPUHandle;
		commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
		return true;
	}

	assert(false);
	return false;
}

bool DX12RenderingServer::BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (!renderPass)
		return false;

	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		return BindComputeResource(l_commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);
	else
		return BindGraphicsResource(l_commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);

	return false;
}

bool DX12RenderingServer::TryToTransitState(TextureComponent* rhs, ICommandList* commandList, Accessibility accessibility)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(rhs->GetGPUResource(frameIndex));
	if (!resource)
		return false;

	auto l_textureDesc = GetDX12TextureDesc(rhs->m_TextureDesc);
	auto l_newState = accessibility == Accessibility::ReadOnly ? rhs->m_ReadState : rhs->m_WriteState;
	
	if (rhs->m_CurrentState == l_newState)
		return false;

	auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
		resource, 
		static_cast<D3D12_RESOURCE_STATES>(rhs->m_CurrentState), 
		static_cast<D3D12_RESOURCE_STATES>(l_newState));
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &l_transition);

	rhs->m_CurrentState = static_cast<uint32_t>(l_newState);
	return true;
}

bool DX12RenderingServer::UpdateIndirectDrawCommand(GPUBufferComponent* indirectDrawCommand, const std::vector<DrawCallInfo>& drawCallList, std::function<bool(const DrawCallInfo&)>&& isDrawCallValid)
{
	auto l_indirectDrawCommandList = reinterpret_cast<DX12IndirectDrawCommandList*>(indirectDrawCommand->m_IndirectDrawCommandList);
	auto l_drawCallCount = drawCallList.size();
	l_indirectDrawCommandList->m_CommandList.clear();
	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto& l_drawCall = drawCallList[i];
		if (!isDrawCallValid(l_drawCall))
			continue;

		auto l_mesh = l_drawCall.mesh;
		
		D3D12_VERTEX_BUFFER_VIEW vbv = {};
		vbv.BufferLocation = l_mesh->m_VertexBufferView.m_BufferLocation;
		vbv.StrideInBytes = l_mesh->m_VertexBufferView.m_StrideInBytes;
		vbv.SizeInBytes = l_mesh->m_VertexBufferView.m_SizeInBytes;

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.BufferLocation = l_mesh->m_IndexBufferView.m_BufferLocation;
		ibv.Format = DXGI_FORMAT_R32_UINT; // Index format
		ibv.SizeInBytes = l_mesh->m_IndexBufferView.m_SizeInBytes;

		DX12DrawIndirectCommand l_command;
		l_command.RootConstant = l_drawCall.m_PerObjectConstantBufferIndex;
		l_command.VBV = vbv;
		l_command.IBV = ibv;
		l_command.DrawIndexedArgs.BaseVertexLocation = 0;
		l_command.DrawIndexedArgs.IndexCountPerInstance = l_mesh->GetIndexCount();
		l_command.DrawIndexedArgs.StartIndexLocation = 0;
		l_command.DrawIndexedArgs.StartInstanceLocation = 0;
		l_command.DrawIndexedArgs.InstanceCount = 1;
		l_indirectDrawCommandList->m_CommandList.emplace_back(l_command);
	}

	auto l_mappedMemory = indirectDrawCommand->m_MappedMemories[GetCurrentFrame()];
	WriteMappedMemory(indirectDrawCommand, l_mappedMemory, l_indirectDrawCommandList->m_CommandList.data(), 0, l_indirectDrawCommandList->m_CommandList.size());

	return true;
}

bool DX12RenderingServer::ExecuteIndirect(RenderPassComponent* rhs, GPUBufferComponent* indirectDrawCommand)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(rhs->m_PipelineStateObject);
	auto l_indirectDrawCommandList = reinterpret_cast<DX12IndirectDrawCommandList*>(indirectDrawCommand->m_IndirectDrawCommandList);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(indirectDrawCommand->m_DeviceMemories[GetCurrentFrame()]);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

	UINT drawCommandCount = l_indirectDrawCommandList->m_CommandList.size();
	if (drawCommandCount == 0)
		return false;

	l_commandList->m_DirectCommandList->ExecuteIndirect(l_PSO->m_IndirectCommandSignature.Get(), drawCommandCount, l_deviceMemory->m_DefaultHeapBuffer.Get(), 0, nullptr, 0);

	return true;
}

void DX12RenderingServer::PushRootConstants(RenderPassComponent* rhs, size_t rootConstants)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList->m_DirectCommandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList->m_ComputeCommandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
}

bool DX12RenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = mesh->m_VertexBufferView.m_BufferLocation;
	vbv.StrideInBytes = mesh->m_VertexBufferView.m_StrideInBytes;
	vbv.SizeInBytes = mesh->m_VertexBufferView.m_SizeInBytes;

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = mesh->m_IndexBufferView.m_BufferLocation;
	ibv.Format = DXGI_FORMAT_R32_UINT; // Index format
	ibv.SizeInBytes = mesh->m_IndexBufferView.m_SizeInBytes;

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, &vbv);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(&ibv);
	l_commandList->m_DirectCommandList->DrawIndexedInstanced(mesh->GetIndexCount(), (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, nullptr);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(nullptr);
	l_commandList->m_DirectCommandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12RenderingServer::Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	l_commandList->m_ComputeCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12RenderingServer::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	auto l_currentFrame = GetCurrentFrame();
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(semaphore);
	auto l_isOnGlobalSemaphore = l_semaphore == l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
	{
		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_directCommandQueue->Signal(m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		m_computeCommandQueue->Signal(m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		UINT64 l_copyCommandFinishedSemaphore = l_globalSemaphore->m_CopyCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_CopyCommandQueueSemaphore = l_copyCommandFinishedSemaphore;

		m_copyCommandQueue->Signal(m_copyCommandQueueFence.Get(), l_copyCommandFinishedSemaphore);
	}

	return true;
}

bool DX12RenderingServer::WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t semaphoreValue = 0;
	auto l_currentFrame = GetCurrentFrame();
	auto l_semaphore = semaphore ? reinterpret_cast<DX12Semaphore*>(semaphore) : reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

	if (queueType == GPUEngineType::Graphics)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	}
	else if (queueType == GPUEngineType::Compute)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	}
	else if (queueType == GPUEngineType::Copy)
	{
		commandQueue = GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY).Get();
	}

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_directCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_computeCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_ComputeCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Copy)
	{
		fence = m_copyCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_CopyCommandQueueSemaphore;
	}

	if (commandQueue && fence)
	{
		commandQueue->Wait(fence, semaphoreValue);
	}

	return true;
}

bool DX12RenderingServer::Execute(ICommandList* commandList, GPUEngineType queueType)
{
	auto l_currentFrame = GetCurrentFrame();
	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);

	if (queueType == GPUEngineType::Graphics)
	{
		ID3D12CommandList* l_directCommandLists[] = { l_commandList->m_DirectCommandList.Get() };
		m_directCommandQueue->ExecuteCommandLists(1, l_directCommandLists);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		ID3D12CommandList* l_computeCommandLists[] = { l_commandList->m_ComputeCommandList.Get() };
		m_computeCommandQueue->ExecuteCommandLists(1, l_computeCommandLists);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		ID3D12CommandList* l_copyCommandLists[] = { l_commandList->m_CopyCommandList.Get() };
		m_copyCommandQueue->ExecuteCommandLists(1, l_copyCommandLists);
	}

	return true;
}

uint64_t DX12RenderingServer::GetSemaphoreValue(GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);

	if (queueType == GPUEngineType::Graphics)
		return l_semaphore->m_DirectCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Compute)
		return l_semaphore->m_ComputeCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Copy)
		return l_semaphore->m_CopyCommandQueueSemaphore;

	return 0;
}

bool DX12RenderingServer::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
	UINT64 l_semaphoreValue = 0;
	HANDLE* fenceEvent = nullptr;

	if (queueType == GPUEngineType::Graphics)
	{
		fenceEvent = &l_semaphore->m_DirectCommandQueueFenceEvent;
	}
	else if (queueType == GPUEngineType::Compute)
	{
		fenceEvent = &l_semaphore->m_ComputeCommandQueueFenceEvent;
	}
	else if (queueType == GPUEngineType::Copy)
	{
		fenceEvent = &l_semaphore->m_CopyCommandQueueFenceEvent;
	}

	if (queueType == GPUEngineType::Graphics)
	{
		if (m_directCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for DirectCommandQueueFence: ", semaphoreValue);
			m_directCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (m_computeCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for ComputeCommandQueueFence: ", semaphoreValue);
			m_computeCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (m_copyCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for CopyCommandQueueFence: ", semaphoreValue);
			m_copyCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			WaitForSingleObject(*fenceEvent, INFINITE);
		}
	}

	return true;
}

bool DX12RenderingServer::DispatchRays(RenderPassComponent* rhs, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	auto l_currentFrame = GetCurrentFrame();
	auto l_rhs = reinterpret_cast<RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_currentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_rhs->m_PipelineStateObject);
	
	auto l_shaderIDBufferVirtualAddress = l_PSO->m_RaytracingShaderIDBuffer->GetGPUVirtualAddress();

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.RayGenerationShaderRecord = {};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = l_shaderIDBufferVirtualAddress;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.MissShaderTable = {};
	dispatchDesc.MissShaderTable.StartAddress = l_shaderIDBufferVirtualAddress + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.HitGroupTable = {};
	dispatchDesc.HitGroupTable.StartAddress = l_shaderIDBufferVirtualAddress + 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	dispatchDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	dispatchDesc.Width = dimensionX;
	dispatchDesc.Height = dimensionY;
	dispatchDesc.Depth = dimensionZ;

	l_commandList->m_ComputeCommandList->DispatchRays(&dispatchDesc);

	return true;
}