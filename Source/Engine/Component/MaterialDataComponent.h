#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "TextureDataComponent.h"

enum TextureAttributeType
{
	Normal,
	Albedo,
	Metallic,
	Roughness,
	AmbientOcclusion,
};

struct TextureSlot
{
	uint32_t m_Index = 0;
	TextureAttributeType m_TextureAttributeType = TextureAttributeType::Normal;
	TextureDataComponent* m_Texture = 0;
	IResourceBinder* m_ResourceBinder = 0;
};

class MaterialDataComponent : public InnoComponent
{
public:
	MeshCustomMaterial m_meshCustomMaterial = {};
	TextureDataComponent* m_normalTexture = 0;
	TextureDataComponent* m_albedoTexture = 0;
	TextureDataComponent* m_metallicTexture = 0;
	TextureDataComponent* m_roughnessTexture = 0;
	TextureDataComponent* m_aoTexture = 0;
	std::vector<TextureSlot*> m_TextureSlots;
	std::vector<IResourceBinder*> m_ResourceBinders;
};
