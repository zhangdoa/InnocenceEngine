#pragma once
#include "MaterialComponent.h"
#include "../RenderingServer/VK/VKHeaders.h"

namespace Inno
{
	class VKMaterialComponent : public MaterialComponent
	{
	public:
		VkDescriptorSet m_descriptorSet;
		std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
	};
}