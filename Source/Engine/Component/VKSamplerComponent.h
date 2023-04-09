#pragma once
#include "SamplerComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKSamplerComponent : public SamplerComponent
	{
	public:
		VkSamplerCreateInfo m_samplerCInfo = {};
		VkSampler m_sampler;
	};
}