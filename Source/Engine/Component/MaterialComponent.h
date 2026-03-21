#pragma once
#include "../Common/GPUDataStructure.h"
#include "TextureComponent.h"

namespace Inno
{
	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };

	struct MaterialComponent
	{
		static uint32_t GetTypeID() { return 7; };
		static const char* GetTypeName() { return "MaterialComponent"; };

		GPUResourceType m_GPUResourceType = GPUResourceType::Sampler;
		Accessibility m_CPUAccessibility = Accessibility::WriteOnly;
		Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
		uint32_t m_ReadState = 0;
		uint32_t m_WriteState = 0;
		std::vector<DescriptorHandle> m_ReadHandles;
		std::vector<DescriptorHandle> m_WriteHandles;
		MaterialAttributes m_materialAttributes = {};
		std::vector<uint64_t> m_TextureComponents;
		ShaderModel m_ShaderModel = ShaderModel::Invalid;
	};
}