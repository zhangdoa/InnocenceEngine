#include "DX12Helper_BindlessMesh.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"

using namespace Inno;

namespace
{
	void InitOneSection(
		DX12Context& ctx,
		DX12DescriptorHeapAccessor& outAccessor,
		const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc,
		uint32_t maxSlots,
		uint32_t incrementSize,
		const DescriptorHandle& firstHandle,
		const wchar_t* name)
	{
		outAccessor = ctx.CreateDescriptorHeapAccessor(ctx.m_CSUDescHeap, heapDesc, maxSlots, incrementSize, firstHandle, true, name);

		auto l_currentHandle = firstHandle;
		D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
		l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		l_SRVDesc.Format = DXGI_FORMAT_R32_SINT;
		l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		for (uint32_t i = 0; i < maxSlots; i++)
		{
			ctx.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
			l_currentHandle.m_CPUHandle += incrementSize;
			l_currentHandle.m_GPUHandle += incrementSize;
		}
	}
}

void DX12Helper_BindlessMesh::InitSRVHeapSections(
	DX12Context& ctx,
	const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc,
	uint32_t maxSlots,
	uint32_t incrementSize,
	const DescriptorHandle& sectionStart)
{
	const uint64_t sectionSize = static_cast<uint64_t>(maxSlots) * incrementSize;
	InitOneSection(ctx, ctx.m_BindlessMeshVertex_SRV_DescHeapAccessor, heapDesc, maxSlots, incrementSize, sectionStart, L"BindlessMeshVertex_SRV_DescHeapAccessor");

	DescriptorHandle indexStart = {};
	indexStart.m_CPUHandle = sectionStart.m_CPUHandle + sectionSize;
	indexStart.m_GPUHandle = sectionStart.m_GPUHandle + sectionSize;
	InitOneSection(ctx, ctx.m_BindlessMeshIndex_SRV_DescHeapAccessor, heapDesc, maxSlots, incrementSize, indexStart, L"BindlessMeshIndex_SRV_DescHeapAccessor");
}

bool DX12Helper_BindlessMesh::TryAutoBindCompute(
	CommandListComponent* commandList,
	GPUBufferUsage usage,
	DX12Context& ctx,
	uint32_t rootParameterIndex)
{
	if (usage != GPUBufferUsage::BindlessMeshVertex && usage != GPUBufferUsage::BindlessMeshIndex)
		return false;

	auto& accessor = (usage == GPUBufferUsage::BindlessMeshVertex)
		? ctx.m_BindlessMeshVertex_SRV_DescHeapAccessor
		: ctx.m_BindlessMeshIndex_SRV_DescHeapAccessor;

	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	l_commandList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ accessor.GetFirstHandle().m_GPUHandle });
	return true;
}

bool DX12Helper_BindlessMesh::TryFillDescriptorRange(
	GPUBufferUsage usage,
	DX12Context& ctx,
	D3D12_DESCRIPTOR_RANGE1& outRange)
{
	if (usage == GPUBufferUsage::BindlessMeshVertex)
	{
		outRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		outRange.NumDescriptors = ctx.m_BindlessMeshVertex_SRV_DescHeapAccessor.GetDesc().m_MaxDescriptors;
		outRange.BaseShaderRegister = 0;
		outRange.RegisterSpace = 1;
		return true;
	}
	if (usage == GPUBufferUsage::BindlessMeshIndex)
	{
		outRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		outRange.NumDescriptors = ctx.m_BindlessMeshIndex_SRV_DescHeapAccessor.GetDesc().m_MaxDescriptors;
		outRange.BaseShaderRegister = 0;
		outRange.RegisterSpace = 2;
		return true;
	}
	return false;
}
