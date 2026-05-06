#include "DX12Helper_Pipeline.h"

using namespace Inno;

D3D12_DESCRIPTOR_HEAP_DESC DX12Helper::GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC l_Desc = {};
	l_Desc.Type = type;
	l_Desc.NumDescriptors = numDescriptors;
	l_Desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	l_Desc.NodeMask = 0;
	return l_Desc;
}
