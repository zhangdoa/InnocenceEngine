#pragma once
#include "../common/InnoType.h"

struct textureDataDesc
{
	textureType textureType;
	textureColorComponentsFormat textureColorComponentsFormat;
	texturePixelDataFormat texturePixelDataFormat;
	textureFilterMethod textureMinFilterMethod;
	textureFilterMethod textureMagFilterMethod;
	textureWrapMethod textureWrapMethod;
	unsigned int textureWidth;
	unsigned int textureHeight;
	texturePixelDataType texturePixelDataType;
};

class TextureDataComponent
{
public:
	TextureDataComponent() {};
	~TextureDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	textureDataDesc m_textureDataDesc;
	std::vector<void*> m_textureData;
};

