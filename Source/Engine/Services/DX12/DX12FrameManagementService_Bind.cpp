#include "DX12FrameManagementService.h"
#include "../../Component/GPUResourceCast.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_BindlessMesh.h"

using namespace Inno;
using namespace DX12Helper;

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
		if (resourceBindingLayoutDesc.m_IsRootConstant) return true;
		if (DX12Helper_BindlessMesh::TryAutoBindCompute(commandList, resourceBindingLayoutDesc.m_GPUBufferUsage, *m_ctx, rootParameterIndex)) return true;
		auto l_buffer = resource->As<GPUBufferComponent>();
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
			auto l_image = resource->As<TextureComponent>();
			if (!l_image) return false;
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
		auto l_sampler = resource->As<SamplerComponent>();
		if (!l_sampler) return false;
		auto l_handle = l_sampler->m_ReadHandles[l_currentFrame].m_GPUHandle;
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

		auto l_buffer = resource->As<GPUBufferComponent>();
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
			auto l_image = resource->As<TextureComponent>();
			if (!l_image) return false;
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
		auto l_sampler = resource->As<SamplerComponent>();
		if (!l_sampler) return false;
		auto l_handle = l_sampler->m_ReadHandles[l_currentFrame].m_GPUHandle;
		l_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ l_handle });
		return true;
	}

	assert(false);
	return false;
}
