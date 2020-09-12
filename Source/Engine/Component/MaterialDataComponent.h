#pragma once
#include "../Common/InnoObject.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "TextureDataComponent.h"

enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };

struct TextureSlot
{
	TextureDataComponent* m_Texture = 0;
	bool m_Activate = false;
};

class MaterialDataComponent : public InnoComponent
{
public:
	static uint32_t GetTypeID() { return 7; };
	static char* GetTypeName() { return "MaterialDataComponent"; };

	MaterialAttributes m_materialAttributes = {};
	TextureSlot m_TextureSlots[8];
	ShaderModel m_ShaderModel = ShaderModel::Invalid;
	IResourceBinder* m_ResourceBinder = 0;
};
