#pragma once
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"

class TextureDataComponent : public InnoComponent
{
public:
	TextureDesc m_TextureDesc = {};
	void* m_TextureData = 0;
	IResourceBinder* m_ResourceBinder = 0;
};
