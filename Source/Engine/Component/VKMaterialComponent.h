#pragma once
#include "MaterialComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKMaterialComponent : public MaterialComponent
	{
	public:
		VkDescriptorSet m_descriptorSet;
		std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
	};
}