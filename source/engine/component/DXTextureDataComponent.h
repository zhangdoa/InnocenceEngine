#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"

class DXTextureDataComponent
{
public:
	DXTextureDataComponent() {};
	~DXTextureDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ID3D11Texture2D* m_texture = 0;
	ID3D11ShaderResourceView* m_textureView = 0;
};

