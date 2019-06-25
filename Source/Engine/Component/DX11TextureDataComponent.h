#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11Headers.h"
#include "TextureDataComponent.h"

class DX11TextureDataComponent : public TextureDataComponent
{
public:
	DX11TextureDataComponent() {};
	~DX11TextureDataComponent() {};

	ID3D11Resource* m_texture = 0;
	ID3D11ShaderResourceView* m_SRV = 0;
	ID3D11UnorderedAccessView* m_UAV = 0;
	D3D11_TEXTURE2D_DESC m_DX11TextureDataDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC m_SRVDesc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC m_UAVDesc = {};
};
