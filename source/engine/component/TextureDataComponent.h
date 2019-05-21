#pragma once
#include "../common/InnoComponent.h"

class TextureDataComponent : public InnoComponent
{
public:
	TextureDataComponent() {};
	~TextureDataComponent() {};

	TextureDataDesc m_textureDataDesc = TextureDataDesc();
	std::vector<void*> m_textureData;
};
