#pragma once
#include "RenderPassComponent.h"
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
	public:
		std::atomic<uint64_t> m_DirectCommandQueueSemaphore = 0;
		std::atomic<uint64_t> m_ComputeCommandQueueSemaphore = 0;
		HANDLE m_DirectCommandQueueFenceEvent = 0;
		HANDLE m_ComputeCommandQueueFenceEvent = 0;
	};

	class DX12RenderPassComponent : public RenderPassComponent
	{
	public:
		D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVDescCPUHandles;

		D3D12_DEPTH_STENCIL_VIEW_DESC m_DSVDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescCPUHandle;

		ComPtr<ID3D12RootSignature> m_RootSignature = 0;
	};
}