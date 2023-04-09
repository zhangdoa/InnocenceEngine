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