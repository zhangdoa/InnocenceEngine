#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "DX12/d3dx12.h"

using namespace Microsoft::WRL;

namespace Inno
{
	#define USE_DXIL
	struct DX12CBV
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	};

	struct DX12SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	};

	struct DX12UAV
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		D3D12_GPU_DESCRIPTOR_HANDLE ShaderVisibleGPUHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE ShaderNonVisibleCPUHandle;
	};

	struct DX12Sampler
	{
		D3D12_SAMPLER_DESC SamplerDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	};
}