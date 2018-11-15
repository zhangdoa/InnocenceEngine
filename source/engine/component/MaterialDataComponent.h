#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "TextureDataComponent.h"

struct texturePack
{
	std::pair<textureType, TextureDataComponent*> m_normalTDC = { textureType::NORMAL, nullptr };
	std::pair<textureType, TextureDataComponent*> m_albedoTDC = { textureType::ALBEDO, nullptr };
	std::pair<textureType, TextureDataComponent*> m_metallicTDC = { textureType::METALLIC, nullptr };
	std::pair<textureType, TextureDataComponent*> m_roughnessTDC = { textureType::ROUGHNESS, nullptr };
	std::pair<textureType, TextureDataComponent*> m_aoTDC = { textureType::AMBIENT_OCCLUSION, nullptr };
};

class MaterialDataComponent
{
public:
	MaterialDataComponent() {};
	~MaterialDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	meshColor m_meshColor;
	texturePack m_texturePack;
};

