#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

struct TextureDataDesc
{
	TextureSamplerType samplerType;
	TextureUsageType usageType;
	TextureColorComponentsFormat colorComponentsFormat;
	TexturePixelDataFormat pixelDataFormat;
	TextureFilterMethod minFilterMethod;
	TextureFilterMethod magFilterMethod;
	TextureWrapMethod wrapMethod;
	TexturePixelDataType pixelDataType;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int depth = 0;
	vec4 borderColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

class TextureDataComponent
{
public:
	TextureDataComponent() {};
	~TextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	TextureDataDesc m_textureDataDesc = TextureDataDesc();
	std::vector<void*> m_textureData;
};
