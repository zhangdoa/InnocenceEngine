#include "DX12GPUBufferResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;
using namespace DX12Helper;

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
