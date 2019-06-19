#pragma once
#include "MaterialDataComponent.h"
#include "vulkan/vulkan.h"

class VKMaterialDataComponent : public MaterialDataComponent
{
public:
	VKMaterialDataComponent() {};
	~VKMaterialDataComponent() {};

	VkDescriptorSet m_descriptorSet;
};
