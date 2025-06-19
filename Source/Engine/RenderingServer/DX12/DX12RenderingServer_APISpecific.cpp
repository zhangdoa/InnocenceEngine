#include "DX12RenderingServer.h"
#include "../../Engine.h"

using namespace Inno;

ComPtr<ID3D12Device8> DX12RenderingServer::GetDevice()
{
	return m_device.Get();
}

ComPtr<ID3D12CommandAllocator> DX12RenderingServer::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_currentFrame = GetCurrentFrame();
	switch (commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_directCommandAllocators[l_currentFrame];
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_computeCommandAllocators[l_currentFrame];
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_copyCommandAllocators[l_currentFrame];
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

ComPtr<ID3D12GraphicsCommandList7> DX12RenderingServer::GetGlobalCommandList(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_currentFrame = GetCurrentFrame();	
	auto l_commandList = reinterpret_cast<DX12CommandList*>(m_GlobalCommandLists[l_currentFrame]);
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
			case TextureUsage::ComputeOnly:
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
			case TextureUsage::ComputeOnly:
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