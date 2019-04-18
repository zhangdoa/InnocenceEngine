#pragma once
#include "../common/InnoType.h"
#include "../system/DX11RenderingBackend/DX11Headers.h"
#include "TextureDataComponent.h"

class DX11TextureDataComponent : public TextureDataComponent
{
public:
	DX11TextureDataComponent() {};
	~DX11TextureDataComponent() {};

	ID3D11Texture2D* m_texture = 0;
	ID3D11ShaderResourceView* m_SRV = 0;
	D3D11_TEXTURE2D_DESC m_textureDesc = D3D11_TEXTURE2D_DESC();
};
