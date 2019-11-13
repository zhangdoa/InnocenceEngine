#pragma once
#include "MaterialDataComponent.h"
#include "vulkan/vulkan.h"

class VKMaterialDataComponent : public MaterialDataComponent
{
public:
	VkDescriptorSet m_descriptorSet;
	std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
};
