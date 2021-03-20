#pragma once
#include "RenderPassDataComponent.h"
#include "../RenderingServer/DX12/DX12Headers.h"

namespace Inno
{
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
	};

	class DX12RenderPassDataComponent : public RenderPassDataComponent
	{
	public:
		ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC m_RTVDescriptorHeapDesc = {};
		D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVDescriptorCPUHandles;

		ComPtr<ID3D12DescriptorHeap> m_DSVDescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC m_DSVDescriptorHeapDesc = {};
		D3D12_DEPTH_STENCIL_VIEW_DESC m_DSVDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescriptorCPUHandle;

		ComPtr<ID3D12RootSignature> m_RootSignature = 0;
	};
}