#include "DX12Context.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

// --- Accessors ---

ComPtr<ID3D12CommandAllocator> DX12Context::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, uint32_t frameIndex)
{
	switch (commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_directCommandAllocators[frameIndex];
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_computeCommandAllocators[frameIndex];
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_copyCommandAllocators[frameIndex];
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
	default:
		throw std::runtime_error("Invalid command list type");
	}
}

ComPtr<ID3D12CommandQueue> DX12Context::GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
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

DX12DescriptorHeapAccessor& DX12Context::GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility,
	Accessibility resourceAccessibility, TextureUsage textureUsage, bool isShaderVisible)
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

// --- Object creation helpers ---

ComPtr<ID3D12Resource> DX12Context::CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, const char* name)
{
	ComPtr<ID3D12Resource> l_uploadHeapBuffer;

	auto l_heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto l_HResult = m_device->CreateCommittedResource(
		&l_heapProps,
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&l_uploadHeapBuffer));

	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "upload heap buffer", name, l_HResult);
		return nullptr;
	}

	return l_uploadHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Context::CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue, bool isShared, const char* name)
{
	ComPtr<ID3D12Resource> l_defaultHeapBuffer;

	auto l_heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto l_HResult = m_device->CreateCommittedResource(
		&l_heapProps,
		isShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&l_defaultHeapBuffer));

	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "default heap buffer", name, l_HResult);
		return nullptr;
	}

	return l_defaultHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Context::CreateReadBackHeapBuffer(UINT64 size, const char* name)
{
	ComPtr<ID3D12Resource> l_readBackHeapBuffer;

	auto l_heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	auto l_resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	auto l_HResult = m_device->CreateCommittedResource(
		&l_heapProps,
		D3D12_HEAP_FLAG_NONE,
		&l_resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&l_readBackHeapBuffer));

	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "read-back heap buffer", name, l_HResult);
		if (m_device->GetDeviceRemovedReason() != S_OK)
			DumpDRED(m_device.Get());
		return nullptr;
	}

	return l_readBackHeapBuffer;
}

ComPtr<ID3D12CommandQueue> DX12Context::CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC* commandQueueDesc, const wchar_t* name)
{
	ComPtr<ID3D12CommandQueue> l_commandQueue;

	auto l_HResult = m_device->CreateCommandQueue(commandQueueDesc, IID_PPV_ARGS(&l_commandQueue));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "CommandQueue", name, l_HResult);
		return nullptr;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	l_commandQueue->SetName(name);
#endif

	Log(Verbose, "CommandQueue: ", name, " has been created.");

	return l_commandQueue;
}

ComPtr<ID3D12CommandAllocator> DX12Context::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, const wchar_t* name)
{
	ComPtr<ID3D12CommandAllocator> l_commandAllocator;

	auto l_HResult = m_device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&l_commandAllocator));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "CommandAllocator", name, l_HResult);
		return nullptr;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	l_commandAllocator->SetName(name);
#endif

	Log(Success, "CommandAllocator: ", name, " has been created.");

	return l_commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList7> DX12Context::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t* name)
{
	ComPtr<ID3D12GraphicsCommandList7> l_commandList;

	auto l_HResult = m_device->CreateCommandList(0, commandListType, commandAllocator.Get(), NULL, IID_PPV_ARGS(&l_commandList));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "CommandList", name, l_HResult);
		return nullptr;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	l_commandList->SetName(name);
#endif

	Log(Success, "CommandList: ", name, " has been created.");

	return l_commandList;
}

ComPtr<ID3D12GraphicsCommandList7> DX12Context::CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator)
{
	static uint64_t index = 0;

	return CreateCommandList(commandListType, commandAllocator, (L"TemporaryCommandList_" + std::to_wstring(index++)).c_str());
// Descriptor-heap creation + DX12DescriptorHeapAccessor::GetNewHandle live in
// DX12Context_Descriptors.cpp (extracted to keep this file under the 300-line
// commit-guard cap).
}
