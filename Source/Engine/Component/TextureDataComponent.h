#pragma once
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"

class TextureDataComponent : public InnoComponent
{
public:
	TextureDesc m_textureDesc = {};
	void* m_textureData = 0;
	IResourceBinder* m_ResourceBinder = 0;
};
