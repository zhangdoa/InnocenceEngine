#pragma once
#include "../../Common/Array.h"
#include "vulkan/vulkan.h"

namespace Inno
{
    struct QueueFamilyIndices
	{
		std::optional<uint32_t> m_graphicsFamily;
		std::optional<uint32_t> m_presentFamily;
		std::optional<uint32_t> m_computeFamily;
		std::optional<uint32_t> m_transferFamily;

		bool isComplete()
		{
			return m_graphicsFamily.has_value() && m_presentFamily.has_value() && m_computeFamily.has_value() && m_transferFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		Inno::Array<VkSurfaceFormatKHR> m_formats;
		Inno::Array<VkPresentModeKHR> m_presentModes;
	};
}