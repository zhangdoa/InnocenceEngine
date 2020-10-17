#pragma once
#include "MaterialDataComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKMaterialDataComponent : public MaterialDataComponent
	{
	public:
		VkDescriptorSet m_descriptorSet;
		std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
	};
}