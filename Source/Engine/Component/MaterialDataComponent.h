#pragma once
#include "../Common/InnoComponent.h"
#include "TextureDataComponent.h"

struct TexturePack
{
	std::pair<TextureUsageType, TextureDataComponent*> m_normalTDC = { TextureUsageType::NORMAL, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_albedoTDC = { TextureUsageType::ALBEDO, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_metallicTDC = { TextureUsageType::METALLIC, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_roughnessTDC = { TextureUsageType::ROUGHNESS, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_aoTDC = { TextureUsageType::AMBIENT_OCCLUSION, nullptr };
};

class MaterialDataComponent : public InnoComponent
{
public:
	MaterialDataComponent() {};
	~MaterialDataComponent() {};

	MeshCustomMaterial m_meshCustomMaterial;
	TexturePack m_texturePack;
};
