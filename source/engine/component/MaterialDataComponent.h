#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "TextureDataComponent.h"

struct TexturePack
{
	std::pair<TextureUsageType, TextureDataComponent*> m_normalTDC = { TextureUsageType::NORMAL, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_albedoTDC = { TextureUsageType::ALBEDO, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_metallicTDC = { TextureUsageType::METALLIC, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_roughnessTDC = { TextureUsageType::ROUGHNESS, nullptr };
	std::pair<TextureUsageType, TextureDataComponent*> m_aoTDC = { TextureUsageType::AMBIENT_OCCLUSION, nullptr };
};

class MaterialDataComponent
{
public:
	MaterialDataComponent() {};
	~MaterialDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	MeshCustomMaterial m_meshColor;
	TexturePack m_texturePack;
};

