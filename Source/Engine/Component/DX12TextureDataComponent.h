#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"
#include "TextureDataComponent.h"

class DX12TextureDataComponent : public TextureDataComponent
{
public:
	ID3D12Resource* m_ResourceHandle = 0;
	D3D12_RESOURCE_DESC m_DX12TextureDataDesc = {};
	unsigned int m_PixelDataSize = 0;
};
