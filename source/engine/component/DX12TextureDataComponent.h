#pragma once
#include "../common/InnoType.h"
#include "../system/DX12RenderingBackend/DX12Headers.h"

class DX12TextureDataComponent
{
public:
	DX12TextureDataComponent() {};
	~DX12TextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D12Resource* m_texture = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc;
	ID3D12Resource* m_SRV = 0;
};
