#pragma once
#include "MaterialComponent.h"
#include "../Services/VK/VKHeaders.h"

namespace Inno
{
	class VKMaterialComponent : public MaterialComponent
	{
	public:
		VkDescriptorSet m_descriptorSet;
		std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
	};
}