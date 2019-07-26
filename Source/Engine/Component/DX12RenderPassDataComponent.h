#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"
#include "RenderPassDataComponent.h"
#include "DX12TextureDataComponent.h"

enum class FenceStatus { IDLE, WORKING };
class DX12RenderPassDataComponent : public RenderPassDataComponent
{
public:
	ID3D12DescriptorHeap* m_RTVDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_RTVDescriptorHeapDesc = {};
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVDescriptorCPUHandles;
	D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};
	std::vector<DX12TextureDataComponent*> m_renderTargets;
	std::vector<DX12SRV> m_SRVs;

	ID3D12DescriptorHeap* m_DSVDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_DSVDescriptorHeapDesc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescriptorCPUHandle;
	D3D12_DEPTH_STENCIL_VIEW_DESC m_DSVDesc = {};
	D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc = {};
	DX12TextureDataComponent* m_depthStencilTarget;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC m_rootSignatureDesc = {};
	ID3D12RootSignature* m_rootSignature;

	D3D12_BLEND_DESC m_blendDesc = {};

	D3D12_RASTERIZER_DESC m_rasterizerDesc = {};

	D3D12_VIEWPORT m_viewport = {};

	D3D12_RECT m_scissor = {};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};
	ID3D12PipelineState* m_PSO;

	std::vector<ID3D12CommandAllocator*> m_commandAllocators;
	D3D12_COMMAND_QUEUE_DESC m_commandQueueDesc = {};
	ID3D12CommandQueue* m_commandQueue;
	std::vector<ID3D12GraphicsCommandList*> m_commandLists;

	unsigned int m_currentFrameIndex = 0;

	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	std::vector<unsigned long long> m_fenceStatus;
};