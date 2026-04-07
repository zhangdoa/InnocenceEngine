#include "DX12GPUBufferResourceService.h"
#include "DX12MeshResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../MeshResourceService.h"
#include "../GraphicsHardwareService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"
#include "../../Services/EntityRegistry.h"
#include "../../Component/WorldTransformComponent.h"
#include "../../Component/MeshComponent.h"
#include "../../Common/MathHelper.h"

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
				l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), l_initialState, D3D12_RESOURCE_STATE_COPY_DEST));
			}

			UploadToGPU(&l_commandList, l_mappedMemory, l_deviceMemory, gpuBuffer);

			if (l_deviceMemory->m_DefaultHeapBuffer)
			{
				// Transition to the buffer's read state (not COMMON) because buffers
				// with ALLOW_UNORDERED_ACCESS do not support implicit promotion from COMMON.
				auto l_readState = static_cast<D3D12_RESOURCE_STATES>(gpuBuffer->m_ReadState);
				l_dx12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					l_deviceMemory->m_DefaultHeapBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, l_readState));
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

bool DX12GPUBufferResourceService::OnSceneLoadingStart()
{
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
		l_descList->m_Descs.clear();
	}

	m_TLASReady = false;
	m_PrevInstanceCount = 0;

	Log(Verbose, "Raytracing instance descriptions have been cleared.");

	return true;
}

bool DX12GPUBufferResourceService::UpdateRaytracingInstances()
{
	if (m_RaytracingInstanceDescs.empty())
		return true;

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshOwners = l_meshStorage.AllOwners();

	if (l_meshOwners.empty())
		return true;

	auto l_meshService = static_cast<DX12MeshResourceService*>(g_Engine->Get<MeshResourceService>());

	bool l_needRebuild = false;

	if (l_meshOwners.size() != m_PrevInstanceCount)
		l_needRebuild = true;

	if (!l_needRebuild)
	{
		for (EntityID l_entity : l_meshOwners)
		{
			auto* l_world = l_registry->Get<WorldTransformComponent>(l_entity);
			if (l_world && l_world->m_Dirty)
			{
				l_needRebuild = true;
				break;
			}
		}
	}

	if (!l_needRebuild)
		return true;

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	for (size_t frameIndex = 0; frameIndex < l_swapChainImageCount; frameIndex++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[frameIndex]);
		l_descList->m_Descs.clear();

		for (EntityID l_entity : l_meshOwners)
		{
			auto* l_mesh = l_registry->Get<MeshComponent>(l_entity);
			if (!l_mesh || !l_mesh->m_Asset.IsValid())
				continue;

			if (l_mesh->m_ObjectStatus != ObjectStatus::Activated)
				continue;

			uint64_t l_blasAddress = l_meshService->GetBLASAddress(l_mesh->m_Asset);
			if (l_blasAddress == 0)
				continue;

			auto* l_world = l_registry->Get<WorldTransformComponent>(l_entity);
			Mat4 l_transform = l_world ? l_world->m_WorldMatrix : Math::generateIdentityMatrix<float>();

			D3D12_RAYTRACING_INSTANCE_DESC l_instanceDesc = {};

			l_instanceDesc.Transform[0][0] = l_transform.m00;
			l_instanceDesc.Transform[0][1] = l_transform.m01;
			l_instanceDesc.Transform[0][2] = l_transform.m02;
			l_instanceDesc.Transform[0][3] = l_transform.m03;

			l_instanceDesc.Transform[1][0] = l_transform.m10;
			l_instanceDesc.Transform[1][1] = l_transform.m11;
			l_instanceDesc.Transform[1][2] = l_transform.m12;
			l_instanceDesc.Transform[1][3] = l_transform.m13;

			l_instanceDesc.Transform[2][0] = l_transform.m20;
			l_instanceDesc.Transform[2][1] = l_transform.m21;
			l_instanceDesc.Transform[2][2] = l_transform.m22;
			l_instanceDesc.Transform[2][3] = l_transform.m23;

			l_instanceDesc.InstanceID = static_cast<UINT>(l_descList->m_Descs.size());
			l_instanceDesc.InstanceMask = 0xFF;
			l_instanceDesc.InstanceContributionToHitGroupIndex = 0;
			l_instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			l_instanceDesc.AccelerationStructure = l_blasAddress;

			l_descList->m_Descs.emplace_back(l_instanceDesc);
		}
	}

	m_PrevInstanceCount = l_meshOwners.size();
	m_TLASReady = false;

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

	auto l_DX12CommandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

	l_DX12CommandList->CopyResource(deviceMemory->m_DefaultHeapBuffer.Get(), mappedMemory->m_UploadHeapBuffer.Get());

	return true;
}

bool DX12GPUBufferResourceService::Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer)
{
	auto l_commandList = reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);

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

bool DX12GPUBufferResourceService::CreateSRV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;
	l_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadWrite);

	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		l_DX12DeviceMemory->m_SRV.SRVDesc = l_desc;
		l_DX12DeviceMemory->m_SRV.Handle = l_descHeapAccessor.GetNewHandle();
		m_ctx->m_device->CreateShaderResourceView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), &l_DX12DeviceMemory->m_SRV.SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_DX12DeviceMemory->m_SRV.Handle.m_CPUHandle });
		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_DX12DeviceMemory->m_SRV.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GPUBufferResourceService::CreateUAV(GPUBufferComponent* gpuBuffer)
{
	bool l_isRaytracingAS = gpuBuffer->m_Usage == GPUBufferUsage::TLAS || gpuBuffer->m_Usage == GPUBufferUsage::ScratchBuffer;
	D3D12_UNORDERED_ACCESS_VIEW_DESC l_desc = {};
	l_desc.Format = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	l_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	l_desc.Buffer.NumElements = l_isRaytracingAS ? 1 : (uint32_t)gpuBuffer->m_ElementCount;
	l_desc.Buffer.StructureByteStride = gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ? 0 : (uint32_t)gpuBuffer->m_ElementSize;

	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite);
	auto& l_descHeapAccessor_ShaderNonVisible = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadWrite, Accessibility::ReadWrite, TextureUsage::Invalid, false);

	for (auto i : gpuBuffer->m_DeviceMemories)
	{
		auto l_DX12DeviceMemory = reinterpret_cast<DX12DeviceMemory*>(i);
		DX12UAV l_result = {};
		l_result.UAVDesc = l_desc;

		auto l_descHandle = l_descHeapAccessor.GetNewHandle();
		auto l_descHandle_ShaderNonVisible = l_descHeapAccessor_ShaderNonVisible.GetNewHandle();

		l_result.Handle.m_CPUHandle = l_descHandle_ShaderNonVisible.m_CPUHandle;
		l_result.Handle.m_GPUHandle = l_descHandle.m_GPUHandle;
		l_result.Handle.m_Index = l_descHandle.m_Index;

		m_ctx->m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle_ShaderNonVisible.m_CPUHandle });
		m_ctx->m_device->CreateUnorderedAccessView(l_DX12DeviceMemory->m_DefaultHeapBuffer.Get(), gpuBuffer->m_Usage == GPUBufferUsage::AtomicCounter ?
			l_DX12DeviceMemory->m_DefaultHeapBuffer.Get() : 0, &l_result.UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_descHandle.m_CPUHandle });

		l_DX12DeviceMemory->m_UAV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GPUBufferResourceService::CreateCBV(GPUBufferComponent* gpuBuffer)
{
	auto& l_descHeapAccessor = m_ctx->GetDescriptorHeapAccessor(gpuBuffer->m_GPUResourceType, Accessibility::ReadOnly, Accessibility::ReadOnly);

	for (auto i : gpuBuffer->m_MappedMemories)
	{
		auto l_DX12MappedMemory = reinterpret_cast<DX12MappedMemory*>(i);
		DX12CBV l_result;

		l_result.CBVDesc.BufferLocation = l_DX12MappedMemory->m_UploadHeapBuffer->GetGPUVirtualAddress();
		l_result.CBVDesc.SizeInBytes = (uint32_t)gpuBuffer->m_ElementSize;
		l_result.Handle = l_descHeapAccessor.GetNewHandle();

		m_ctx->m_device->CreateConstantBufferView(&l_result.CBVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_result.Handle.m_CPUHandle });

		l_DX12MappedMemory->m_CBV = l_result;

		Log(Verbose, "New handle on ", l_descHeapAccessor.GetDesc().m_Name, " with index ", l_result.Handle.m_Index, " for ", gpuBuffer->m_InstanceName, " has been created.");
	}

	return true;
}

bool DX12GPUBufferResourceService::CreateRaytracingResources()
{
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	m_TLASBufferComponent = Add("TLASBuffer/");
	m_TLASBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_TLASBufferComponent->m_Usage = GPUBufferUsage::TLAS;
	m_TLASBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_TLASBufferComponent);

	m_ScratchBufferComponent = Add("ScratchBuffer/");
	m_ScratchBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_ScratchBufferComponent->m_Usage = GPUBufferUsage::ScratchBuffer;
	m_ScratchBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_ScratchBufferComponent);

	m_RaytracingInstanceBufferComponent = Add("RaytracingInstanceBuffer/");
	m_RaytracingInstanceBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RaytracingInstanceBufferComponent->m_ElementCount = 256;
	m_RaytracingInstanceBufferComponent->m_ElementSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

	InitializeImpl(m_RaytracingInstanceBufferComponent);

	m_RaytracingInstanceDescs.resize(l_swapChainImageCount);
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = new DX12RaytracingInstanceDescList();
		l_descList->m_Descs.reserve(m_RaytracingInstanceBufferComponent->m_ElementCount);
		m_RaytracingInstanceDescs[i] = l_descList;
	}

	return true;
}

bool DX12GPUBufferResourceService::ReleaseRaytracingResources()
{
	Delete(m_RaytracingInstanceBufferComponent);
	Delete(m_ScratchBufferComponent);
	Delete(m_TLASBufferComponent);

	return true;
}
