#include "VKHelper.h"
#include "../../Core/InnoLogger.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

bool VKHelper::checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
	uint32_t l_layerCount;
	vkEnumerateInstanceLayerProperties(&l_layerCount, nullptr);

	std::vector<VkLayerProperties> l_availableLayers(l_layerCount);
	vkEnumerateInstanceLayerProperties(&l_layerCount, l_availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : l_availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

bool VKHelper::checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VKHelper::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		VkBool32 l_presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, windowSurface, &l_presentSupport);

		if (queueFamily.queueCount > 0 && l_presentSupport)
		{
			indices.m_presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

VkSurfaceFormatKHR VKHelper::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VKHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR l_bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			l_bestMode = availablePresentMode;
		}
	}

	return l_bestMode;
}

VkExtent2D VKHelper::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

		VkExtent2D l_actualExtent;
		l_actualExtent.width = l_screenResolution.x;
		l_actualExtent.height = l_screenResolution.y;

		l_actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, l_actualExtent.width));
		l_actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, l_actualExtent.height));

		return l_actualExtent;
	}
}

SwapChainSupportDetails VKHelper::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR windowSurface)
{
	SwapChainSupportDetails l_details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &l_details.m_capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		l_details.m_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, l_details.m_formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		l_details.m_presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, l_details.m_presentModes.data());
	}

	return l_details;
}

bool VKHelper::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
	QueueFamilyIndices indices = findQueueFamilies(device, windowSurface);

	bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, windowSurface);
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

VkCommandBuffer VKHelper::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VKHelper::endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(commandQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(commandQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

uint32_t VKHelper::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to find suitable memory type!!");
	return 0;
}

bool VKHelper::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkBuffer!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDeviceMemory for VkBuffer!");
		return false;
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);

	return true;
}

bool VKHelper::copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(device, commandPool, commandQueue, commandBuffer);

	return true;
}

VkTextureDataDesc VKHelper::getVKTextureDataDesc(TextureDataDesc textureDataDesc)
{
	VkTextureDataDesc l_result;
	l_result.imageType = getImageType(textureDataDesc.SamplerType);
	l_result.samplerAddressMode = getSamplerAddressMode(textureDataDesc.WrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.MinFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.MagFilterMethod);
	l_result.format = getTextureFormat(textureDataDesc);
	l_result.imageSize = textureDataDesc.Width * textureDataDesc.Height * textureDataDesc.DepthOrArraySize * ((uint32_t)textureDataDesc.PixelDataFormat + 1);
	l_result.aspectFlags = getImageAspectFlags(textureDataDesc);
	return l_result;
}

VkImageType VKHelper::getImageType(TextureSamplerType rhs)
{
	VkImageType l_result;

	switch (rhs)
	{
	case TextureSamplerType::Sampler1D:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSamplerType::Sampler2D:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSamplerType::Sampler3D:
		l_result = VkImageType::VK_IMAGE_TYPE_3D;
		break;
	case TextureSamplerType::SamplerCubemap:
		// @TODO: Add cubemap support
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerAddressMode VKHelper::getSamplerAddressMode(TextureWrapMethod rhs)
{
	VkSamplerAddressMode l_result;

	switch (rhs)
	{
	case TextureWrapMethod::Edge:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TextureWrapMethod::Repeat:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TextureWrapMethod::Border:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerMipmapMode VKHelper::getTextureFilterParam(TextureFilterMethod rhs)
{
	VkSamplerMipmapMode l_result;

	switch (rhs)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

VkFormat VKHelper::getTextureFormat(TextureDataDesc textureDataDesc)
{
	VkFormat l_internalFormat = VK_FORMAT_R8_UNORM;

	if (textureDataDesc.UsageType == TextureUsageType::Albedo)
	{
		l_internalFormat = VK_FORMAT_R8G8B8A8_SRGB;
	}
	else if (textureDataDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D32_SFLOAT;
	}
	else if (textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		if (textureDataDesc.PixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R32G32B32A32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R32G32B32A32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SFLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SFLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R32_SFLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R32G32_SFLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

VkImageAspectFlagBits VKHelper::getImageAspectFlags(TextureDataDesc textureDataDesc)
{
	VkImageAspectFlagBits l_result;

	if (textureDataDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_result = VkImageAspectFlagBits(VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	else
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	}

	return l_result;
}

bool VKHelper::createDescriptorPool(VkDevice device, VkDescriptorPoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool& poolHandle)
{
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizeCount;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &poolHandle) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorPool!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorPool has been created.");
	return true;
}

bool VKHelper::createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = setLayoutBindingsCount;
	layoutInfo.pBindings = setLayoutBindings;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &setLayout) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorSetLayout!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorSetLayout has been created.");
	return true;
}

bool VKHelper::createDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout& setLayout, VkDescriptorSet& setHandle, uint32_t count)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = &setLayout;

	if (vkAllocateDescriptorSets(device, &allocInfo, &setHandle) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDescriptorSet!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorSet has been allocated.");
	return true;
}

bool VKHelper::updateDescriptorSet(VkDevice device, VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount)
{
	vkUpdateDescriptorSets(
		device,
		writeDescriptorSetsCount,
		writeDescriptorSets,
		0,
		nullptr);

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Write VkDescriptorSet has been updated.");
	return true;
}

bool VKHelper::reserveRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer)
{
	size_t l_framebufferNumber = 0;
	if (VKRPDC->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_framebufferNumber = VKRPDC->m_RenderPassDesc.m_RenderTargetCount;
		VKRPDC->m_RenderTargets.reserve(1);
		VKRPDC->m_RenderTargets.emplace_back();
	}
	else
	{
		l_framebufferNumber = 1;
		VKRPDC->m_RenderTargets.reserve(VKRPDC->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			VKRPDC->m_RenderTargets.emplace_back();
		}
	}

	// Reserve vectors and emplace empty objects
	VKRPDC->m_Framebuffers.reserve(l_framebufferNumber);
	for (size_t i = 0; i < l_framebufferNumber; i++)
	{
		VKRPDC->m_Framebuffers.emplace_back();
	}

	return true;
}

bool VKHelper::createRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer)
{
	for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		VKRPDC->m_RenderTargets[i] = renderingServer->AddTextureDataComponent((std::string(VKRPDC->m_componentName.c_str()) + "_" + std::to_string(i) + "/").c_str());

		VKRPDC->m_RenderTargets[i]->m_textureDataDesc = VKRPDC->m_RenderPassDesc.m_RenderTargetDesc;

		VKRPDC->m_RenderTargets[i]->m_textureData = nullptr;

		renderingServer->InitializeTextureDataComponent(VKRPDC->m_RenderTargets[i]);
	}

	if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		VKRPDC->m_DepthStencilRenderTarget = renderingServer->AddTextureDataComponent((std::string(VKRPDC->m_componentName.c_str()) + "_DS/").c_str());
		VKRPDC->m_DepthStencilRenderTarget->m_textureDataDesc = VKRPDC->m_RenderPassDesc.m_RenderTargetDesc;
		VKRPDC->m_DepthStencilRenderTarget->m_textureDataDesc.UsageType = TextureUsageType::DepthAttachment;
		if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer)
		{
			VKRPDC->m_DepthStencilRenderTarget->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			VKRPDC->m_DepthStencilRenderTarget->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}
		VKRPDC->m_DepthStencilRenderTarget->m_textureData = { nullptr };

		renderingServer->InitializeTextureDataComponent(VKRPDC->m_DepthStencilRenderTarget);
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Render targets have been created.");

	return true;
}

bool VKHelper::createRenderPass(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject*>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_RenderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	l_PSO->m_SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (VKRPDC->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_PSO->m_SubpassDesc.colorAttachmentCount = 1;
		l_PSO->m_RenderPassCInfo.attachmentCount = 1;
	}
	else
	{
		l_PSO->m_SubpassDesc.colorAttachmentCount = (uint32_t)VKRPDC->m_RenderPassDesc.m_RenderTargetCount;
		l_PSO->m_RenderPassCInfo.attachmentCount = (uint32_t)l_PSO->m_AttachmentDescs.size();
	}

	l_PSO->m_SubpassDesc.pColorAttachments = &l_PSO->m_ColorAttachmentRefs[0];
	if (l_PSO->m_DepthAttachmentRef.attachment)
	{
		l_PSO->m_SubpassDesc.pDepthStencilAttachment = &l_PSO->m_DepthAttachmentRef;
	}

	l_PSO->m_RenderPassCInfo.pSubpasses = &l_PSO->m_SubpassDesc;

	l_PSO->m_RenderPassCInfo.pAttachments = &l_PSO->m_AttachmentDescs[0];

	if (vkCreateRenderPass(device, &l_PSO->m_RenderPassCInfo, nullptr, &l_PSO->m_RenderPass) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkRenderPass!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkRenderPass has been created.");
	return true;
}

bool VKHelper::createSingleFramebuffer(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	// create frame buffer and attach image view
	auto l_PSO = reinterpret_cast<VKPipelineStateObject*>(VKRPDC->m_PipelineStateObject);

	std::vector<VkImageView> attachments(l_PSO->m_AttachmentDescs.size());

	for (size_t i = 0; i < VKRPDC->m_RenderTargets.size(); i++)
	{
		auto l_VKTDC = reinterpret_cast<VKTextureDataComponent*>(VKRPDC->m_RenderTargets[i]);
		attachments[i] = l_VKTDC->m_imageView;
	}

	if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		auto l_VKTDC = reinterpret_cast<VKTextureDataComponent*>(VKRPDC->m_DepthStencilRenderTarget);
		attachments[l_PSO->m_AttachmentDescs.size() - 1] = l_VKTDC->m_imageView;
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = l_PSO->m_RenderPass;
	framebufferInfo.attachmentCount = (uint32_t)attachments.size();
	framebufferInfo.pAttachments = &attachments[0];
	framebufferInfo.width = (uint32_t)l_PSO->m_Viewport.width;
	framebufferInfo.height = (uint32_t)l_PSO->m_Viewport.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &VKRPDC->m_Framebuffers[0]) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkFramebuffer!");
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Single VkFramebuffer has been created.");
	return true;
}

bool VKHelper::createMultipleFramebuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject*>(VKRPDC->m_PipelineStateObject);

	for (size_t i = 0; i < VKRPDC->m_RenderTargets.size(); i++)
	{
		// create frame buffer and attach image view
		auto l_VKTDC = reinterpret_cast<VKTextureDataComponent*>(VKRPDC->m_DepthStencilRenderTarget);
		VkImageView attachments[] = { l_VKTDC->m_imageView };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = l_PSO->m_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = (uint32_t)l_PSO->m_Viewport.width;
		framebufferInfo.height = (uint32_t)l_PSO->m_Viewport.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &VKRPDC->m_Framebuffers[i]) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkFramebuffer!");
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Multiple VkFramebuffers have been created.");
	return true;
}

bool VKHelper::createPipelineLayout(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject*>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_InputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	l_PSO->m_PipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_PSO->m_PipelineLayoutCInfo.setLayoutCount = static_cast<uint32_t>(VKRPDC->m_DescriptorSetLayouts.size());
	l_PSO->m_PipelineLayoutCInfo.pSetLayouts = VKRPDC->m_DescriptorSetLayouts.data();

	if (VKRPDC->m_PushConstantRanges.size() > 0)
	{
		l_PSO->m_PipelineLayoutCInfo.pushConstantRangeCount = static_cast<uint32_t>(VKRPDC->m_PushConstantRanges.size());
		l_PSO->m_PipelineLayoutCInfo.pPushConstantRanges = VKRPDC->m_PushConstantRanges.data();
	}

	if (vkCreatePipelineLayout(device, &l_PSO->m_PipelineLayoutCInfo, nullptr, &l_PSO->m_PipelineLayout) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkPipelineLayout!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkPipelineLayout has been created.");
	return true;
}

bool VKHelper::createGraphicsPipelines(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject*>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_ViewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	l_PSO->m_ViewportStateCInfo.pViewports = &l_PSO->m_Viewport;
	l_PSO->m_ViewportStateCInfo.pScissors = &l_PSO->m_Scissor;

	l_PSO->m_RasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	l_PSO->m_MultisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	l_PSO->m_ColorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	l_PSO->m_ColorBlendStateCInfo.attachmentCount = (uint32_t)l_PSO->m_ColorBlendAttachmentStates.size();
	l_PSO->m_ColorBlendStateCInfo.pAttachments = &l_PSO->m_ColorBlendAttachmentStates[0];

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent*>(VKRPDC->m_ShaderProgram);
	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStages = { l_VKSPC->m_vertexShaderStageCInfo, l_VKSPC->m_fragmentShaderStageCInfo };

	l_PSO->m_PipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_PSO->m_PipelineCInfo.stageCount = (uint32_t)l_shaderStages.size();
	l_PSO->m_PipelineCInfo.pStages = &l_shaderStages[0];
	l_PSO->m_PipelineCInfo.pVertexInputState = &l_VKSPC->m_vertexInputStateCInfo;
	l_PSO->m_PipelineCInfo.pInputAssemblyState = &l_PSO->m_InputAssemblyStateCInfo;
	l_PSO->m_PipelineCInfo.pViewportState = &l_PSO->m_ViewportStateCInfo;
	l_PSO->m_PipelineCInfo.pRasterizationState = &l_PSO->m_RasterizationStateCInfo;
	l_PSO->m_PipelineCInfo.pMultisampleState = &l_PSO->m_MultisampleStateCInfo;
	l_PSO->m_PipelineCInfo.pColorBlendState = &l_PSO->m_ColorBlendStateCInfo;
	l_PSO->m_PipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_PipelineCInfo.renderPass = l_PSO->m_RenderPass;
	l_PSO->m_PipelineCInfo.subpass = 0;
	l_PSO->m_PipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &l_PSO->m_PipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to to create VkPipeline!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkPipeline has been created.");
	return true;
}

bool VKHelper::createCommandBuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	VKRPDC->m_CommandLists.resize(VKRPDC->m_Framebuffers.size());

	for (size_t i = 0; i < VKRPDC->m_CommandLists.size(); i++)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = VKRPDC->m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		auto l_VKCommandList = reinterpret_cast<VKCommandList*>(VKRPDC->m_CommandLists[i]);
		if (vkAllocateCommandBuffers(device, &allocInfo, &l_VKCommandList->m_CommandBuffer) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkCommandBuffer!");
			return false;
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkCommandBuffer has been created.");
	return true;
}

bool VKHelper::createSyncPrimitives(VkDevice device, VKRenderPassDataComponent* VKRPDC)
{
	VKRPDC->m_SignalSemaphores.resize(VKRPDC->m_Framebuffers.size());
	VKRPDC->m_Fences.resize(VKRPDC->m_Framebuffers.size());

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VKRPDC->m_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	for (size_t i = 0; i < VKRPDC->m_Framebuffers.size(); i++)
	{
		auto l_VKSemaphore = reinterpret_cast<VKSemaphore*>(VKRPDC->m_SignalSemaphores[i]);
		auto l_VKFence = reinterpret_cast<VKFence*>(VKRPDC->m_Fences[i]);

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &l_VKSemaphore->m_Semaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &l_VKFence->m_Fence) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create synchronization primitives!");
			return false;
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Synchronization primitives has been created.");

	return true;
}