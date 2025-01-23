#include "VKRenderingServer.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/EntityManager.h"

namespace Inno
{
	namespace VKHelper
	{
		const char *m_shaderRelativePath = "..//Res//Shaders//SPIRV//";
	}
} // namespace Inno

// @TODO: Maybe store the function pointers rather than querying them every time.
VkResult VKRenderingServer::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
	auto l_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
	if (l_func != nullptr)
	{
		return l_func(m_instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKRenderingServer::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto l_func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (l_func != nullptr)
	{
		l_func(m_instance, callback, pAllocator);
	}
}

VkResult VKRenderingServer::SetDebugUtilsObjectNameEXT(const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	auto l_func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkSetDebugUtilsObjectNameEXT");
	if (l_func != nullptr)
	{
		return l_func(m_device, pNameInfo);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

bool VKRenderingServer::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
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

bool VKRenderingServer::CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VKRenderingServer::FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.m_computeFamily = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			indices.m_transferFamily = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		VkBool32 l_presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &l_presentSupport);

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

VkSurfaceFormatKHR VKRenderingServer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR VKRenderingServer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
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

VkExtent2D VKRenderingServer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

		VkExtent2D l_actualExtent;
		l_actualExtent.width = l_screenResolution.x;
		l_actualExtent.height = l_screenResolution.y;

		l_actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, l_actualExtent.width));
		l_actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, l_actualExtent.height));

		return l_actualExtent;
	}
}

SwapChainSupportDetails VKRenderingServer::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	SwapChainSupportDetails l_details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &l_details.m_capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		l_details.m_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, l_details.m_formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		l_details.m_presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, l_details.m_presentModes.data());
	}

	return l_details;
}

bool VKRenderingServer::IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, windowSurface);

	bool extensionsSupported = CheckDeviceExtensionSupport(deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, windowSurface);
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VKRenderingServer::CreateHostStagingBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBufferCreateInfo l_stagingBufferCInfo = {};
	l_stagingBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_stagingBufferCInfo.size = bufferSize;
	l_stagingBufferCInfo.usage = usageFlags;
	l_stagingBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(l_stagingBufferCInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMemory);
}

bool VKRenderingServer::CreateDeviceLocalBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBufferCreateInfo l_localBufferCInfo = {};
	l_localBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_localBufferCInfo.size = bufferSize;
	l_localBufferCInfo.usage = usageFlags;
	l_localBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(l_localBufferCInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);
}

bool VKRenderingServer::CopyHostMemoryToDeviceMemory(void* hostMemory, size_t bufferSize, VkDeviceMemory& deviceMemory)
{
	void* l_mappedMemory;
	vkMapMemory(m_device, deviceMemory, 0, bufferSize, 0, &l_mappedMemory);
	std::memcpy(l_mappedMemory, hostMemory, (size_t)bufferSize);
	vkUnmapMemory(m_device, deviceMemory);

	return true;
}

bool VKRenderingServer::InitializeDeviceLocalBuffer(void* hostMemory, size_t bufferSize, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;

	CreateHostStagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

	CopyHostMemoryToDeviceMemory(hostMemory, bufferSize, l_stagingBufferMemory);

	CopyBuffer(m_globalCommandPool, m_graphicsQueue, l_stagingBuffer, buffer, bufferSize);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	return true;
}

VkCommandBuffer VKRenderingServer::OpenTemporaryCommandBuffer(VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VKRenderingServer::CloseTemporaryCommandBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(commandQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(commandQueue);

	vkFreeCommandBuffers(m_device, commandPool, 1, &commandBuffer);
}

uint32_t VKRenderingServer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	Log(Error, "Failed to find suitable memory type!!");
	return 0;
}

bool VKRenderingServer::CreateCommandPool(VkSurfaceKHR windowSurface, GPUEngineType GPUEngineType, VkCommandPool& commandPool)
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice, windowSurface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	if (GPUEngineType == GPUEngineType::Graphics)
		poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();
	else if (GPUEngineType == GPUEngineType::Compute)
		poolInfo.queueFamilyIndex = queueFamilyIndices.m_computeFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		Log(Error, "Failed to create CommandPool!");
		return false;
	}

	Log(Verbose, "CommandPool has been created.");
	return true;
}

bool VKRenderingServer::CreateBuffer(const VkBufferCreateInfo& bufferCInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	if (vkCreateBuffer(m_device, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkBuffer!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		Log(Error, "Failed to allocate VkDeviceMemory for VkBuffer!");
		return false;
	}

	vkBindBufferMemory(m_device, buffer, bufferMemory, 0);

	return true;
}

bool VKRenderingServer::CopyBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = OpenTemporaryCommandBuffer(commandPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	CloseTemporaryCommandBuffer(commandPool, commandQueue, commandBuffer);

	return true;
}

bool VKRenderingServer::CreateImage(const VkImageCreateInfo& imageCInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	if (vkCreateImage(m_device, &imageCInfo, nullptr, &image) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkImage!");
		return false;
	}

	VkMemoryRequirements l_memRequirements;
	vkGetImageMemoryRequirements(m_device, image, &l_memRequirements);

	VkMemoryAllocateInfo l_allocInfo = {};
	l_allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	l_allocInfo.allocationSize = l_memRequirements.size;
	l_allocInfo.memoryTypeIndex = FindMemoryType(l_memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(m_device, &l_allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		Log(Error, "Failed to allocate VkDeviceMemory for VkImage!");
		return false;
	}

	vkBindImageMemory(m_device, image, imageMemory, 0);

	return true;
}

bool VKRenderingServer::TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, ShaderStage shaderStage)
{
	VkImageMemoryBarrier l_barrier = {};
	l_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	l_barrier.oldLayout = oldLayout;
	l_barrier.newLayout = newLayout;
	l_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	l_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	l_barrier.image = image;
	l_barrier.srcAccessMask = GetAccessMask(oldLayout);
	l_barrier.dstAccessMask = GetAccessMask(newLayout);
	l_barrier.subresourceRange.aspectMask = aspectFlags;
	l_barrier.subresourceRange.baseMipLevel = 0;
	l_barrier.subresourceRange.levelCount = 1;
	l_barrier.subresourceRange.baseArrayLayer = 0;
	l_barrier.subresourceRange.layerCount = 1;

	auto l_sourceStage = GetPipelineStageFlags(oldLayout, shaderStage);
	auto l_destinationStage = GetPipelineStageFlags(newLayout, shaderStage);

	vkCmdPipelineBarrier(
		commandBuffer,
		l_sourceStage, l_destinationStage,
		VK_DEPENDENCY_BY_REGION_BIT,
		0, nullptr,
		0, nullptr,
		1, &l_barrier);

	return true;
}

bool VKRenderingServer::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height)
{
	VkBufferImageCopy l_region = {};
	l_region.bufferOffset = 0;
	l_region.bufferRowLength = 0;
	l_region.bufferImageHeight = 0;
	l_region.imageSubresource.aspectMask = aspectFlags;
	l_region.imageSubresource.mipLevel = 0;
	l_region.imageSubresource.baseArrayLayer = 0;
	l_region.imageSubresource.layerCount = 1;
	l_region.imageOffset = { 0, 0, 0 };
	l_region.imageExtent = {
		width,
		height,
		1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_region);

	return true;
}

bool VKRenderingServer::CreateDescriptorPool(VkDescriptorPoolSize *poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool &poolHandle)
{
	VkDescriptorPoolCreateInfo l_poolCInfo = {};
	l_poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	l_poolCInfo.poolSizeCount = poolSizeCount;
	l_poolCInfo.pPoolSizes = poolSize;
	l_poolCInfo.maxSets = maxSets;
	l_poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(m_device, &l_poolCInfo, nullptr, &poolHandle) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkDescriptorPool!");
		return false;
	}

	Log(Verbose, "VkDescriptorPool has been created.");
	return true;
}

bool VKRenderingServer::CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout &setLayout)
{
	VkDescriptorSetLayoutCreateInfo l_layoutCInfo = {};
	l_layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	l_layoutCInfo.bindingCount = setLayoutBindingsCount;
	l_layoutCInfo.pBindings = setLayoutBindings;
	l_layoutCInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	if (vkCreateDescriptorSetLayout(m_device, &l_layoutCInfo, nullptr, &setLayout) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkDescriptorSetLayout!");
		return false;
	}

	Log(Verbose, "VkDescriptorSetLayout has been created.");
	return true;
}

bool VKRenderingServer::CreateDescriptorSets(VkDescriptorPool pool, const VkDescriptorSetLayout *setLayout, VkDescriptorSet &setHandle, uint32_t count)
{
	VkDescriptorSetAllocateInfo l_allocCInfo = {};
	l_allocCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	l_allocCInfo.descriptorPool = pool;
	l_allocCInfo.descriptorSetCount = count;
	l_allocCInfo.pSetLayouts = setLayout;

	if (vkAllocateDescriptorSets(m_device, &l_allocCInfo, &setHandle) != VK_SUCCESS)
	{
		Log(Error, "Failed to allocate VkDescriptorSet!");
		return false;
	}

	Log(Verbose, "VkDescriptorSet has been allocated.");
	return true;
}

bool VKRenderingServer::UpdateDescriptorSet(VkWriteDescriptorSet *writeDescriptorSets, uint32_t writeDescriptorSetsCount)
{
	vkUpdateDescriptorSets(
		m_device,
		writeDescriptorSetsCount,
		writeDescriptorSets,
		0,
		nullptr);

	Log(Verbose, "Write VkDescriptorSet has been updated.");
	return true;
}

VkWriteDescriptorSet VKRenderingServer::GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
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

VkWriteDescriptorSet VKRenderingServer::GetWriteDescriptorSet(const VkDescriptorImageInfo &imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
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

VkWriteDescriptorSet VKRenderingServer::GetWriteDescriptorSet(const VkDescriptorBufferInfo &bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet &descriptorSet)
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

bool VKRenderingServer::CreateShaderModule(VkShaderModule &vkShaderModule, const ShaderFilePath &shaderFilePath)
{
	auto l_shaderFileName = m_shaderRelativePath + std::string(shaderFilePath.c_str()) + ".spv";
	auto l_shaderContent = g_Engine->Get<IOService>()->loadFile(l_shaderFileName.c_str(), IOMode::Binary);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_shaderContent.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t *>(l_shaderContent.data());

	if (vkCreateShaderModule(m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkShaderModule for: ", shaderFilePath.c_str(), "!");
		return false;
	}

	Log(Verbose, "", shaderFilePath.c_str(), " has been loaded.");
	return true;
}