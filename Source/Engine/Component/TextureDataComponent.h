#pragma once
#include "../Common/InnoComponent.h"

class TextureDataComponent : public InnoComponent
{
public:
	TextureDataComponent() {};
	~TextureDataComponent() {};

	TextureDataDesc m_textureDataDesc = TextureDataDesc();
	void* m_textureData;
};
