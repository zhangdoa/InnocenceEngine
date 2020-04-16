#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"

class DX12TextureDataComponent : public TextureDataComponent
{
public:
	ID3D12Resource* m_ResourceHandle = 0;
	D3D12_RESOURCE_DESC m_DX12TextureDesc = {};
	uint32_t m_PixelDataSize = 0;
	D3D12_RESOURCE_STATES m_WriteState;
	D3D12_RESOURCE_STATES m_ReadState;
	D3D12_RESOURCE_STATES m_CurrentState;
};
