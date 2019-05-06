#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"
#include "DX12TextureDataComponent.h"

enum class FenceStatus { IDLE, WORKING };
class DX12RenderPassComponent
{
public:
	DX12RenderPassComponent() {};
	~DX12RenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	RenderPassDesc m_renderPassDesc;

	std::vector<ID3D12CommandAllocator*> m_commandAllocators;
	D3D12_COMMAND_QUEUE_DESC m_commandQueueDesc = {};
	ID3D12CommandQueue* m_commandQueue;
	std::vector<ID3D12GraphicsCommandList*> m_commandLists;

	ID3D12DescriptorHeap* m_RTVHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_RTVHeapDesc = {};
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVCPUDescHandles;

	D3D12_RENDER_TARGET_VIEW_DESC m_RTVDesc = {};

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC m_rootSignatureDesc = {};
	ID3D12RootSignature* m_rootSignature;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};
	ID3D12PipelineState* m_PSO;

	std::vector<DX12TextureDataComponent*> m_DXTDCs;

	D3D12_DEPTH_STENCIL_DESC m_depthStencilBufferDesc = {};
	ID3D12Resource* m_depthStencilBuffer = 0;

	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc = {};
	ID3D12Resource* m_depthStencilView = 0;

	D3D12_BLEND_DESC m_blendDesc = {};

	D3D12_RASTERIZER_DESC m_rasterizerDesc = {};

	D3D12_VIEWPORT m_viewport = {};

	D3D12_RECT m_scissor = {};

	unsigned int m_frameIndex = 0;

	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	std::vector<unsigned long long> m_fenceStatus;
};