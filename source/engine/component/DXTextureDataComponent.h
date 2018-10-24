#pragma once
#include "TextureDataComponent.h"
#include "../system/HighLevelSystem/DXHeaders.h"

class DXTextureDataComponent : public BaseComponent
{
public:
	DXTextureDataComponent() {};
	~DXTextureDataComponent() {};

	ID3D11Texture2D* m_texture = 0;
	ID3D11ShaderResourceView* m_textureView = 0;
};

