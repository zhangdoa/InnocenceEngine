#pragma once
#include "../Common/GPUDataStructure.h"
#include "GPUResourceComponent.h"
#include "TextureComponent.h"

namespace Inno
{
	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };
	
	struct MaterialComponent : public GPUResourceComponent
	{
		MaterialAttributes m_materialAttributes = {};
		std::vector<uint64_t> m_TextureComponents;
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}