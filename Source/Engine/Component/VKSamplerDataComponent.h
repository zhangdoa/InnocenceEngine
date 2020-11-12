#pragma once
#include "SamplerDataComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKSamplerDataComponent : public SamplerDataComponent
	{
	public:
		VkSamplerCreateInfo m_samplerCInfo = {};
		VkSampler m_sampler;
	};
}