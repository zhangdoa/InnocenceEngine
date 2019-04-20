#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"

class DX12TextureDataComponent : public TextureDataComponent
{
public:
	DX12TextureDataComponent() {};
	~DX12TextureDataComponent() {};

	ID3D12Resource* m_texture = 0;
	ID3D12DescriptorHeap* m_SRVHeap = 0;
	ID3D12Resource* m_SRV = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc = D3D12_SHADER_RESOURCE_VIEW_DESC();
};
