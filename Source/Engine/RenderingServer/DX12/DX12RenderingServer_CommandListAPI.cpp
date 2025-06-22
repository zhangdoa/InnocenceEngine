#include "DX12RenderingServer.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Texture.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::CommandListBegin(RenderPassComponent* renderPassComp, CommandListComponent* commandList, size_t frameIndex)
{
	if (!commandList || !renderPassComp)
	{
		Log(Error, "DX12RenderingServer::CommandListBegin: Invalid parameters");
		return false;
	}
		
	return Open(commandList, commandList->m_Type, renderPassComp->m_PipelineStateObject);;
}



bool DX12RenderingServer::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindRenderPassComponent");
		return false;
	}
	
	// Cast once for readability
	auto l_dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	if (!l_dx12CommandList)
	{
		Log(Error, "Invalid DX12 command list in BindRenderPassComponent");
		return false;
	}

	ChangeRenderTargetStates(renderPass, commandList, Accessibility::WriteOnly);
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

bool DX12RenderingServer::ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index)
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

	// Cast once for readability
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

bool DX12RenderingServer::BindComputeResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
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
			auto& l_textureDescHeapAccessor = GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
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

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
			
			// Note: Resource state transitions should be handled by the rendering client
			// using direct command lists before compute passes begin
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
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[GetCurrentFrame()].m_GPUHandle;
		l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
	}

	assert(false);
	return false;
}

bool DX12RenderingServer::BindGraphicsResource(CommandListComponent* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
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
			auto& l_textureDescHeapAccessor = GetDescriptorHeapAccessor(GPUResourceType::Image, resourceBindingLayoutDesc.m_BindingAccessibility
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

			auto l_handleIndex = l_image->m_TextureDesc.IsMultiBuffer ? GetCurrentFrame() : 0;
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
		auto l_handle = reinterpret_cast<SamplerComponent*>(resource)->m_ReadHandles[GetCurrentFrame()].m_GPUHandle;
		l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
		return true;
	}

	assert(false);
	return false;
}

bool DX12RenderingServer::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in BindGPUResource");
		return false;
	}

	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		return BindComputeResource(commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);
	else
		return BindGraphicsResource(commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);

	return false;
}

bool DX12RenderingServer::TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility accessibility)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto* resource = static_cast<ID3D12Resource*>(texture->GetGPUResource(frameIndex));
	if (!resource)
	{
		Log(Error, "No resource found for ", texture->m_InstanceName);
		return false;
	}

	auto l_textureDesc = GetDX12TextureDesc(texture->m_TextureDesc);
	auto l_newState = accessibility.CanWrite() ? texture->m_WriteState : texture->m_ReadState;

	if (texture->m_CurrentState == l_newState)
		return false;

	// Check if the resource states are valid for the command list type
	auto l_currentState = static_cast<D3D12_RESOURCE_STATES>(texture->m_CurrentState);
	auto l_targetState = static_cast<D3D12_RESOURCE_STATES>(l_newState);
	
	// Compute command lists have restrictions on which resource states they can use
	if (commandList->m_Type == GPUEngineType::Compute)
	{
		// States not allowed on compute command lists
		const D3D12_RESOURCE_STATES invalidComputeStates = 
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
			D3D12_RESOURCE_STATE_INDEX_BUFFER |
			D3D12_RESOURCE_STATE_RENDER_TARGET |
			D3D12_RESOURCE_STATE_DEPTH_WRITE |
			D3D12_RESOURCE_STATE_DEPTH_READ |
			D3D12_RESOURCE_STATE_STREAM_OUT |
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		
		if ((l_currentState & invalidComputeStates) || (l_targetState & invalidComputeStates))
		{
			Log(Warning, "Skipping resource state transition for ", texture->m_InstanceName, 
				" on compute command list. Current state: 0x", std::hex, (uint32_t)l_currentState, 
				", target state: 0x", (uint32_t)l_targetState, std::dec, 
				". Resource state transitions with graphics-specific states should be done on direct command lists.");
			return false;
		}
	}

	auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
		resource, 
		l_currentState, 
		l_targetState);

	l_commandList->ResourceBarrier(1, &l_transition);

	// @TODO: State should only be updated after command list execution, not during recording
	texture->m_CurrentState = static_cast<uint32_t>(l_newState);
	return true;
}

bool DX12RenderingServer::TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility accessibility)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	uint32_t frameIndex = GetCurrentFrame();

	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[frameIndex]);
	if (!l_deviceMemory || !l_deviceMemory->m_DefaultHeapBuffer)
		return false;

	auto l_newState = accessibility.CanWrite() ? gpuBuffer->m_WriteState : gpuBuffer->m_ReadState;
	
	if (gpuBuffer->m_CurrentState == l_newState)
		return false;

	auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(
		l_deviceMemory->m_DefaultHeapBuffer.Get(),
		static_cast<D3D12_RESOURCE_STATES>(gpuBuffer->m_CurrentState),
		static_cast<D3D12_RESOURCE_STATES>(l_newState));

	// Resource state transitions use the command list that was passed in
	// The caller is responsible for ensuring proper synchronization between different queue types
	l_commandList->ResourceBarrier(1, &l_transition);

	// @TODO: State should only be updated after command list execution, not during recording
	gpuBuffer->m_CurrentState = static_cast<uint32_t>(l_newState);
	return true;
}


bool DX12RenderingServer::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	if (!renderPass || !commandList || !indirectDrawCommand)
	{
		Log(Error, "Null parameters in ExecuteIndirect");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(indirectDrawCommand->m_DeviceMemories[GetCurrentFrame()]);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);

	// Get actual model count from RenderingContextService instead of buffer capacity
	auto l_renderingContextService = g_Engine->Get<RenderingContextService>();
	auto& l_gpuModelData = l_renderingContextService->GetGPUModelData();
	UINT maxDrawCommandCount = static_cast<UINT>(l_gpuModelData.size());
	
	if (maxDrawCommandCount == 0)
		return false;

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadOnly);

	l_commandList->ExecuteIndirect(l_PSO->m_IndirectCommandSignature.Get(), maxDrawCommandCount, l_deviceMemory->m_DefaultHeapBuffer.Get(), 0, nullptr, 0);

	TryToTransitState(indirectDrawCommand, commandList, Accessibility::ReadWrite);

	return true;
}

void DX12RenderingServer::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
	if (!renderPass || !commandList)
		return;

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
		l_commandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
		l_commandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
}

bool DX12RenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	if (!renderPass || !commandList || !mesh)
	{
		Log(Error, "Null parameters in DrawIndexedInstanced");
		return false;
	}

	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = mesh->m_VertexBufferView.m_BufferLocation;
	vbv.StrideInBytes = mesh->m_VertexBufferView.m_StrideInBytes;
	vbv.SizeInBytes = mesh->m_VertexBufferView.m_SizeInBytes;

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = mesh->m_IndexBufferView.m_BufferLocation;
	ibv.Format = DXGI_FORMAT_R32_UINT; // Index format
	ibv.SizeInBytes = mesh->m_IndexBufferView.m_SizeInBytes;

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 1, &vbv);
	l_commandList->IASetIndexBuffer(&ibv);
	l_commandList->DrawIndexedInstanced(mesh->GetIndexCount(), (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DrawInstanced");
		return false;
	}

	auto l_renderPass = reinterpret_cast<RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);

	l_commandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->IASetVertexBuffers(0, 1, nullptr);
	l_commandList->IASetIndexBuffer(nullptr);
	l_commandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12RenderingServer::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in Dispatch");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (!reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList))
	{
		Log(Error, "CommandList is null in Dispatch for render pass ", renderPass->m_InstanceName);
		return false;
	}

	l_commandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null render pass or command list in CommandListEnd");
		return false;
	}

	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	if (!Close(commandList, renderPass->m_RenderPassDesc.m_GPUEngineType))
	{
		Log(Error, "Failed to close command list for render pass ", renderPass->m_InstanceName);
		return false;
	}
	
	return true;
}

bool DX12RenderingServer::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	if (!m_GlobalSemaphore)
	{
		Log(Error, "Global semaphore is null in SignalOnGPU");
		return false;
	}
	
	auto l_currentFrame = GetCurrentFrame();
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(m_GlobalSemaphore);
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(semaphore);
	auto l_isOnGlobalSemaphore = l_semaphore == l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_directCommandQueue || !m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueue or DirectCommandQueueFence is null in SignalOnGPU");
			return false;
		}
		
		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_directCommandQueue->Signal(m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_computeCommandQueue || !m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueue or ComputeCommandQueueFence is null in SignalOnGPU");
			return false;
		}
		
		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		m_computeCommandQueue->Signal(m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_copyCommandQueue || !m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueue or CopyCommandQueueFence is null in SignalOnGPU");
			return false;
		}
		
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

bool DX12RenderingServer::Execute(CommandListComponent* commandList, GPUEngineType queueType)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	ID3D12CommandList* l_commandListToExecute[] = { l_commandList };
	
	if (queueType == GPUEngineType::Graphics)
	{
		m_directCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		m_computeCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		m_copyCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
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

	// Validate fence event handle
	if (!fenceEvent || *fenceEvent == nullptr)
	{
		Log(Error, "Invalid fence event handle in WaitOnCPU for queue type: ", (uint32_t)queueType);
		return false;
	}

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueueFence is null in WaitOnCPU");
			return false;
		}
		
		if (m_directCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for DirectCommandQueueFence: ", semaphoreValue);
			m_directCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			
			// Double-check to avoid race condition where GPU completes between check and SetEventOnCompletion
			if (m_directCommandQueueFence->GetCompletedValue() < semaphoreValue)
			{
				DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000); // 30 second timeout
				if (waitResult == WAIT_TIMEOUT)
				{
					//Log(Error, "DirectCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue, 
					//	", Completed value: ", m_directCommandQueueFence->GetCompletedValue());
					return false;
				}
				else if (waitResult != WAIT_OBJECT_0)
				{
					//Log(Error, "DirectCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
					return false;
				}
			}
		}
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueueFence is null in WaitOnCPU");
			return false;
		}
		
		if (m_computeCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for ComputeCommandQueueFence: ", semaphoreValue);
			m_computeCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			
			// Double-check to avoid race condition
			if (m_computeCommandQueueFence->GetCompletedValue() < semaphoreValue)
			{
				DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000); // 30 second timeout
				if (waitResult == WAIT_TIMEOUT)
				{
					Log(Error, "ComputeCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue, 
						", Completed value: ", m_computeCommandQueueFence->GetCompletedValue());
					return false;
				}
				else if (waitResult != WAIT_OBJECT_0)
				{
					Log(Error, "ComputeCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
					return false;
				}
			}
		}
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueueFence is null in WaitOnCPU");
			return false;
		}
		
		if (m_copyCommandQueueFence->GetCompletedValue() < semaphoreValue)
		{
			//Log(Verbose, "Waiting for CopyCommandQueueFence: ", semaphoreValue);
			m_copyCommandQueueFence->SetEventOnCompletion(semaphoreValue, *fenceEvent);
			
			// Double-check to avoid race condition
			if (m_copyCommandQueueFence->GetCompletedValue() < semaphoreValue)
			{
				DWORD waitResult = WaitForSingleObject(*fenceEvent, 30000); // 30 second timeout
				if (waitResult == WAIT_TIMEOUT)
				{
					Log(Error, "CopyCommandQueueFence wait timeout! Semaphore value: ", semaphoreValue, 
						", Completed value: ", m_copyCommandQueueFence->GetCompletedValue());
					return false;
				}
				else if (waitResult != WAIT_OBJECT_0)
				{
					Log(Error, "CopyCommandQueueFence wait failed with error: ", static_cast<uint32_t>(GetLastError()));
					return false;
				}
			}
		}
	}

	return true;
}

bool DX12RenderingServer::DispatchRays(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t dimensionX, uint32_t dimensionY, uint32_t dimensionZ)
{
	if (!renderPass || !commandList)
	{
		Log(Error, "Null parameters in DispatchRays");
		return false;
	}

	auto l_currentFrame = GetCurrentFrame();
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	
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

	l_commandList->DispatchRays(&dispatchDesc);

	return true;
}