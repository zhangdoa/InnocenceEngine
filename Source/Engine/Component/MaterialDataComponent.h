#pragma once
#include "../Common/InnoComponent.h"
#include "TextureDataComponent.h"

class MaterialDataComponent : public InnoComponent
{
public:
	MeshCustomMaterial m_meshCustomMaterial = {};
	TextureDataComponent* m_normalTexture = 0;
	TextureDataComponent* m_albedoTexture = 0;
	TextureDataComponent* m_metallicTexture = 0;
	TextureDataComponent* m_roughnessTexture = 0;
	TextureDataComponent* m_aoTexture = 0;
};
