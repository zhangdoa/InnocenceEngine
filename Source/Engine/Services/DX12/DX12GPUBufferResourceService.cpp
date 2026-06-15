#include "DX12GPUBufferResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

#ifdef max
#undef max
#endif

using namespace Inno;
using namespace DX12Helper;

bool DX12GPUBufferResourceService::Delete(GPUBufferComponent* gpuBuffer)
{
	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		if (l_DX12DeviceMemory->m_DefaultHeapBuffer)
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Reset();
	}

	gpuBuffer->m_DeviceMemories.clear();

	for (auto i : gpuBuffer->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		if (l_DX12MappedMemory->m_UploadHeapBuffer)
			l_DX12MappedMemory->m_UploadHeapBuffer.Reset();
	}

	gpuBuffer->m_MappedMemories.clear();

	GPUBufferResourceService::Delete(gpuBuffer);

	return true;
}

bool DX12GPUBufferResourceService::InitializeImpl(GPUBufferComponent* gpuBuffer)
{
	auto l_initialState = D3D12_RESOURCE_STATE_COMMON;
	auto l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	if (gpuBuffer->m_Usage == GPUBufferUsage::IndirectDraw)
	{
		gpuBuffer->m_ElementSize = 64;
		gpuBuffer->m_CPUAccessibility = Accessibility::Immutable;
		gpuBuffer->m_GPUAccessibility = Accessibility::ReadWrite;
		gpuBuffer->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	else if (l_isRaytracingAS)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
		tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		tlasInputs.NumDescs = gpuBuffer->m_ElementCount;
		tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
			| D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		m_ctx->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfo);

		UINT64 TLAS_SIZE_IN_BYTES = prebuildInfo.ResultDataMaxSizeInBytes;
		UINT64 SCRATCH_SIZE_IN_BYTES = prebuildInfo.ScratchDataSizeInBytes;

		gpuBuffer->m_ElementSize = gpuBuffer->m_Usage == GPUBufferUsage::TLAS ? TLAS_SIZE_IN_BYTES : SCRATCH_SIZE_IN_BYTES;
		if (gpuBuffer->m_Usage == GPUBufferUsage::TLAS)
			l_initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		gpuBuffer->m_ReadState = static_cast<uint32_t>(l_initialState);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	else
	{
		bool l_needDefaultHeap = gpuBuffer->m_GPUAccessibility.CanRead() && !gpuBuffer->m_CPUAccessibility.CanRead();
		if (l_needDefaultHeap)
			gpuBuffer->m_ReadState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		else
			gpuBuffer->m_ReadState = static_cast<uint32_t>(l_initialState);
		gpuBuffer->m_WriteState = static_cast<uint32_t>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	auto l_actualElementCount = l_isRaytracingAS ? 1 : gpuBuffer->m_ElementCount;
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
	gpuBuffer->m_GPUResourceType = GPUResourceType::Buffer;
	gpuBuffer->m_TotalSize = l_actualElementCount * gpuBuffer->m_ElementSize;
	gpuBuffer->m_MappedMemories.resize(l_swapChainImageCount);
	gpuBuffer->m_DeviceMemories.resize(l_swapChainImageCount);

	gpuBuffer->m_CurrentState.resize(l_swapChainImageCount, static_cast<uint32_t>(l_initialState));

	auto l_uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize);
	bool l_needDefaultHeap = gpuBuffer->m_GPUAccessibility.CanRead() && !gpuBuffer->m_CPUAccessibility.CanRead();

	for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
	{
		auto l_mappedMemory = new DX12MappedMemory();
		l_mappedMemory->m_UploadHeapBuffer = m_ctx->CreateUploadHeapBuffer(&l_uploadBufferDesc);

		if (!l_mappedMemory->m_UploadHeapBuffer)
		{
			Log(Error, "Failed to create upload heap buffer for frame ", i);
			delete l_mappedMemory;
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(gpuBuffer, l_mappedMemory->m_UploadHeapBuffer, ("UploadHeap_" + std::to_string(i)).c_str());
#endif
		CD3DX12_RANGE m_readRange(0, 0);
		l_mappedMemory->m_UploadHeapBuffer->Map(0, &m_readRange, &l_mappedMemory->m_Address);
		gpuBuffer->m_MappedMemories[i] = l_mappedMemory;

		if (!l_needDefaultHeap)
			continue;

		auto l_defaultHeapResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gpuBuffer->m_TotalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto l_deviceMemory = new DX12DeviceMemory();
		l_deviceMemory->m_DefaultHeapBuffer = m_ctx->CreateDefaultHeapBuffer(&l_defaultHeapResourceDesc, l_initialState);

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(gpuBuffer, l_deviceMemory->m_DefaultHeapBuffer, ("DefaultHeap_" + std::to_string(i)).c_str());
#endif
		gpuBuffer->m_DeviceMemories[i] = l_deviceMemory;
	}

	if (gpuBuffer->m_InitialData)
	{

		auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();

		CommandListComponent l_commandList = {};
		l_commandList.m_Type = GPUEngineType::Graphics;
		auto l_dx12CommandList = m_ctx->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_ctx->GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, l_currentFrame), L"GPUBufferInitCommandList");
		l_commandList.m_CommandList = reinterpret_cast<uint64_t>(l_dx12CommandList.Get());

		for (uint32_t i = 0; i < l_swapChainImageCount; ++i)
		{
			auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(gpuBuffer->m_MappedMemories[i]);
			auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[i]);
			std::memcpy(l_mappedMemory->m_Address, gpuBuffer->m_InitialData, gpuBuffer->m_TotalSize);
			l_mappedMemory->m_NeedUploadToGPU = false;

			if (l_deviceMemory->m_DefaultHeapBuffer)
			{
				auto l_barrierToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), l_initialState, D3D12_RESOURCE_STATE_COPY_DEST);
				l_dx12CommandList->ResourceBarrier(1, &l_barrierToCopy);
			}
			UploadToGPU(&l_commandList, l_mappedMemory, l_deviceMemory, gpuBuffer);

			if (l_deviceMemory->m_DefaultHeapBuffer)
			{
				// Transition to the buffer's read state (not COMMON) because buffers
				auto l_readState = static_cast<D3D12_RESOURCE_STATES>(gpuBuffer->m_ReadState);
				auto l_barrierToRead = CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_readState);
				l_dx12CommandList->ResourceBarrier(1, &l_barrierToRead);
				gpuBuffer->m_CurrentState[i] = gpuBuffer->m_ReadState;
			}
		}

		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		auto l_globalSemaphore = g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore();
		l_fmService->Close(&l_commandList, GPUEngineType::Graphics);
		l_hwService->Execute(&l_commandList, GPUEngineType::Graphics);
		l_hwService->SignalOnGPU(l_globalSemaphore, GPUEngineType::Graphics);
		auto l_semaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semaphoreValue, GPUEngineType::Graphics);
	}

	if (gpuBuffer->m_Usage != GPUBufferUsage::ScratchBuffer)
	{
		CreateSRV(gpuBuffer);
		if (l_needDefaultHeap)
			CreateUAV(gpuBuffer);
	}

	Log(Verbose, gpuBuffer->m_InstanceName, " (", gpuBuffer->m_Usage, ") is initialized.");
	gpuBuffer->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool DX12GPUBufferResourceService::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	auto l_mappedMemory = reinterpret_cast<DX12MappedMemory*>(gpuBuffer->m_MappedMemories[l_currentFrame]);
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[l_currentFrame]);

	return UploadToGPU(commandList, l_mappedMemory, l_deviceMemory, gpuBuffer);
}

bool DX12GPUBufferResourceService::UploadToGPU(CommandListComponent* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* gpuBuffer)
{
	if (!deviceMemory->m_DefaultHeapBuffer)
		return true;

	auto l_DX12CommandList = DX12Helper::AsDX12CommandList(commandList);

	l_DX12CommandList->CopyResource(deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get());

	return true;
}

bool DX12GPUBufferResourceService::Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);

	ID3D12DescriptorHeap* l_heaps[] = { m_ctx->m_CSUDescHeap.Get() };
	l_commandList->SetDescriptorHeaps(1, l_heaps);

	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	const uint32_t zero = 0;
	auto l_deviceMemory = reinterpret_cast<DX12DeviceMemory*>(gpuBuffer->m_DeviceMemories[l_currentFrame]);
	l_commandList->ClearUnorderedAccessViewUint(
		D3D12_GPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_GPUHandle },
		D3D12_CPU_DESCRIPTOR_HANDLE{ l_deviceMemory->m_UAV.Handle.m_CPUHandle },
		l_deviceMemory->m_DefaultHeapBuffer.Get(),
		&zero,
		0,
		NULL);

	return true;
}
