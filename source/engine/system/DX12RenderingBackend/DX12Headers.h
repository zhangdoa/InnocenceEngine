#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "DX12/d3dx12.h"

struct DX12ConstantBuffer
{
	ID3D12Resource* m_ConstantBufferPtr = 0;
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_CBVDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_CBVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_CBVGPUHandle;
	void* m_mappedPtr = 0;
};