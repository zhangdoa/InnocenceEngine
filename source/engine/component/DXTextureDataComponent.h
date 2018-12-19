#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"

class DXTextureDataComponent
{
public:
	DXTextureDataComponent() {};
	~DXTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	D3D11_TEXTURE2D_DESC m_textureDesc = D3D11_TEXTURE2D_DESC();
	ID3D11Texture2D* m_texture = 0;
	ID3D11ShaderResourceView* m_SRV = 0;
};

