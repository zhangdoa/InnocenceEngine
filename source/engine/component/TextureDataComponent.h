#pragma once
#include "../common/InnoType.h"
class TextureDataComponent
{
public:
	TextureDataComponent() { m_textureID = std::rand(); };
	~TextureDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	EntityID m_textureID;
	textureType m_textureType;
	textureColorComponentsFormat m_textureColorComponentsFormat;
	texturePixelDataFormat m_texturePixelDataFormat;
	textureFilterMethod m_textureMinFilterMethod;
	textureFilterMethod m_textureMagFilterMethod;
	textureWrapMethod m_textureWrapMethod;
	int m_textureWidth;
	int m_textureHeight;
	texturePixelDataType m_texturePixelDataType;
	std::vector<void *> m_textureData;
};

