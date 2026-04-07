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

	auto l_HResult = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&l_uploadHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create upload heap buffer ", name);
		return nullptr;
	}

	return l_uploadHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Context::CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue, bool isShared, const char* name)
{
	ComPtr<ID3D12Resource> l_defaultHeapBuffer;

	auto l_HResult = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		isShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&l_defaultHeapBuffer));

	if (FAILED(l_HResult))
	{
		auto l_removeReason = m_device->GetDeviceRemovedReason();
		Log(Error, "Can't create default heap buffer ", name,
			" HRESULT=", static_cast<int32_t>(l_HResult),
			" DeviceRemovedReason=", static_cast<int32_t>(l_removeReason));
		return nullptr;
	}

	return l_defaultHeapBuffer;
}

ComPtr<ID3D12Resource> DX12Context::CreateReadBackHeapBuffer(UINT64 size, const char* name)
{
	ComPtr<ID3D12Resource> l_readBackHeapBuffer;

	auto l_HResult = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&l_readBackHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Warning, "Can't create read-back heap buffer ", name);
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
		Log(Error, "Can't create CommandQueue: ", name);
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
		Log(Error, "Can't create CommandAllocator: ", name);
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
		Log(Error, "Can't create CommandList ", name);
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
}

ComPtr<ID3D12DescriptorHeap> DX12Context::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc, const wchar_t* name)
{
	ComPtr<ID3D12DescriptorHeap> l_descriptorHeap = 0;
	auto l_HResult = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&l_descriptorHeap));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create ", name, " descriptor heap.");
		return 0;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(name, l_descriptorHeap, "DescriptorHeap");
#endif

	return l_descriptorHeap;
}

DX12DescriptorHeapAccessor DX12Context::CreateDescriptorHeapAccessor(ComPtr<ID3D12DescriptorHeap> descHeap, D3D12_DESCRIPTOR_HEAP_DESC desc,
	uint32_t maxDescriptors, uint32_t descriptorSize, const DescriptorHandle& firstHandle, bool shaderVisible, const wchar_t* name)
{
	DX12DescriptorHeapAccessor l_descHeapAccessor = {};
	l_descHeapAccessor.m_Desc.m_HeapDesc = desc;
	l_descHeapAccessor.m_Desc.m_MaxDescriptors = maxDescriptors;
	l_descHeapAccessor.m_Desc.m_DescriptorSize = descriptorSize;
	l_descHeapAccessor.m_Desc.m_ShaderVisible = shaderVisible;
	l_descHeapAccessor.m_Desc.m_Name = name;
	l_descHeapAccessor.m_OffsetFromHeapStart = firstHandle.m_CPUHandle - descHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	l_descHeapAccessor.m_OffsetFromHeapStart /= descriptorSize;
	l_descHeapAccessor.m_FirstHandle = firstHandle;
	l_descHeapAccessor.m_CurrentHandle = firstHandle;

	l_descHeapAccessor.m_Heap = descHeap;

	Log(Verbose, "Descriptor heap accessor ", name, " has been created.");

	return l_descHeapAccessor;
}

// --- DX12DescriptorHeapAccessor ---

DescriptorHandle DX12DescriptorHeapAccessor::GetNewHandle()
{
	if (m_CurrentHandle.m_Index == m_Desc.m_MaxDescriptors)
	{
		Log(Error, "Descriptor heap for ", m_Desc.m_Name, " is full.");
		return {};
	}

	auto l_handle = m_CurrentHandle;
	if (m_Desc.m_ShaderVisible)
		m_CurrentHandle.m_GPUHandle += m_Desc.m_DescriptorSize;

	m_CurrentHandle.m_CPUHandle += m_Desc.m_DescriptorSize;

	m_CurrentHandle.m_Index++;

	return l_handle;
}
