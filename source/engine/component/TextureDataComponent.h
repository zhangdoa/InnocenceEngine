#pragma once
#include "../common/InnoType.h"

struct TextureDataDesc
{
	TextureUsageType textureUsageType;
	TextureColorComponentsFormat textureColorComponentsFormat;
	TexturePixelDataFormat texturePixelDataFormat;
	TextureFilterMethod textureMinFilterMethod;
	TextureFilterMethod textureMagFilterMethod;
	TextureWrapMethod textureWrapMethod;
	unsigned int textureWidth;
	unsigned int textureHeight;
	TexturePixelDataType texturePixelDataType;
};

class TextureDataComponent
{
public:
	TextureDataComponent() {};
	~TextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	TextureDataDesc m_textureDataDesc = TextureDataDesc();
	std::vector<void*> m_textureData;
};

