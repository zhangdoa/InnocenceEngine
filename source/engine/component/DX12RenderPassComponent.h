#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"
#include "DX12TextureDataComponent.h"

class DX12RenderPassComponent
{
public:
	DX12RenderPassComponent() {};
	~DX12RenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<DX12TextureDataComponent*> m_DXTDCs;

	ID3D12DescriptorHeap* m_renderTargetViewHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetViewHandle;
	std::vector<D3D12_RENDER_TARGET_VIEW_DESC*> m_renderTargetViewDescs;
	std::vector<ID3D12Resource*> m_renderTargetViews;

	D3D12_DEPTH_STENCIL_DESC m_depthStencilBufferDesc = D3D12_DEPTH_STENCIL_DESC();
	ID3D12Resource* m_depthStencilBuffer = 0;

	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilViewDesc = D3D12_DEPTH_STENCIL_VIEW_DESC();
	ID3D12Resource* m_depthStencilView = 0;

	D3D12_VIEWPORT m_viewport = D3D12_VIEWPORT();
};