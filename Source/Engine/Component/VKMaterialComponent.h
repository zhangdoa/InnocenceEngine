#pragma once
#include "../Common/Array.h"
#include "MaterialComponent.h"
#include "../Services/VK/VKHeaders.h"

namespace Inno
{
	class VKMaterialComponent : public MaterialComponent
	{
	public:
		VkDescriptorSet m_descriptorSet;
		Inno::Array<VkWriteDescriptorSet> m_writeDescriptorSets;
	};
}