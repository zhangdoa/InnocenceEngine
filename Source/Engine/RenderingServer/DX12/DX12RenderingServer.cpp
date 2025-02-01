#include "DX12RenderingServer.h"

#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"

#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"

#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12RenderingServer::Update()
{
	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::ClearGPUBufferComponent(GPUBufferComponent* rhs)
{
	const uint32_t zero = 0;
	auto l_rhs = reinterpret_cast<DX12GPUBufferComponent*>(rhs);

	auto l_commandList = GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	l_commandList->ClearUnorderedAccessViewUint(
		l_rhs->m_UAV.Handle.GPUHandle,
		l_rhs->m_UAV.Handle.CPUHandle,
		l_rhs->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::ClearTextureComponent(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);

	auto l_commandList = GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	ID3D12DescriptorHeap* l_heaps[] = { m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	if (l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	if (l_rhs->m_TextureDesc.PixelDataType < TexturePixelDataType::Float16)
	{
		l_commandList->ClearUnorderedAccessViewUint(
			l_rhs->m_UAV.Handle.GPUHandle,
			l_rhs->m_UAV.Handle.CPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			(UINT*)&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}
	else
	{
		l_commandList->ClearUnorderedAccessViewFloat(
			l_rhs->m_UAV.Handle.GPUHandle,
			l_rhs->m_UAV.Handle.CPUHandle,
			l_rhs->m_DefaultHeapBuffer.Get(),
			&l_rhs->m_TextureDesc.ClearColor[0],
			0,
			NULL);
	}

	if (l_rhs->m_CurrentState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, l_rhs->m_CurrentState));

	ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

// @TODO: The command list should be passed as a parameter.
bool DX12RenderingServer::CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs)
{
	auto l_commandList = GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	l_commandList->Reset(GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	auto l_src = reinterpret_cast<DX12TextureComponent*>(lhs);
	auto l_dest = reinterpret_cast<DX12TextureComponent*>(rhs);

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), l_src->m_CurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), l_dest->m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST));

	l_commandList->CopyResource(l_dest->m_DefaultHeapBuffer.Get(), l_src->m_DefaultHeapBuffer.Get());

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_src->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, l_src->m_CurrentState));

	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_dest->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_dest->m_CurrentState));

	ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	return true;
}

uint32_t DX12RenderingServer::GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
	if (bindingAccessibility == Accessibility::ReadOnly)
		return l_rhs->m_SRV.Handle.m_Index;
	else if (bindingAccessibility.CanWrite())
		return l_rhs->m_UAV.Handle.m_Index;

	return 0;
}

bool DX12RenderingServer::CommandListBegin(RenderPassComponent* rhs, size_t frameIndex)
{
	if (rhs->m_RenderPassDesc.m_UseMultiFrames)
		rhs->m_CurrentFrame = GetCurrentFrame();
	else
		rhs->m_CurrentFrame = frameIndex;

	auto l_commandList = reinterpret_cast<DX12CommandList*>(rhs->m_CommandLists[rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(rhs->m_PipelineStateObject);

	Open(l_commandList, GPUEngineType::Graphics, l_PSO);
	Open(l_commandList, GPUEngineType::Compute, l_PSO);

	return true;
}

bool DX12RenderingServer::BindRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
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
	if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics && rhs->m_RenderPassDesc.m_RenderTargetCount)
	{
		auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
		auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
		auto f_clearRTAsUAV = [&](DX12TextureComponent* l_RT)
			{
				if (l_rhs->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType < TexturePixelDataType::Float16)
				{
					l_commandList->m_DirectCommandList->ClearUnorderedAccessViewUint(
						l_RT->m_UAV.Handle.GPUHandle,
						l_RT->m_UAV.Handle.CPUHandle,
						l_RT->m_DefaultHeapBuffer.Get(),
						(UINT*)l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
						0,
						NULL);
				}
				else
				{
					l_commandList->m_DirectCommandList->ClearUnorderedAccessViewFloat(
						l_RT->m_UAV.Handle.GPUHandle,
						l_RT->m_UAV.Handle.CPUHandle,
						l_RT->m_DefaultHeapBuffer.Get(),
						l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor,
						0,
						NULL);
				}
			};

		if (l_rhs->m_RenderPassDesc.m_UseOutputMerger)
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[l_rhs->m_CurrentFrame], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
			}
			else
			{
				if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[index], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
				}
				else
				{
					for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
					{
						l_commandList->m_DirectCommandList->ClearRenderTargetView(l_rhs->m_RTVDescCPUHandles[i], l_rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor, 0, nullptr);
					}
				}
			}
		}
		else
		{
			if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
			{
				f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(l_rhs->m_RenderTargets[l_rhs->m_CurrentFrame].m_Texture));
			}
			else
			{
				if (index != -1 && index < l_rhs->m_RenderPassDesc.m_RenderTargetCount)
				{
					f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(index));
				}
				else
				{

					for (auto i : l_rhs->m_RenderTargets)
					{
						f_clearRTAsUAV(reinterpret_cast<DX12TextureComponent*>(i.m_Texture));
					}
				}
			}
		}

		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_flag = D3D12_CLEAR_FLAG_DEPTH;
			if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite)
			{
				l_flag |= D3D12_CLEAR_FLAG_STENCIL;
			}
			l_commandList->m_DirectCommandList->ClearDepthStencilView(l_rhs->m_DSVDescCPUHandle, l_flag, 1.0f, 0x00, 0, nullptr);
		}
	}

	return true;
}

bool DX12RenderingServer::BindComputeResource(DX12CommandList* commandList, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc, GPUResourceComponent* resource)
{
	if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
	{
		if (resourceBindingLayoutDesc.m_IsRootConstant)
			return true;

		auto l_buffer = reinterpret_cast<DX12GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto& l_SRV = l_buffer->m_SRV;
		auto& l_UAV = l_buffer->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_buffer->m_UploadHeapBuffer->GetGPUVirtualAddress();
				commandList->m_ComputeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_SRV.Handle.GPUHandle);
				return true;
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_UAV.Handle.GPUHandle);
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
			commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_textureDescHeapAccessor.GetFirstHandle().GPUHandle);
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment)
		{
			auto l_image = reinterpret_cast<DX12TextureComponent*>(resource);
			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_image->m_UAV.Handle.GPUHandle);
			else
				commandList->m_ComputeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_image->m_SRV.Handle.GPUHandle);
		}
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<DX12SamplerComponent*>(resource)->m_Sampler.Handle.GPUHandle;
		commandList->m_DirectCommandList->SetComputeRootDescriptorTable(rootParameterIndex, l_handle);
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

		auto l_buffer = reinterpret_cast<DX12GPUBufferComponent*>(resource);
		if (!l_buffer)
			return false;

		if (l_buffer->m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto& l_SRV = l_buffer->m_SRV;
		auto& l_UAV = l_buffer->m_UAV;
		if (resourceBindingLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
		{
			if (l_buffer->m_GPUAccessibility == Accessibility::ReadOnly)
			{
				auto l_GPUVirtualAddress = l_buffer->m_UploadHeapBuffer->GetGPUVirtualAddress();
				commandList->m_DirectCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, l_GPUVirtualAddress);
				return true;
			}
			else if ((l_buffer->m_GPUAccessibility.CanWrite()))
			{
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_SRV.Handle.GPUHandle);
				return true;
			}
		}
		else if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
		{
			if (l_buffer->m_GPUAccessibility.CanWrite())
			{
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_UAV.Handle.GPUHandle);
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
			commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_textureDescHeapAccessor.GetFirstHandle().GPUHandle);
		}
		else if (resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBindingLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment)
		{
			auto l_image = reinterpret_cast<DX12TextureComponent*>(resource);
			if (resourceBindingLayoutDesc.m_BindingAccessibility.CanWrite())
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_image->m_UAV.Handle.GPUHandle);
			else
				commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_image->m_SRV.Handle.GPUHandle);
		}
		return true;
	}
	else if (resourceBindingLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		auto l_handle = reinterpret_cast<DX12SamplerComponent*>(resource)->m_Sampler.Handle.GPUHandle;
		commandList->m_DirectCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, l_handle);
		return true;
	}

	assert(false);
	return false;
}

bool DX12RenderingServer::Bind(RenderPassComponent* renderPass, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_GPUResourceType = resourceBindingLayoutDesc.m_GPUResourceType;

	// @TODO: Finish this
	return false;
}

bool DX12RenderingServer::BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (!renderPass)
		return false;

	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (shaderStage == ShaderStage::Compute)
	{
		return BindComputeResource(l_commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);
	}
	else
	{
		return BindGraphicsResource(l_commandList, resourceBindingLayoutDescIndex, l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex], resource);
	}

	return false;
}

void DX12RenderingServer::PushRootConstants(RenderPassComponent* rhs, size_t rootConstants)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_commandList->m_DirectCommandList->SetGraphicsRoot32BitConstants(0, 1, &rootConstants, 0);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandList->m_ComputeCommandList->SetComputeRoot32BitConstants(0, 1, &rootConstants, 0);
	}
}

bool DX12RenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<DX12MeshComponent*>(mesh);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, &l_mesh->m_VBV);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(&l_mesh->m_IBV);
	l_commandList->m_DirectCommandList->DrawIndexedInstanced((uint32_t)l_mesh->m_IndexCount, (uint32_t)instanceCount, 0, 0, 0);

	return true;
}

bool DX12RenderingServer::DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<DX12RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(l_renderPass->m_PipelineStateObject);

	l_commandList->m_DirectCommandList->IASetPrimitiveTopology(l_PSO->m_PrimitiveTopology);
	l_commandList->m_DirectCommandList->IASetVertexBuffers(0, 1, nullptr);
	l_commandList->m_DirectCommandList->IASetIndexBuffer(nullptr);
	l_commandList->m_DirectCommandList->DrawInstanced(1, (uint32_t)instanceCount, 0, 0);

	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	ChangeRenderTargetStates(l_rhs, l_commandList, Accessibility::ReadOnly);

	Close(l_commandList, GPUEngineType::Graphics);
	Close(l_commandList, GPUEngineType::Compute);

	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return Execute(l_commandList, l_semaphore, GPUEngineType);
}

bool DX12RenderingServer::WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return Wait(l_semaphore, queueType, semaphoreType);
}

bool DX12RenderingServer::WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(rhs);
	auto l_semaphore = l_rhs == nullptr ? nullptr : reinterpret_cast<DX12Semaphore*>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	return WaitFence(l_semaphore, GPUEngineType);
}

bool DX12RenderingServer::TryToTransitState(TextureComponent* rhs, ICommandList* commandList, Accessibility accessibility)
{
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(rhs);
	auto l_newState = accessibility == Accessibility::ReadOnly ? l_rhs->m_ReadState : l_rhs->m_WriteState;
	if (l_rhs->m_CurrentState == l_newState)
		return false;

	auto l_commandList = reinterpret_cast<DX12CommandList*>(commandList);
	auto l_transition = CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), l_rhs->m_CurrentState, l_newState);
	l_commandList->m_DirectCommandList->ResourceBarrier(1, &l_transition);
	l_rhs->m_CurrentState = l_newState;
	return true;
}

bool DX12RenderingServer::Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<DX12RenderPassComponent*>(renderPass);
	auto l_commandList = reinterpret_cast<DX12CommandList*>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	l_commandList->m_ComputeCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

Vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
	// @TODO: Support different pixel data type
	auto l_rhs = reinterpret_cast<DX12TextureComponent*>(TextureComp);
	auto l_srcDesc = l_rhs->m_DefaultHeapBuffer->GetDesc();

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> l_footprints;
	l_footprints.resize(l_rhs->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap ? 6 : l_rhs->m_TextureDesc.DepthOrArraySize);

	GetDevice()->GetCopyableFootprints(&l_srcDesc, 0, (UINT)l_footprints.size(), 0, l_footprints.data(), NULL, NULL, NULL);

	if (!l_rhs->m_ReadBackHeapBuffer)
	{
		UINT64 bufferSize = 0;
		for (size_t i = 0; i < l_footprints.size(); ++i)
		{
			bufferSize += l_footprints[i].Footprint.RowPitch * l_footprints[i].Footprint.Height;
		}

		l_rhs->m_ReadBackHeapBuffer = CreateReadBackHeapBuffer(bufferSize);
#ifdef INNO_DEBUG
		SetObjectName(l_rhs, l_rhs->m_ReadBackHeapBuffer, "ReadBackHeap_Texture");
#endif // INNO_DEBUG
	}

	auto f_DefaultToReadbackHeap = [this](ComPtr<ID3D12Resource> l_defaultHeapBuffer, ComPtr<ID3D12Resource> l_readbackHeapBuffer, const std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, DXGI_FORMAT l_format, D3D12_RESOURCE_STATES currentState)
		{
			{
				auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));

				l_commandList->ResourceBarrier(
					1,
					&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
						currentState,
						D3D12_RESOURCE_STATE_COMMON));
				ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
			}

			for (size_t i = 0; i < footprints.size(); i++)
			{
				D3D12_TEXTURE_COPY_LOCATION l_srcLocation = {};
				l_srcLocation.pResource = l_defaultHeapBuffer.Get();
				l_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				l_srcLocation.SubresourceIndex = (UINT)i;

				D3D12_TEXTURE_COPY_LOCATION l_destLocation = {};
				l_destLocation.pResource = l_readbackHeapBuffer.Get();
				l_destLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				l_destLocation.PlacedFootprint = footprints[i];
				l_destLocation.PlacedFootprint.Footprint.Format = l_format;

				auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_COPY, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY));

				l_commandList->ResourceBarrier(
					1,
					&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
						D3D12_RESOURCE_STATE_COMMON,
						D3D12_RESOURCE_STATE_COPY_SOURCE));

				l_commandList->CopyTextureRegion(&l_destLocation, 0, 0, 0, &l_srcLocation, NULL);

				l_commandList->ResourceBarrier(
					1,
					&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
						D3D12_RESOURCE_STATE_COPY_SOURCE,
						D3D12_RESOURCE_STATE_COMMON));

				ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY));
			}

			{
				auto l_commandList = CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT));
				l_commandList->ResourceBarrier(
					1,
					&CD3DX12_RESOURCE_BARRIER::Transition(l_defaultHeapBuffer.Get(),
						D3D12_RESOURCE_STATE_COMMON,
						currentState));
				ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
			}
		};

	size_t l_pixelCount = 0;

	auto textureDesc = l_rhs->m_TextureDesc;
	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_pixelCount = textureDesc.Width;
		break;
	case TextureSampler::Sampler2D:
		l_pixelCount = textureDesc.Width * textureDesc.Height;
		break;
	case TextureSampler::Sampler3D:
		l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler1DArray:
		l_pixelCount = textureDesc.Width * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::Sampler2DArray:
		l_pixelCount = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize;
		break;
	case TextureSampler::SamplerCubemap:
		l_pixelCount = textureDesc.Width * textureDesc.Height * 6;
		break;
	default:
		break;
	}

	auto f_ReadbackToHostHeap = [](ComPtr<ID3D12Resource> l_readbackHeapBuffer, uint32_t l_pixelDataSize, size_t l_pixelCount) -> std::vector<unsigned char>
		{
			std::vector<unsigned char> l_result;
			l_result.resize(l_pixelCount * l_pixelDataSize);

			CD3DX12_RANGE m_ReadRange(0, l_result.size());
			void* l_pData;
			auto l_HResult = l_readbackHeapBuffer->Map(0, &m_ReadRange, &l_pData);

			if (FAILED(l_HResult))
			{
				Log(Error, "Can't map texture for CPU to read!");
			}

			std::memcpy(l_result.data(), l_pData, l_result.size());
			l_readbackHeapBuffer->Unmap(0, 0);

			return l_result;
		};

	// Copy from default heap to readback heap, then copy from readback heap to application's heap region
	DXGI_FORMAT l_format;
	std::vector<Vec4> l_result;

	if (textureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R8_TYPELESS for the stencil
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);
		auto l_pixelCount = l_rawResult.size() / 4;

		std::vector<uint32_t> l_resultUint32;
		l_resultUint32.resize(l_pixelCount);
		l_result.resize(l_pixelCount);

		std::memcpy(l_resultUint32.data(), l_rawResult.data(), l_rawResult.size());
		for (size_t i = 0; i < l_pixelCount; i++)
		{
			auto l_value = l_resultUint32[i];
			auto l_depth = l_value & 0x00FFFFFF;
			auto l_stencil = l_value & 0xFF000000;
			l_result[i].x = float(l_depth) / float(0x00FFFFFF);
			l_result[i].y = float(l_stencil);
		}
	}
	else
	{
		l_format = l_rhs->m_DX12TextureDesc.Format;
		f_DefaultToReadbackHeap(l_rhs->m_DefaultHeapBuffer, l_rhs->m_ReadBackHeapBuffer, l_footprints, l_format, l_rhs->m_CurrentState);
		auto l_rawResult = f_ReadbackToHostHeap(l_rhs->m_ReadBackHeapBuffer, l_rhs->m_PixelDataSize, l_pixelCount);

		l_result.resize(l_pixelCount);
		if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			for (int i = 0; i < l_pixelCount; ++i)
			{
				const unsigned char* pixelData = &l_rawResult[i * 8];

				// Convert the RGBA FLOAT16 data to float values
				float floatData[4];
				for (int j = 0; j < 4; ++j)
				{
					const unsigned char* floatBytes = &pixelData[j * 2];
					unsigned short float16;
					memcpy(&float16, floatBytes, sizeof(unsigned short));
					floatData[j] = Math::float16ToFloat32(float16);
				}

				// Construct a Vec4 from the float values
				l_result[i] = Vec4(floatData[0], floatData[1], floatData[2], floatData[3]);
			}
		}
		else
		{
			std::memcpy(l_result.data(), l_rawResult.data(), l_rawResult.size());
		}
	}

	return l_result;
}

// @TODO: This is expensive, it should be running entirely on the GPU
bool DX12RenderingServer::GenerateMipmap(TextureComponent* rhs)
{
	// auto l_rhs = reinterpret_cast<DX12TextureComponent *>(rhs);

	// if(l_rhs->m_TextureDesc.IsSRGB)
	// {
	// 	auto l_copy = reinterpret_cast<DX12TextureComponent*>(AddTextureComponent((l_rhs->m_InstanceName.c_str() + std::string("_MipCopy/")).c_str()));
	// 	l_copy->m_TextureDesc = l_rhs->m_TextureDesc;
	// 	l_copy->m_TextureData = l_rhs->m_TextureData;
	// 	l_copy->m_TextureDesc.IsSRGB = false;
	// 	InitializeTextureComponent(l_copy);

	// 	D3D12_RESOURCE_BARRIER barrier[2] = {};
	// 	barrier[0].Type = barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// 	barrier[0].Transition.Subresource = barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	// 	barrier[0].Transition.pResource = l_copy->m_DefaultHeapBuffer.Get();
	// 	barrier[0].Transition.StateBefore = l_copy->m_CurrentState;
	// 	barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

	// 	barrier[1].Transition.pResource = l_rhs->m_DefaultHeapBuffer.Get();
	// 	barrier[1].Transition.StateBefore = l_rhs->m_CurrentState;
	// 	barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

	// 	auto l_commandList = GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	// 	l_commandList->Reset(GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(), nullptr);

	// 	l_commandList->ResourceBarrier(2, barrier);

	// 	// Copy the entire resource back
	// 	l_commandList->CopyResource(l_rhs->m_DefaultHeapBuffer.Get(), l_copy->m_DefaultHeapBuffer.Get());
	// 	l_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(l_rhs->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_rhs->m_CurrentState));

	// 	ExecuteCommandListAndWait(l_commandList, GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	// 	return true;
	// }

	// return GenerateMipmapImpl(l_rhs);

	return true;
}

ComPtr<ID3D12Device8> DX12RenderingServer::GetDevice()
{
	return m_device.Get();
}

ComPtr<ID3D12CommandAllocator> DX12RenderingServer::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	switch (commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_directCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame];
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_computeCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame];
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_copyCommandAllocators[m_SwapChainRenderPassComp->m_CurrentFrame];
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
	default:
		throw std::runtime_error("Invalid command list type");
	}
}

ComPtr<ID3D12CommandQueue> DX12RenderingServer::GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	switch (commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_directCommandQueue;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_computeCommandQueue;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_copyCommandQueue;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
	default:
		throw std::runtime_error("Invalid command list type");
	}
}

ComPtr<ID3D12GraphicsCommandList> DX12RenderingServer::GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_commandList = reinterpret_cast<DX12CommandList*>(m_GlobalCommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]);
	switch (commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return l_commandList->m_DirectCommandList;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return l_commandList->m_ComputeCommandList;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return l_commandList->m_CopyCommandList;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
	default:
		throw std::runtime_error("Invalid command list type");
	}
}

DX12DescriptorHeapAccessor& DX12RenderingServer::GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility
	, Accessibility resourceAccessibility, TextureUsage textureUsage, bool isShaderVisible)
{
	if (type == GPUResourceType::Buffer)
	{
		if (bindingAccessibility == Accessibility::ReadOnly)
		{
			if (resourceAccessibility == Accessibility::ReadOnly)
				return m_GPUBuffer_CBV_DescHeapAccessor;
			else
				return m_GPUBuffer_SRV_DescHeapAccessor;
		}
		else if (bindingAccessibility.CanWrite())
		{
			if (!resourceAccessibility.CanWrite())
				Log(Error, "Trying to get the writable descriptor heap accessor for a non-writable buffer resource");

			return isShaderVisible ? m_GPUBuffer_UAV_DescHeapAccessor : m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible;
		}
	}
	else if (type == GPUResourceType::Image)
	{
		if (bindingAccessibility == Accessibility::ReadOnly)
		{
			switch (textureUsage)
			{
			case TextureUsage::Sample:
				return m_MaterialTexture_SRV_DescHeapAccessor;
			case TextureUsage::DepthAttachment:
			case TextureUsage::DepthStencilAttachment:
			case TextureUsage::ColorAttachment:
				return m_RenderTarget_SRV_DescHeapAccessor;
			case TextureUsage::Invalid:
			default:
			{
				assert(false);
				Log(Error, "Invalid texture sampler type.");
			}
			}
		}
		else if (bindingAccessibility.CanWrite())
		{
			switch (textureUsage)
			{
			case TextureUsage::Sample:
				return isShaderVisible ? m_MaterialTexture_UAV_DescHeapAccessor : m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible;
			case TextureUsage::DepthAttachment:
			case TextureUsage::DepthStencilAttachment:
			case TextureUsage::ColorAttachment:
				return isShaderVisible ? m_RenderTarget_UAV_DescHeapAccessor : m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible;
			case TextureUsage::Invalid:
			default:
			{
				assert(false);
				Log(Error, "Invalid texture sampler type.");
			}
			}
		}
	}
	else if (type == GPUResourceType::Sampler)
	{
		return m_SamplerDescHeapAccessor;
	}

	assert(false);
	return m_GPUBuffer_CBV_DescHeapAccessor;
}