#pragma once
#include "GPUResourceComponent.h"
#include "TextureComponent.h"

namespace Inno
{
	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };

	struct TextureSlot
	{
		TextureComponent* m_Texture = 0;
		bool m_Activate = false;
	};

	struct MaterialAttributes
	{
		float AlbedoR = 1.0f;
		float AlbedoG = 1.0f;
		float AlbedoB = 1.0f;
		float Alpha = 1.0f;
		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float AO = 0.0f;
		float Thickness = 1.0f;
	};
	
	class MaterialComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 7; };
		static const char* GetTypeName() { return "MaterialComponent"; };

		MaterialAttributes m_materialAttributes = {};
		TextureSlot m_TextureSlots[8];
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}