#pragma once
#include <wrl/client.h>
#include "directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DXProgrammableCapture.h>
#include <DXGIDebug.h>

#include "../../Common/GraphicsPrimitive.h"

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

	struct DX12RTV
	{
		// Assuming that all render targets have the same format
		D3D12_RENDER_TARGET_VIEW_DESC m_Desc = {};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_Handles;
	};

	struct DX12DSV
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC m_Desc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_Handle;
	};

	struct DX12DescriptorHeapAccessorDesc
	{
		D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc = {};
		const wchar_t* m_Name = nullptr;
		bool m_ShaderVisible = false;
		uint32_t m_MaxDescriptors = 0;
		uint32_t m_DescriptorSize = 0;
	};
	
	class DX12DescriptorHeapAccessor
	{
		friend class DX12RenderingServer;
		
	public:
		ComPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }
		uint32_t GetOffsetFromHeapStart() const { return m_OffsetFromHeapStart; }
		const DX12DescriptorHeapAccessorDesc& GetDesc() const { return m_Desc; }
		const DX12DescriptorHandle& GetFirstHandle() const { return m_FirstHandle; }
		DX12DescriptorHandle GetNewHandle();

	private:
		ComPtr<ID3D12DescriptorHeap> m_Heap = 0;
		uint32_t m_OffsetFromHeapStart = 0;
		DX12DescriptorHeapAccessorDesc m_Desc = {};
		DX12DescriptorHandle m_FirstHandle = {};
		DX12DescriptorHandle m_CurrentHandle = {};
	};

	class DX12PipelineStateObject : public IPipelineStateObject
	{
	public:
		D3D12_INPUT_ELEMENT_DESC m_InputLayoutDesc = {};
		D3D12_DEPTH_STENCIL_DESC m_DepthStencilDesc = {};
		D3D12_BLEND_DESC m_BlendDesc = {};
		D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
		D3D12_RASTERIZER_DESC m_RasterizerDesc = {};
		D3D12_VIEWPORT m_Viewport = {};
		D3D12_RECT m_Scissor = {};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_GraphicsPSODesc = {};
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_ComputePSODesc = {};
		ComPtr<ID3D12PipelineState> m_PSO = 0;
		ComPtr<ID3D12RootSignature> m_RootSignature = 0;
	};

	class DX12CommandList : public ICommandList
	{
	public:
		ComPtr<ID3D12GraphicsCommandList> m_DirectCommandList = 0;
		ComPtr<ID3D12GraphicsCommandList> m_ComputeCommandList = 0;
		ComPtr<ID3D12GraphicsCommandList> m_CopyCommandList = 0;
	};

	class DX12Semaphore : public ISemaphore
	{
	public:
		std::atomic<uint64_t> m_DirectCommandQueueSemaphore = 0;
		std::atomic<uint64_t> m_ComputeCommandQueueSemaphore = 0;
		std::atomic<uint64_t> m_CopyCommandQueueSemaphore = 0;
		HANDLE m_DirectCommandQueueFenceEvent = 0;
		HANDLE m_ComputeCommandQueueFenceEvent = 0;
		HANDLE m_CopyCommandQueueFenceEvent = 0;
	};

	class DX12OutputMergerTarget : public IOutputMergerTarget
	{
	public:
		std::vector<DX12RTV> m_RTVs;
		std::vector<DX12DSV> m_DSVs;
	};

	struct DX12MappedMemory : public IMappedMemory
	{
		ComPtr<ID3D12Resource> m_UploadHeapBuffer = 0;
		DX12CBV m_CBV = {};
	};
	
	struct DX12DeviceMemory : public IDeviceMemory
	{
		ComPtr<ID3D12Resource> m_DefaultHeapBuffer = 0;
		ComPtr<ID3D12Resource> m_ReadBackHeapBuffer = 0;

		DX12SRV m_SRV = {};
		DX12UAV m_UAV = {};
	};
}