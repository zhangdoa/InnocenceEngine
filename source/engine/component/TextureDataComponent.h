#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

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
