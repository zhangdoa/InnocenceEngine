#pragma once
#include "TextureDataComponent.h"
#include "../system/HighLevelSystem/DXHeaders.h"

class DXTextureDataComponent : public TextureDataComponent
{
public:
	DXTextureDataComponent() {};
	~DXTextureDataComponent() {};

	ID3D11Texture2D* m_texture;
	ID3D11ShaderResourceView* m_textureView;
};

