#pragma once
#include "DX12Headers.h"
#include "../../Common/GraphicsPrimitive.h"

namespace Inno
{
	struct DX12Context;
	struct CommandListComponent;
}

namespace Inno::DX12Helper_BindlessMesh
{
	void InitSRVHeapSections(
		DX12Context& ctx,
		const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc,
		uint32_t maxSlots,
		uint32_t incrementSize,
		const DescriptorHandle& sectionStart);

	bool TryAutoBindCompute(
		CommandListComponent* commandList,
		GPUBufferUsage usage,
		DX12Context& ctx,
		uint32_t rootParameterIndex);

	bool TryFillDescriptorRange(
		GPUBufferUsage usage,
		DX12Context& ctx,
		D3D12_DESCRIPTOR_RANGE1& outRange);
}
