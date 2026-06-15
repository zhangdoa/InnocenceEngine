#include "DX12Context.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"

using namespace Inno;
using namespace DX12Helper;

// Descriptor-heap creation + DX12DescriptorHeapAccessor::GetNewHandle.
// Extracted from DX12Context.cpp to keep that file under the 300-line
// commit-guard cap. Behaviorally identical; no semantic change.

ComPtr<ID3D12DescriptorHeap> DX12Context::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc, const wchar_t* name)
{
	ComPtr<ID3D12DescriptorHeap> l_descriptorHeap = 0;
	auto l_HResult = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&l_descriptorHeap));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_device.Get(), "descriptor heap", name, l_HResult);
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
