#pragma once
#include "GPUResourceComponent.h"
#include "TextureDataComponent.h"

namespace Inno
{
	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };

	struct TextureSlot
	{
		TextureDataComponent* m_Texture = 0;
		bool m_Activate = false;
	};

	class MaterialDataComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 7; };
		static const char* GetTypeName() { return "MaterialDataComponent"; };

		MaterialAttributes m_materialAttributes = {};
		TextureSlot m_TextureSlots[8];
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}