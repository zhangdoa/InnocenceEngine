#pragma once
#include "../Common/GPUDataStructure.h"
#include "GPUResourceComponent.h"
#include "TextureComponent.h"

namespace Inno
{
	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };
	
	class MaterialComponent : public GPUResourceComponent
	{
	public:
		static uint32_t GetTypeID() { return 7; };
		static const char* GetTypeName() { return "MaterialComponent"; };

		MaterialAttributes m_materialAttributes = {};
		Array<uint64_t> m_TextureComponents;
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}