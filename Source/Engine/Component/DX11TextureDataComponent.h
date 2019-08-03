#pragma once
#include "../Common/InnoType.h"
#include "../RenderingBackend/DX11RenderingBackend/DX11Headers.h"
#include "TextureDataComponent.h"

typedef struct D3D11_TEXTURE_DESC
{
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
	UINT PixelDataSize;
} D3D11_TEXTURE_DESC;

class DX11TextureDataComponent : public TextureDataComponent
{
public:
	ID3D11Resource* m_ResourceHandle = 0;
	ID3D11ShaderResourceView* m_SRV = 0;
	ID3D11UnorderedAccessView* m_UAV = 0;
	D3D11_TEXTURE_DESC m_DX11TextureDataDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC m_SRVDesc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC m_UAVDesc = {};
};
