#include "VKHelper.h"
#include "../../Core/InnoLogger.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine *g_Engine;

namespace Inno
{
	namespace VKHelper
	{
		const char *m_shaderRelativePath = "..//Res//Shaders//SPIRV//";
	}
} // namespace Inno

bool VKHelper::CheckValidationLayerSupport(const std::vector<const char *> &validationLayers)
{
	uint32_t l_layerCount;
	vkEnumerateInstanceLayerProperties(&l_layerCount, nullptr);

	std::vector<VkLayerProperties> l_availableLayers(l_layerCount);
	vkEnumerateInstanceLayerProperties(&l_layerCount, l_availableLayers.data());

	for (const char *layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto &layerProperties : l_availableLayers)
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

bool VKHelper::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto &extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VKHelper::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (const auto &queueFamily : queueFamilies)
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

VkSurfaceFormatKHR VKHelper::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto &availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VKHelper::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	VkPresentModeKHR l_bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto &availablePresentMode : availablePresentModes)
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

VkExtent2D VKHelper::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();

		VkExtent2D l_actualExtent;
		l_actualExtent.width = l_screenResolution.x;
		l_actualExtent.height = l_screenResolution.y;

		l_actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, l_actualExtent.width));
		l_actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, l_actualExtent.height));

		return l_actualExtent;
	}
}

SwapChainSupportDetails VKHelper::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR windowSurface)
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

bool VKHelper::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR windowSurface, const std::vector<const char *> &deviceExtensions)
{
	QueueFamilyIndices indices = FindQueueFamilies(device, windowSurface);

	bool extensionsSupported = CheckDeviceExtensionSupport(device, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, windowSurface);
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

VkCommandBuffer VKHelper::OpenTemporaryCommandBuffer(VkDevice device, VkCommandPool commandPool)
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

void VKHelper::CloseTemporaryCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer)
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

uint32_t VKHelper::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

bool VKHelper::CreateCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, VkDevice device, VkCommandPool &commandPool)
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, windowSurface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create CommandPool!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: CommandPool has been created.");
	return true;
}

bool VKHelper::CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkBufferCreateInfo &bufferCInfo, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
	if (vkCreateBuffer(device, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkBuffer!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDeviceMemory for VkBuffer!");
		return false;
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);

	return true;
}

bool VKHelper::CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = OpenTemporaryCommandBuffer(device, commandPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	CloseTemporaryCommandBuffer(device, commandPool, commandQueue, commandBuffer);

	return true;
}

bool VKHelper::CreateImage(VkPhysicalDevice physicalDevice, VkDevice device, const VkImageCreateInfo &imageCInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
{
	if (vkCreateImage(device, &imageCInfo, nullptr, &image) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkImage!");
		return false;
	}

	VkMemoryRequirements l_memRequirements;
	vkGetImageMemoryRequirements(device, image, &l_memRequirements);

	VkMemoryAllocateInfo l_allocInfo = {};
	l_allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	l_allocInfo.allocationSize = l_memRequirements.size;
	l_allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, l_memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device, &l_allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDeviceMemory for VkImage!");
		return false;
	}

	vkBindImageMemory(device, image, imageMemory, 0);

	return true;
}

VKTextureDesc VKHelper::GetVKTextureDesc(TextureDesc textureDesc)
{
	VKTextureDesc l_result;

	l_result.imageType = GetImageType(textureDesc.Sampler);
	l_result.imageViewType = GetImageViewType(textureDesc.Sampler);
	l_result.imageUsageFlags = GetImageUsageFlags(textureDesc.Usage);
	l_result.format = GetTextureFormat(textureDesc);
	l_result.imageSize = GetImageSize(textureDesc);
	l_result.aspectFlags = GetImageAspectFlags(textureDesc.Usage);

	return l_result;
}

VkImageType VKHelper::GetImageType(TextureSampler textureSampler)
{
	VkImageType l_result;

	switch (textureSampler)
	{
	case TextureSampler::Sampler1D:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSampler::Sampler2D:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSampler::Sampler3D:
		l_result = VkImageType::VK_IMAGE_TYPE_3D;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageViewType VKHelper::GetImageViewType(TextureSampler textureSampler)
{
	VkImageViewType l_result;

	switch (textureSampler)
	{
	case TextureSampler::Sampler1D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_1D;
		break;
	case TextureSampler::Sampler2D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
		break;
	case TextureSampler::Sampler3D:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_3D;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageUsageFlags VKHelper::GetImageUsageFlags(TextureUsage textureUsage)
{
	VkImageUsageFlags l_result = VK_IMAGE_USAGE_SAMPLED_BIT;

	if (textureUsage == TextureUsage::ColorAttachment)
	{
		l_result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	else if (textureUsage == TextureUsage::DepthAttachment)
	{
		l_result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else if (textureUsage == TextureUsage::DepthStencilAttachment)
	{
		l_result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else
	{
		l_result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	}

	return l_result;
}

VkSamplerAddressMode VKHelper::GetSamplerAddressMode(TextureWrapMethod textureWrapMethod)
{
	VkSamplerAddressMode l_result;

	switch (textureWrapMethod)
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

VkFilter VKHelper::GetFilter(TextureFilterMethod textureFilterMethod)
{
	VkFilter l_result;

	switch (textureFilterMethod)
	{
	case TextureFilterMethod::Nearest:
		l_result = VkFilter::VK_FILTER_NEAREST;
		break;
	case TextureFilterMethod::Linear:
		l_result = VkFilter::VK_FILTER_LINEAR;
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerMipmapMode VKHelper::GetSamplerMipmapMode(TextureFilterMethod minFilterMethod)
{
	VkSamplerMipmapMode l_result;

	switch (minFilterMethod)
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

VkFormat VKHelper::GetTextureFormat(TextureDesc textureDesc)
{
	VkFormat l_internalFormat = VK_FORMAT_R8_UNORM;

	if (textureDesc.IsSRGB)
	{
		l_internalFormat = VK_FORMAT_R8G8B8A8_SRGB;
	}
	else if (textureDesc.Usage == TextureUsage::DepthAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D32_SFLOAT;
	}
	else if (textureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		if (textureDesc.PixelDataType == TexturePixelDataType::UByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_UNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_UNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_UNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SByte)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_SNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_SNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_SNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_UNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_UNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SShort)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SNORM;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SNORM;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_UINT;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt8)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R8_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R8G8_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R8G8B8A8_SINT;
				break;
			case TexturePixelDataFormat::BGRA:
				l_internalFormat = VK_FORMAT_B8G8R8A8_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::UInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_UINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_UINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_UINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_UINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::SInt32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_SINT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_SINT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SINT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SINT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float16)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R16_SFLOAT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R16G16_SFLOAT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			default:
				break;
			}
		}
		else if (textureDesc.PixelDataType == TexturePixelDataType::Float32)
		{
			switch (textureDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:
				l_internalFormat = VK_FORMAT_R32_SFLOAT;
				break;
			case TexturePixelDataFormat::RG:
				l_internalFormat = VK_FORMAT_R32G32_SFLOAT;
				break;
			case TexturePixelDataFormat::RGB:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			case TexturePixelDataFormat::RGBA:
				l_internalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			default:
				break;
			}
		}
	}

	return l_internalFormat;
}

VkDeviceSize VKHelper::GetImageSize(TextureDesc textureDesc)
{
	VkDeviceSize l_result;

	VkDeviceSize l_singlePixelSize;

	switch (textureDesc.PixelDataType)
	{
	case TexturePixelDataType::UByte:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::SByte:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::UShort:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::SShort:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::UInt8:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::SInt8:
		l_singlePixelSize = 1;
		break;
	case TexturePixelDataType::UInt16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::SInt16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::UInt32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::SInt32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::Float16:
		l_singlePixelSize = 2;
		break;
	case TexturePixelDataType::Float32:
		l_singlePixelSize = 4;
		break;
	case TexturePixelDataType::Double:
		l_singlePixelSize = 8;
		break;
	}

	VkDeviceSize l_channelSize;

	switch (textureDesc.PixelDataFormat)
	{
	case TexturePixelDataFormat::R:
		l_channelSize = 1;
		break;
	case TexturePixelDataFormat::RG:
		l_channelSize = 2;
		break;
	case TexturePixelDataFormat::RGB:
		l_channelSize = 3;
		break;
	case TexturePixelDataFormat::RGBA:
		l_channelSize = 4;
		break;
	case TexturePixelDataFormat::Depth:
		l_channelSize = 1;
		break;
	case TexturePixelDataFormat::DepthStencil:
		l_channelSize = 1;
		break;
	}

	switch (textureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_result = textureDesc.Width * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler2D:
		l_result = textureDesc.Width * textureDesc.Height * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler3D:
		l_result = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler1DArray:
		l_result = textureDesc.Width * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::Sampler2DArray:
		l_result = textureDesc.Width * textureDesc.Height * textureDesc.DepthOrArraySize * l_singlePixelSize * l_channelSize;
		break;
	case TextureSampler::SamplerCubemap:
		l_result = textureDesc.Width * textureDesc.Height * 6 * l_singlePixelSize * l_channelSize;
		break;
	default:
		break;
	}

	return l_result;
}

VkImageAspectFlagBits VKHelper::GetImageAspectFlags(TextureUsage textureUsage)
{
	VkImageAspectFlagBits l_result;

	if (textureUsage == TextureUsage::DepthAttachment)
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (textureUsage == TextureUsage::DepthStencilAttachment)
	{
		l_result = VkImageAspectFlagBits(VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	else
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	}

	return l_result;
}

VkImageCreateInfo VKHelper::GetImageCreateInfo(TextureDesc textureDesc, VKTextureDesc vKTextureDesc)
{
	VkImageCreateInfo l_result = {};

	l_result.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	if (textureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		//l_result.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
	}
	else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_result.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	l_result.imageType = vKTextureDesc.imageType;
	l_result.extent.width = textureDesc.Width;
	l_result.extent.height = textureDesc.Height;
	if (textureDesc.Sampler == TextureSampler::Sampler3D)
	{
		l_result.extent.depth = textureDesc.DepthOrArraySize;
	}
	else
	{
		l_result.extent.depth = 1;
	}
	l_result.mipLevels = 1;
	if (textureDesc.Sampler == TextureSampler::Sampler1DArray ||
		textureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		l_result.arrayLayers = textureDesc.DepthOrArraySize;
	}
	else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_result.arrayLayers = 6;
	}
	else
	{
		l_result.arrayLayers = 1;
	}
	l_result.format = vKTextureDesc.format;
	l_result.tiling = VK_IMAGE_TILING_OPTIMAL;
	l_result.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_result.samples = VK_SAMPLE_COUNT_1_BIT;
	l_result.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	l_result.usage = vKTextureDesc.imageUsageFlags;

	return l_result;
}

bool VKHelper::TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier l_barrier = {};
	l_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	l_barrier.oldLayout = oldLayout;
	l_barrier.newLayout = newLayout;
	l_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	l_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	l_barrier.image = image;
	l_barrier.subresourceRange.aspectMask = aspectFlags;
	l_barrier.subresourceRange.baseMipLevel = 0;
	l_barrier.subresourceRange.levelCount = 1;
	l_barrier.subresourceRange.baseArrayLayer = 0;
	l_barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags l_sourceStage;
	VkPipelineStageFlags l_destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		l_barrier.srcAccessMask = 0;
		l_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		l_sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		l_destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		l_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		l_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		l_sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		l_destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		l_barrier.srcAccessMask = 0;
		l_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		l_sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		l_destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		l_barrier.srcAccessMask = 0;
		l_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		l_sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		l_destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Unsupported transition!");
		return false;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		l_sourceStage, l_destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &l_barrier);

	return true;
}

bool VKHelper::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height)
{
	VkBufferImageCopy l_region = {};
	l_region.bufferOffset = 0;
	l_region.bufferRowLength = 0;
	l_region.bufferImageHeight = 0;
	l_region.imageSubresource.aspectMask = aspectFlags;
	l_region.imageSubresource.mipLevel = 0;
	l_region.imageSubresource.baseArrayLayer = 0;
	l_region.imageSubresource.layerCount = 1;
	l_region.imageOffset = {0, 0, 0};
	l_region.imageExtent = {
		width,
		height,
		1};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_region);

	return true;
}

bool VKHelper::CreateImageView(VkDevice device, VKTextureDataComponent *VKTDC)
{
	VkImageViewCreateInfo l_viewCInfo = {};
	l_viewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	l_viewCInfo.image = VKTDC->m_image;
	l_viewCInfo.viewType = VKTDC->m_VKTextureDesc.imageViewType;
	l_viewCInfo.format = VKTDC->m_VKTextureDesc.format;
	l_viewCInfo.subresourceRange.aspectMask = VKTDC->m_VKTextureDesc.aspectFlags;
	l_viewCInfo.subresourceRange.baseMipLevel = 0;
	l_viewCInfo.subresourceRange.levelCount = 1;
	l_viewCInfo.subresourceRange.baseArrayLayer = 0;
	if (VKTDC->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray ||
		VKTDC->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray)
	{
		l_viewCInfo.subresourceRange.layerCount = VKTDC->m_TextureDesc.DepthOrArraySize;
	}
	else if (VKTDC->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
	{
		l_viewCInfo.subresourceRange.layerCount = 6;
	}
	else
	{
		l_viewCInfo.subresourceRange.layerCount = 1;
	}

	if (vkCreateImageView(device, &l_viewCInfo, nullptr, &VKTDC->m_imageView) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkImageView!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkImageView ", VKTDC->m_imageView, " is initialized.");

	return true;
}

bool VKHelper::CreateDescriptorSetLayoutBindings(VKRenderPassDataComponent *VKRPDC)
{
	std::sort(VKRPDC->m_ResourceBindingLayoutDescs.begin(), VKRPDC->m_ResourceBindingLayoutDescs.end(), [&](ResourceBindingLayoutDesc A, ResourceBindingLayoutDesc B) {
		return A.m_DescriptorIndex < B.m_DescriptorIndex;
	});

	std::sort(VKRPDC->m_ResourceBindingLayoutDescs.begin(), VKRPDC->m_ResourceBindingLayoutDescs.end(), [&](ResourceBindingLayoutDesc A, ResourceBindingLayoutDesc B) {
		return A.m_DescriptorSetIndex < B.m_DescriptorSetIndex;
	});

	auto l_resourceBinderLayoutDescsSize = VKRPDC->m_ResourceBindingLayoutDescs.size();

	size_t l_currentSetAbsoluteIndex = 0;
	size_t l_currentSetRelativeIndex = 0;

	for (size_t i = 0; i < l_resourceBinderLayoutDescsSize; i++)
	{
		auto l_resourceBinderLayoutDesc = VKRPDC->m_ResourceBindingLayoutDescs[i];

		if (l_currentSetAbsoluteIndex != l_resourceBinderLayoutDesc.m_DescriptorSetIndex)
		{
			l_currentSetAbsoluteIndex = l_resourceBinderLayoutDesc.m_DescriptorSetIndex;
			l_currentSetRelativeIndex++;
		}
	}

	VKRPDC->m_DescriptorSetLayoutBindings.reserve(l_resourceBinderLayoutDescsSize);
	VKRPDC->m_DescriptorSetLayoutBindingIndices.resize(l_currentSetRelativeIndex + 1);

	l_currentSetAbsoluteIndex = 0;
	l_currentSetRelativeIndex = 0;
	size_t l_currentBindingOffset = 0;

	for (size_t i = 0; i < l_resourceBinderLayoutDescsSize; i++)
	{
		auto l_resourceBinderLayoutDesc = VKRPDC->m_ResourceBindingLayoutDescs[i];

		VkDescriptorSetLayoutBinding l_descriptorLayoutBinding = {};
		l_descriptorLayoutBinding.binding = (uint32_t)l_resourceBinderLayoutDesc.m_DescriptorIndex;
		l_descriptorLayoutBinding.descriptorCount = 1;
		l_descriptorLayoutBinding.pImmutableSamplers = nullptr;
		l_descriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		switch (l_resourceBinderLayoutDesc.m_GPUResourceType)
		{
		case GPUResourceType::Sampler:
			l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case GPUResourceType::Image:
			if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
			{
				l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			else
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				}
			}
			break;
		case GPUResourceType::Buffer:
			if (l_resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				}
			}
			else
			{
				if (l_resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				{
					InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Not allow to create write-only or read-write ResourceBinderLayout to read-only buffer!");
				}
				else
				{
					l_descriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				}
			}
			break;
		default:
			break;
		}

		VKRPDC->m_DescriptorSetLayoutBindings.emplace_back(l_descriptorLayoutBinding);

		if (l_currentSetAbsoluteIndex != l_resourceBinderLayoutDesc.m_DescriptorSetIndex)
		{
			l_currentSetAbsoluteIndex = l_resourceBinderLayoutDesc.m_DescriptorSetIndex;
			l_currentSetRelativeIndex++;
			VKRPDC->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_LayoutBindingOffset = i;
		}

		VKRPDC->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_SetIndex = l_currentSetAbsoluteIndex;
		VKRPDC->m_DescriptorSetLayoutBindingIndices[l_currentSetRelativeIndex].m_BindingCount++;
	}

	return true;
}

bool VKHelper::createDescriptorPool(VkDevice device, VkDescriptorPoolSize *poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool &poolHandle)
{
	VkDescriptorPoolCreateInfo l_poolCInfo = {};
	l_poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	l_poolCInfo.poolSizeCount = poolSizeCount;
	l_poolCInfo.pPoolSizes = poolSize;
	l_poolCInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(device, &l_poolCInfo, nullptr, &poolHandle) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorPool!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorPool has been created.");
	return true;
}

bool VKHelper::createDescriptorPool(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	// Currently support less than 10 descriptor types actually
	std::array<uint32_t, 10> l_descriptorTypeCount = {};

	for (auto i : VKRPDC->m_DescriptorSetLayoutBindings)
	{
		l_descriptorTypeCount[i.descriptorType]++;
	}

	// What a name
	auto l_VkDescriptorPoolSizesSize = std::count_if(l_descriptorTypeCount.begin(), l_descriptorTypeCount.end(), [](uint32_t i) { return i != 0; });

	std::vector<VkDescriptorPoolSize> l_descriptorPoolSizes(l_VkDescriptorPoolSizesSize);

	size_t l_index = 0;
	for (size_t i = 0; i < l_descriptorTypeCount.size(); i++)
	{
		if (l_descriptorTypeCount[i])
		{
			l_descriptorPoolSizes[l_index].type = VkDescriptorType(i);
			l_descriptorPoolSizes[l_index].descriptorCount = l_descriptorTypeCount[i];
			l_index++;
		}
	}

	return createDescriptorPool(device, &l_descriptorPoolSizes[0], (uint32_t)l_descriptorPoolSizes.size(), (uint32_t)VKRPDC->m_ResourceBindingLayoutDescs[VKRPDC->m_ResourceBindingLayoutDescs.size() - 1].m_DescriptorSetIndex + 1, VKRPDC->m_DescriptorPool);
}

bool VKHelper::createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutBinding *setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout &setLayout)
{
	VkDescriptorSetLayoutCreateInfo l_layoutCInfo = {};
	l_layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	l_layoutCInfo.bindingCount = setLayoutBindingsCount;
	l_layoutCInfo.pBindings = setLayoutBindings;

	if (vkCreateDescriptorSetLayout(device, &l_layoutCInfo, nullptr, &setLayout) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorSetLayout!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorSetLayout has been created.");
	return true;
}

bool VKHelper::createDescriptorSets(VkDevice device, VkDescriptorPool pool, const VkDescriptorSetLayout *setLayout, VkDescriptorSet &setHandle, uint32_t count)
{
	VkDescriptorSetAllocateInfo l_allocCInfo = {};
	l_allocCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	l_allocCInfo.descriptorPool = pool;
	l_allocCInfo.descriptorSetCount = count;
	l_allocCInfo.pSetLayouts = setLayout;

	if (vkAllocateDescriptorSets(device, &l_allocCInfo, &setHandle) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDescriptorSet!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkDescriptorSet has been allocated.");
	return true;
}

bool VKHelper::UpdateDescriptorSet(VkDevice device, VkWriteDescriptorSet *writeDescriptorSets, uint32_t writeDescriptorSetsCount)
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

bool VKHelper::ReserveRenderTargets(VKRenderPassDataComponent *VKRPDC, IRenderingServer *renderingServer)
{
	auto l_framebufferCount = VKRPDC->m_RenderPassDesc.m_UseMultiFrames ? VKRPDC->m_RenderPassDesc.m_RenderTargetCount : 1;

	if (VKRPDC->m_RenderPassDesc.m_UseColorBuffer)
	{
		if (VKRPDC->m_RenderPassDesc.m_UseMultiFrames)
		{
			VKRPDC->m_RenderTargets.reserve(1);
			VKRPDC->m_RenderTargets.emplace_back();
		}
		else
		{
			VKRPDC->m_RenderTargets.reserve(VKRPDC->m_RenderPassDesc.m_RenderTargetCount);
			for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				VKRPDC->m_RenderTargets.emplace_back();
			}
		}

		InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " render targets have been allocated.");
	}

	VKRPDC->m_Framebuffers.reserve(l_framebufferCount);
	for (size_t i = 0; i < l_framebufferCount; i++)
	{
		VKRPDC->m_Framebuffers.emplace_back();
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " framebuffers have been allocated.");

	return true;
}

bool VKHelper::CreateRenderTargets(VKRenderPassDataComponent *VKRPDC, IRenderingServer *renderingServer)
{
	if (VKRPDC->m_RenderPassDesc.m_UseColorBuffer)
	{
		for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			VKRPDC->m_RenderTargets[i] = renderingServer->AddTextureDataComponent((std::string(VKRPDC->m_InstanceName.c_str()) + "_" + std::to_string(i) + "/").c_str());

			VKRPDC->m_RenderTargets[i]->m_TextureDesc = VKRPDC->m_RenderPassDesc.m_RenderTargetDesc;

			VKRPDC->m_RenderTargets[i]->m_TextureData = nullptr;

			renderingServer->InitializeTextureDataComponent(VKRPDC->m_RenderTargets[i]);
		}
	}
	if (VKRPDC->m_RenderPassDesc.m_UseDepthBuffer)
	{
		VKRPDC->m_DepthStencilRenderTarget = renderingServer->AddTextureDataComponent((std::string(VKRPDC->m_InstanceName.c_str()) + "_DS/").c_str());
		VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc = VKRPDC->m_RenderPassDesc.m_RenderTargetDesc;

		if (VKRPDC->m_RenderPassDesc.m_UseStencilBuffer)
		{
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			VKRPDC->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}
		VKRPDC->m_DepthStencilRenderTarget->m_TextureData = {nullptr};

		renderingServer->InitializeTextureDataComponent(VKRPDC->m_DepthStencilRenderTarget);
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " render targets have been created.");

	return true;
}

bool VKHelper::CreateRenderPass(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_RenderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	if (VKRPDC->m_RenderPassDesc.m_UseOutputMerger)
	{
		l_PSO->m_ColorAttachmentRefs.reserve(VKRPDC->m_RenderPassDesc.m_RenderTargetCount);

		for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			VkAttachmentReference l_colorAttachmentRef = {};
			l_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_colorAttachmentRef.attachment = (uint32_t)i;

			l_PSO->m_ColorAttachmentRefs.emplace_back(l_colorAttachmentRef);
		}

		VkAttachmentDescription l_colorAttachmentDesc = {};
		l_colorAttachmentDesc.format = GetTextureFormat(VKRPDC->m_RenderPassDesc.m_RenderTargetDesc);
		l_colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		l_colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		l_colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		for (size_t i = 0; i < VKRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			l_PSO->m_AttachmentDescs.emplace_back(l_colorAttachmentDesc);
		}

		// last attachment is depth attachment
		if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			l_PSO->m_DepthAttachmentRef.attachment = (uint32_t)VKRPDC->m_RenderPassDesc.m_RenderTargetCount;
			l_PSO->m_DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription l_depthAttachmentDesc = {};

			if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D24_UNORM_S8_UINT;
			}
			else
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D32_SFLOAT;
			}
			l_depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			l_depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			l_depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			l_depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			l_PSO->m_AttachmentDescs.emplace_back(l_depthAttachmentDesc);
		}
	}

	l_PSO->m_RenderPassCInfo.subpassCount = 1;

	l_PSO->m_SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (VKRPDC->m_RenderPassDesc.m_UseOutputMerger)
	{
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

		if (l_PSO->m_SubpassDesc.colorAttachmentCount)
		{
			l_PSO->m_SubpassDesc.pColorAttachments = &l_PSO->m_ColorAttachmentRefs[0];
		}

		if (l_PSO->m_DepthAttachmentRef.attachment)
		{
			l_PSO->m_SubpassDesc.pDepthStencilAttachment = &l_PSO->m_DepthAttachmentRef;
		}

		if (l_PSO->m_RenderPassCInfo.attachmentCount)
		{
			l_PSO->m_RenderPassCInfo.pAttachments = &l_PSO->m_AttachmentDescs[0];
		}
	}

	l_PSO->m_RenderPassCInfo.pSubpasses = &l_PSO->m_SubpassDesc;

	if (vkCreateRenderPass(device, &l_PSO->m_RenderPassCInfo, nullptr, &l_PSO->m_RenderPass) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " failed to create VkRenderPass!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkRenderPass has been created for ", VKRPDC->m_InstanceName.c_str());
	return true;
}

bool VKHelper::CreateViewportAndScissor(VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_Viewport.width = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width;
	l_PSO->m_Viewport.height = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height;
	l_PSO->m_Viewport.maxDepth = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MaxDepth;
	l_PSO->m_Viewport.minDepth = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MinDepth;
	l_PSO->m_Viewport.x = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginX;
	l_PSO->m_Viewport.y = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginY;

	l_PSO->m_Scissor.offset = {0, 0};
	l_PSO->m_Scissor.extent.width = (uint32_t)l_PSO->m_Viewport.width;
	l_PSO->m_Scissor.extent.height = (uint32_t)l_PSO->m_Viewport.height;

	return true;
}

bool VKHelper::CreateSingleFramebuffer(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);
	VkFramebufferCreateInfo l_framebufferCInfo = {};
	l_framebufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	l_framebufferCInfo.renderPass = l_PSO->m_RenderPass;
	l_framebufferCInfo.width = (uint32_t)l_PSO->m_Viewport.width;
	l_framebufferCInfo.height = (uint32_t)l_PSO->m_Viewport.height;
	l_framebufferCInfo.layers = 1;

	std::vector<VkImageView> l_attachments(l_PSO->m_AttachmentDescs.size());

	VkFramebufferAttachmentsCreateInfo l_framebufferAttachmentsCInfo = {};
	l_framebufferAttachmentsCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
	l_framebufferAttachmentsCInfo.attachmentImageInfoCount = 0;

	if (VKRPDC->m_RenderPassDesc.m_UseOutputMerger)
	{
		for (size_t i = 0; i < VKRPDC->m_RenderTargets.size(); i++)
		{
			auto l_VKTDC = reinterpret_cast<VKTextureDataComponent *>(VKRPDC->m_RenderTargets[i]);
			l_attachments[i] = l_VKTDC->m_imageView;
		}

		if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			auto l_VKTDC = reinterpret_cast<VKTextureDataComponent *>(VKRPDC->m_DepthStencilRenderTarget);
			l_attachments[l_attachments.size() - 1] = l_VKTDC->m_imageView;
		}
		l_framebufferCInfo.attachmentCount = (uint32_t)l_attachments.size();
		if (l_attachments.size())
		{
			l_framebufferCInfo.pAttachments = &l_attachments[0];
		}
	}
	else
	{
		l_framebufferCInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
		l_framebufferCInfo.pNext = &l_framebufferAttachmentsCInfo;
	}

	if (vkCreateFramebuffer(device, &l_framebufferCInfo, nullptr, &VKRPDC->m_Framebuffers[0]) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " failed to create VkFramebuffer!");
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Single VkFramebuffer has been created for ", VKRPDC->m_InstanceName.c_str());

	return true;
}

bool VKHelper::CreateMultipleFramebuffers(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	VkFramebufferCreateInfo l_framebufferCInfo = {};
	l_framebufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	l_framebufferCInfo.renderPass = l_PSO->m_RenderPass;
	l_framebufferCInfo.width = (uint32_t)l_PSO->m_Viewport.width;
	l_framebufferCInfo.height = (uint32_t)l_PSO->m_Viewport.height;
	l_framebufferCInfo.layers = 1;

	for (size_t i = 0; i < VKRPDC->m_RenderTargets.size(); i++)
	{
		auto l_attachmentCount = VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable ? 2 : 1;
		std::vector<VkImageView> l_attachments(l_attachmentCount);

		if (VKRPDC->m_RenderPassDesc.m_UseOutputMerger)
		{
			auto l_VKTDC = reinterpret_cast<VKTextureDataComponent *>(VKRPDC->m_RenderTargets[i]);
			l_attachments[0] = l_VKTDC->m_imageView;
			if (VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
			{
				auto l_VKTDC = reinterpret_cast<VKTextureDataComponent *>(VKRPDC->m_DepthStencilRenderTarget);
				l_attachments[1] = l_VKTDC->m_imageView;
			}
			l_framebufferCInfo.attachmentCount = (uint32_t)l_attachments.size();
			l_framebufferCInfo.pAttachments = &l_attachments[0];
		}
		else
		{
			l_framebufferCInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
		}

		if (vkCreateFramebuffer(device, &l_framebufferCInfo, nullptr, &VKRPDC->m_Framebuffers[i]) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: ", VKRPDC->m_InstanceName.c_str(), " failed to create VkFramebuffer!");
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Multiple VkFramebuffers have been created for ", VKRPDC->m_InstanceName.c_str());

	return true;
}

VkCompareOp VKHelper::GetComparisionFunctionEnum(ComparisionFunction comparisionFunction)
{
	VkCompareOp l_result;

	switch (comparisionFunction)
	{
	case ComparisionFunction::Never:
		l_result = VkCompareOp::VK_COMPARE_OP_NEVER;
		break;
	case ComparisionFunction::Less:
		l_result = VkCompareOp::VK_COMPARE_OP_LESS;
		break;
	case ComparisionFunction::Equal:
		l_result = VkCompareOp::VK_COMPARE_OP_EQUAL;
		break;
	case ComparisionFunction::LessEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case ComparisionFunction::Greater:
		l_result = VkCompareOp::VK_COMPARE_OP_GREATER;
		break;
	case ComparisionFunction::NotEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_NOT_EQUAL;
		break;
	case ComparisionFunction::GreaterEqual:
		l_result = VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;
		break;
	case ComparisionFunction::Always:
		l_result = VkCompareOp::VK_COMPARE_OP_ALWAYS;
		break;
	default:
		break;
	}

	return l_result;
}

VkStencilOp VKHelper::GetStencilOperationEnum(StencilOperation stencilOperation)
{
	VkStencilOp l_result;

	switch (stencilOperation)
	{
	case StencilOperation::Keep:
		l_result = VkStencilOp::VK_STENCIL_OP_KEEP;
		break;
	case StencilOperation::Zero:
		l_result = VkStencilOp::VK_STENCIL_OP_ZERO;
		break;
	case StencilOperation::Replace:
		l_result = VkStencilOp::VK_STENCIL_OP_REPLACE;
		break;
	case StencilOperation::IncreaseSat:
		l_result = VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;
		break;
	case StencilOperation::DecreaseSat:
		l_result = VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;
		break;
	case StencilOperation::Invert:
		l_result = VkStencilOp::VK_STENCIL_OP_INVERT;
		break;
	case StencilOperation::Increase:
		l_result = VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		break;
	case StencilOperation::Decrease:
		l_result = VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		break;
	default:
		break;
	}

	return l_result;
}

VkBlendFactor VKHelper::GetBlendFactorEnum(BlendFactor blendFactor)
{
	VkBlendFactor l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		break;
	case BlendFactor::One:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		break;
	case BlendFactor::SrcColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case BlendFactor::DestColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case BlendFactor::DestAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case BlendFactor::Src1Color:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha:
		l_result = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

VkBlendOp VKHelper::GetBlendOperation(BlendOperation blendOperation)
{
	VkBlendOp l_result;

	switch (blendOperation)
	{
	case BlendOperation::Add:
		l_result = VkBlendOp::VK_BLEND_OP_ADD;
		break;
	case BlendOperation::Substruct:
		l_result = VkBlendOp::VK_BLEND_OP_SUBTRACT;
		break;
	default:
		break;
	}

	return l_result;
}

bool VKHelper::CreatePipelineLayout(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	l_PSO->m_PipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_PSO->m_PipelineLayoutCInfo.setLayoutCount = static_cast<uint32_t>(VKRPDC->m_DescriptorSetLayouts.size());
	l_PSO->m_PipelineLayoutCInfo.pSetLayouts = &VKRPDC->m_DescriptorSetLayouts[0];

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

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkPipelineLayout has been created for ", VKRPDC->m_InstanceName.c_str());
	return true;
}

bool VKHelper::GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject *PSO)
{
	PSO->m_ViewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	PSO->m_ViewportStateCInfo.viewportCount = 1;
	PSO->m_ViewportStateCInfo.pViewports = &PSO->m_Viewport;
	PSO->m_ViewportStateCInfo.scissorCount = 1;
	PSO->m_ViewportStateCInfo.pScissors = &PSO->m_Scissor;

	return true;
}

bool VKHelper::GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject *PSO)
{
	PSO->m_InputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	switch (rasterizerDesc.m_PrimitiveTopology)
	{
	case PrimitiveTopology::Point:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case PrimitiveTopology::Line:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case PrimitiveTopology::TriangleList:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case PrimitiveTopology::TriangleStrip:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	case PrimitiveTopology::Patch:
		PSO->m_InputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		break;
	default:
		break;
	}
	PSO->m_InputAssemblyStateCInfo.primitiveRestartEnable = false;

	PSO->m_RasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	if (rasterizerDesc.m_UseCulling)
	{
		switch (rasterizerDesc.m_RasterizerCullMode)
		{
		case RasterizerCullMode::Back:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case RasterizerCullMode::Front:
			PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		default:
			break;
		}
	}
	else
	{
		PSO->m_RasterizationStateCInfo.cullMode = VK_CULL_MODE_NONE;
	}

	switch (rasterizerDesc.m_RasterizerFillMode)
	{
	case InnoType::RasterizerFillMode::Point:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_POINT;
		break;
	case InnoType::RasterizerFillMode::Wireframe:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_LINE;
		break;
	case InnoType::RasterizerFillMode::Solid:
		PSO->m_RasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
		break;
	default:
		break;
	}

	PSO->m_RasterizationStateCInfo.frontFace = (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	PSO->m_RasterizationStateCInfo.lineWidth = 1.0f;

	PSO->m_MultisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	PSO->m_MultisampleStateCInfo.sampleShadingEnable = rasterizerDesc.m_AllowMultisample;
	PSO->m_MultisampleStateCInfo.rasterizationSamples = rasterizerDesc.m_AllowMultisample ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

	return true;
}

bool VKHelper::GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject *PSO)
{
	PSO->m_DepthStencilStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	PSO->m_DepthStencilStateCInfo.depthTestEnable = depthStencilDesc.m_DepthEnable;
	PSO->m_DepthStencilStateCInfo.depthWriteEnable = depthStencilDesc.m_AllowDepthWrite;
	PSO->m_DepthStencilStateCInfo.depthCompareOp = GetComparisionFunctionEnum(depthStencilDesc.m_DepthComparisionFunction);
	PSO->m_DepthStencilStateCInfo.depthBoundsTestEnable = VK_FALSE;
	PSO->m_DepthStencilStateCInfo.minDepthBounds = 0.0f; // Optional
	PSO->m_DepthStencilStateCInfo.maxDepthBounds = 1.0f; // Optional

	PSO->m_DepthStencilStateCInfo.stencilTestEnable = depthStencilDesc.m_StencilEnable;

	PSO->m_DepthStencilStateCInfo.front.failOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.front.passOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.front.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_FrontFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.front.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_FrontFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.front.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.front.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.front.reference = depthStencilDesc.m_StencilReference;

	PSO->m_DepthStencilStateCInfo.back.failOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilFailOperation);
	PSO->m_DepthStencilStateCInfo.back.passOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassOperation);
	PSO->m_DepthStencilStateCInfo.back.depthFailOp = GetStencilOperationEnum(depthStencilDesc.m_BackFaceStencilPassDepthFailOperation);
	PSO->m_DepthStencilStateCInfo.back.compareOp = GetComparisionFunctionEnum(depthStencilDesc.m_BackFaceStencilComparisionFunction);
	PSO->m_DepthStencilStateCInfo.back.compareMask = 0xFF;
	PSO->m_DepthStencilStateCInfo.back.writeMask = depthStencilDesc.m_StencilWriteMask;
	PSO->m_DepthStencilStateCInfo.back.reference = depthStencilDesc.m_StencilReference;

	return true;
}

bool VKHelper::GenerateBlendState(BlendDesc blendDesc, size_t RTCount, VKPipelineStateObject *PSO)
{
	PSO->m_ColorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineColorBlendAttachmentState l_colorBlendAttachmentState = {};
	l_colorBlendAttachmentState.blendEnable = blendDesc.m_UseBlend;
	l_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_colorBlendAttachmentState.srcColorBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceRGBFactor);
	l_colorBlendAttachmentState.srcAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_SourceAlphaFactor);
	l_colorBlendAttachmentState.dstColorBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationRGBFactor);
	l_colorBlendAttachmentState.dstAlphaBlendFactor = GetBlendFactorEnum(blendDesc.m_DestinationAlphaFactor);
	l_colorBlendAttachmentState.colorBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);
	l_colorBlendAttachmentState.alphaBlendOp = GetBlendOperation(blendDesc.m_BlendOperation);

	PSO->m_ColorBlendAttachmentStates.reserve(RTCount);

	for (size_t i = 0; i < RTCount; i++)
	{
		PSO->m_ColorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);
	}

	PSO->m_ColorBlendStateCInfo.logicOpEnable = VK_FALSE;
	PSO->m_ColorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;

	PSO->m_ColorBlendStateCInfo.blendConstants[0] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[1] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[2] = 0.0f;
	PSO->m_ColorBlendStateCInfo.blendConstants[3] = 0.0f;

	if (PSO->m_ColorBlendAttachmentStates.size())
	{
		PSO->m_ColorBlendStateCInfo.attachmentCount = (uint32_t)PSO->m_ColorBlendAttachmentStates.size();
		PSO->m_ColorBlendStateCInfo.pAttachments = &PSO->m_ColorBlendAttachmentStates[0];
	}

	return true;
}

bool VKHelper::CreateGraphicsPipelines(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	GenerateViewportState(VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);
	GenerateRasterizerState(VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
	GenerateDepthStencilState(VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
	GenerateBlendState(VKRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, VKRPDC->m_RenderPassDesc.m_RenderTargetCount, l_PSO);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRPDC->m_ShaderProgram);
	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStageCInfos;
	l_shaderStageCInfos.reserve(6);

	if (l_VKSPC->m_ShaderFilePaths.m_VSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_VSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_HSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_HSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_DSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_DSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_GSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_GSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_PSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_PSCInfo);
	}

	l_PSO->m_GraphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_PSO->m_GraphicsPipelineCInfo.stageCount = (uint32_t)l_shaderStageCInfos.size();
	l_PSO->m_GraphicsPipelineCInfo.pStages = &l_shaderStageCInfos[0];
	l_PSO->m_GraphicsPipelineCInfo.pVertexInputState = &l_VKSPC->m_vertexInputStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pInputAssemblyState = &l_PSO->m_InputAssemblyStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pViewportState = &l_PSO->m_ViewportStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pRasterizationState = &l_PSO->m_RasterizationStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pMultisampleState = &l_PSO->m_MultisampleStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pDepthStencilState = &l_PSO->m_DepthStencilStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pColorBlendState = &l_PSO->m_ColorBlendStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_GraphicsPipelineCInfo.renderPass = l_PSO->m_RenderPass;
	l_PSO->m_GraphicsPipelineCInfo.subpass = 0;
	l_PSO->m_GraphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &l_PSO->m_GraphicsPipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkPipeline for GraphicsPipeline!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkPipeline for GraphicsPipeline has been created for ", VKRPDC->m_InstanceName.c_str());
	return true;
}

bool VKHelper::CreateComputePipelines(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRPDC->m_PipelineStateObject);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRPDC->m_ShaderProgram);

	l_PSO->m_ComputePipelineCInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	l_PSO->m_ComputePipelineCInfo.stage = l_VKSPC->m_CSCInfo;
	l_PSO->m_ComputePipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_ComputePipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &l_PSO->m_ComputePipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkPipeline for ComputePipeline!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkPipeline for ComputePipeline has been created for ", VKRPDC->m_InstanceName.c_str());
	return true;
}

bool VKHelper::CreateCommandBuffers(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	if (VKRPDC->m_RenderPassDesc.m_UseMultiFrames)
	{
		VKRPDC->m_CommandLists.resize(VKRPDC->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		VKRPDC->m_CommandLists.resize(1);
	}

	for (size_t i = 0; i < VKRPDC->m_CommandLists.size(); i++)
	{
		VkCommandBufferAllocateInfo l_allocInfo = {};
		l_allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		l_allocInfo.commandPool = VKRPDC->m_CommandPool;
		l_allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_allocInfo.commandBufferCount = 1;

		auto l_VKCommandList = reinterpret_cast<VKCommandList *>(VKRPDC->m_CommandLists[i]);
		if (vkAllocateCommandBuffers(device, &l_allocInfo, &l_VKCommandList->m_CommandBuffer) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkCommandBuffer!");
			return false;
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkCommandBuffer has been created for ", VKRPDC->m_InstanceName.c_str());
	return true;
}

bool VKHelper::CreateSyncPrimitives(VkDevice device, VKRenderPassDataComponent *VKRPDC)
{
	VkSemaphoreCreateInfo l_semaphoreInfo = {};
	l_semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo l_fenceInfo = {};
	l_fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	l_fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VKRPDC->m_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	for (size_t i = 0; i < VKRPDC->m_SignalSemaphores.size(); i++)
	{
		auto l_VKSemaphore = reinterpret_cast<VKSemaphore *>(VKRPDC->m_SignalSemaphores[i]);
		auto l_VKFence = reinterpret_cast<VKFence *>(VKRPDC->m_Fences[i]);

		if (vkCreateSemaphore(device, &l_semaphoreInfo, nullptr, &l_VKSemaphore->m_Semaphore) != VK_SUCCESS ||
			vkCreateFence(device, &l_fenceInfo, nullptr, &l_VKFence->m_Fence) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create synchronization primitives!");
			return false;
		}
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: Synchronization primitives has been created for ", VKRPDC->m_InstanceName.c_str());

	return true;
}

bool VKHelper::CreateShaderModule(VkDevice device, VkShaderModule &vkShaderModule, const ShaderFilePath &shaderFilePath)
{
	auto l_shaderFileName = m_shaderRelativePath + std::string(shaderFilePath.c_str()) + ".spv";
	auto l_shaderContent = g_Engine->getFileSystem()->loadFile(l_shaderFileName.c_str(), IOMode::Binary);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_shaderContent.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t *>(l_shaderContent.data());

	if (vkCreateShaderModule(device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkShaderModule for: ", shaderFilePath.c_str(), "!");
		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: ", shaderFilePath.c_str(), " has been loaded.");
	return true;
}
VkWriteDescriptorSet VKHelper::GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.dstSet = descriptorSet;

	return l_result;
}
VkWriteDescriptorSet VKHelper::GetWriteDescriptorSet(const VkDescriptorImageInfo &imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.pImageInfo = &imageInfo;
	l_result.dstSet = descriptorSet;

	return l_result;
}
VkWriteDescriptorSet VKHelper::GetWriteDescriptorSet(const VkDescriptorBufferInfo &bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
{
	VkWriteDescriptorSet l_result = {};
	l_result.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	l_result.dstBinding = dstBinding;
	l_result.dstArrayElement = 0;
	l_result.descriptorType = descriptorType;
	l_result.descriptorCount = 1;
	l_result.pBufferInfo = &bufferInfo;
	l_result.dstSet = descriptorSet;

	return l_result;
}