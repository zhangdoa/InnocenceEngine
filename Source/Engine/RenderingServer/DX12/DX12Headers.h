#pragma once
#include <wrl/client.h>
#include "directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DXProgrammableCapture.h>
#include <DXGIDebug.h>

using namespace Microsoft::WRL;

namespace Inno
{
#define USE_DXIL

	struct DX12DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
		uint32_t m_Index = 0;
	};

	struct DX12CBV
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
		DX12DescriptorHandle Handle;
	};

	struct DX12SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		DX12DescriptorHandle Handle;
	};

	struct DX12UAV
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};

		// CPU handle is a shader-non-visible descriptor
		DX12DescriptorHandle Handle;
	};

	struct DX12Sampler
	{
		D3D12_SAMPLER_DESC SamplerDesc = {};
		DX12DescriptorHandle Handle;
	};

	class DX12DescriptorHeap
	{
		friend class DX12RenderingServer;
		
	public:
		ComPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }

		DX12DescriptorHandle GetNewHandle()
		{
			auto l_handle = m_Handle;
			if (m_ShaderVisible)
				m_Handle.GPUHandle.ptr += m_DescriptorSize;

			m_Handle.CPUHandle.ptr += m_DescriptorSize;
			m_Handle.m_Index++;

			return l_handle;
		}

	private:
		ComPtr<ID3D12DescriptorHeap> m_Heap = 0;
		D3D12_DESCRIPTOR_HEAP_DESC m_Desc = {};
		bool m_ShaderVisible = false;
		uint32_t m_DescriptorSize = 0;
		DX12DescriptorHandle m_Handle = {};
	};
}