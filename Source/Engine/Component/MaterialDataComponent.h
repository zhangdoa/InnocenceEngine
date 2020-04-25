#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "TextureDataComponent.h"

enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Debug };

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
	ShaderModel m_ShaderModel = ShaderModel::Invalid;
	IResourceBinder* m_ResourceBinder = 0;
};
