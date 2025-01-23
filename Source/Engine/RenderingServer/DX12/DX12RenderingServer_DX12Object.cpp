#include "DX12RenderingServer.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"

#include "DX12Helper_Common.h"

#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

DX12SRV DX12RenderingServer::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle)
{
	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DX12SRV l_result = {};
	l_result.SRVDesc = desc;
	l_result.Handle = m_CSUDescHeap.GetNewHandle();

	m_device->CreateShaderResourceView(resourceHandle.Get(), &l_result.SRVDesc, l_result.Handle.CPUHandle);

	return l_result;
}

DX12UAV DX12RenderingServer::CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC desc, ComPtr<ID3D12Resource> resourceHandle, bool isAtomicCounter)
{
	auto l_CSUDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DX12UAV l_result = {};
	l_result.UAVDesc = desc;

	auto l_descHandle = m_CSUDescHeap.GetNewHandle();
	auto l_descHandle_ShaderNonVisible = m_ShaderNonVisibleCSUDescHeap.GetNewHandle();

	l_result.Handle.CPUHandle = l_descHandle_ShaderNonVisible.CPUHandle;
	l_result.Handle.GPUHandle = l_descHandle.GPUHandle;
	l_result.Handle.m_Index = l_descHandle.m_Index; // @TODO: Not sure which one is correct

	m_device->CreateUnorderedAccessView(resourceHandle.Get(), isAtomicCounter ? resourceHandle.Get() : 0, &l_result.UAVDesc, l_descHandle_ShaderNonVisible.CPUHandle);
	m_device->CreateUnorderedAccessView(resourceHandle.Get(), isAtomicCounter ? resourceHandle.Get() : 0, &l_result.UAVDesc, l_descHandle.CPUHandle);

	return l_result;
}

ComPtr<ID3D12Resource> DX12RenderingServer::CreateUploadHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, const char* name)
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

ComPtr<ID3D12Resource> DX12RenderingServer::CreateDefaultHeapBuffer(D3D12_RESOURCE_DESC* resourceDesc, D3D12_CLEAR_VALUE* clearValue, const char* name)
{
	ComPtr<ID3D12Resource> l_defaultHeapBuffer;

	auto l_HResult = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		clearValue,
		IID_PPV_ARGS(&l_defaultHeapBuffer));

	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create default heap buffer ", name);
		return false;
	}

	return l_defaultHeapBuffer;
}

ComPtr<ID3D12Resource> DX12RenderingServer::CreateReadBackHeapBuffer(UINT64 size, const char* name)
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
		Log(Error, "Can't create read-back heap buffer ", name);
		return nullptr;
	}

	return l_readBackHeapBuffer;
}

ComPtr<ID3D12CommandQueue> DX12RenderingServer::CreateCommandQueue(D3D12_COMMAND_QUEUE_DESC* commandQueueDesc, const wchar_t* name)
{
	ComPtr<ID3D12CommandQueue> l_commandQueue;

	auto l_HResult = m_device->CreateCommandQueue(commandQueueDesc, IID_PPV_ARGS(&l_commandQueue));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandQueue: ", name);
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandQueue->SetName(name);
#endif // INNO_DEBUG

	Log(Verbose, "CommandQueue: ", name, " has been created.");

	return l_commandQueue;
}

ComPtr<ID3D12CommandAllocator> DX12RenderingServer::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, const wchar_t* name)
{
	ComPtr<ID3D12CommandAllocator> l_commandAllocator;

	auto l_HResult = m_device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&l_commandAllocator));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandAllocator: ", name);
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandAllocator->SetName(name);
#endif // INNO_DEBUG

	Log(Success, "CommandAllocator: ", name, " has been created.");

	return l_commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> DX12RenderingServer::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator, const wchar_t* name)
{
	ComPtr<ID3D12GraphicsCommandList> l_commandList;

	auto l_HResult = m_device->CreateCommandList(0, commandListType, commandAllocator.Get(), NULL, IID_PPV_ARGS(&l_commandList));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create CommandList ", name);
		return nullptr;
	}

#ifdef INNO_DEBUG
	l_commandList->SetName(name);
#endif // INNO_DEBUG

	Log(Success, "CommandList: ", name, " has been created.");

	return l_commandList;
}

ComPtr<ID3D12GraphicsCommandList> DX12RenderingServer::CreateTemporaryCommandList(D3D12_COMMAND_LIST_TYPE commandListType, ComPtr<ID3D12CommandAllocator> commandAllocator)
{
	static uint64_t index = 0;

	return CreateCommandList(commandListType, commandAllocator, (L"TemporaryCommandList_" + std::to_wstring(index++)).c_str());
}

bool DX12RenderingServer::ExecuteCommandListAndWait(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandQueue> commandQueue)
{
	auto l_HResult = commandList->Close();
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't close command list.");
	}

	ComPtr<ID3D12Fence1> l_commandListFence;
	l_HResult = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&l_commandListFence));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create fence for command list.");
	}

	auto l_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (l_fenceEvent == nullptr)
	{
		Log(Error, "Can't create fence event for command list.");
	}

	const auto onFinishedValue = 1;
	l_commandListFence->SetEventOnCompletion(onFinishedValue, l_fenceEvent);

	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	commandQueue->Signal(l_commandListFence.Get(), onFinishedValue);

	WaitForSingleObject(l_fenceEvent, INFINITE);
	CloseHandle(l_fenceEvent);

	return true;
}

ComPtr<ID3D12DescriptorHeap> DX12RenderingServer::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc, const wchar_t* name)
{
	ComPtr<ID3D12DescriptorHeap> l_descriptorHeap = 0;
	auto l_HResult = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&l_descriptorHeap));
	if (FAILED(l_HResult))
	{
		Log(Error, "Can't create DescriptorHeap.");
		return 0;
	}

#ifdef INNO_DEBUG
	SetObjectName(name, l_descriptorHeap, "DescriptorHeap");
#endif // INNO_DEBUG

	return l_descriptorHeap;
}