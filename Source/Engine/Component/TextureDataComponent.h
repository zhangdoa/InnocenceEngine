#pragma once
#include "../Common/InnoComponent.h"

class TextureDataComponent : public InnoComponent
{
public:
	TextureDataDesc m_textureDataDesc = TextureDataDesc();
	void* m_textureData;
};
