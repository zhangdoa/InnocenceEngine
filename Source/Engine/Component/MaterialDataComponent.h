#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "TextureDataComponent.h"

struct TextureSlot
{
	TextureDataComponent* m_Texture = 0;
	bool m_Activate = false;
};

class MaterialDataComponent : public InnoComponent
{
public:
	MaterialAttributes m_materialAttributes = {};
	TextureSlot m_TextureSlots[8];
	IResourceBinder* m_ResourceBinder = 0;
};
