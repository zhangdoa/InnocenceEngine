#include "VKGraphicsService.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

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
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::CreateHostStagingBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBufferCreateInfo l_stagingBufferCInfo = {};
	l_stagingBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_stagingBufferCInfo.size = bufferSize;
	l_stagingBufferCInfo.usage = usageFlags;
	l_stagingBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(l_stagingBufferCInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMemory);
}

bool VKGraphicsService::CreateDeviceLocalBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBufferCreateInfo l_localBufferCInfo = {};
	l_localBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_localBufferCInfo.size = bufferSize;
	l_localBufferCInfo.usage = usageFlags;
	l_localBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(l_localBufferCInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);
}

bool VKGraphicsService::CopyHostMemoryToDeviceMemory(void* hostMemory, size_t bufferSize, VkDeviceMemory& deviceMemory)
{
	void* l_mappedMemory;
	vkMapMemory(m_device, deviceMemory, 0, bufferSize, 0, &l_mappedMemory);
	std::memcpy(l_mappedMemory, hostMemory, (size_t)bufferSize);
	vkUnmapMemory(m_device, deviceMemory);

	return true;
}

bool VKGraphicsService::InitializeDeviceLocalBuffer(void* hostMemory, size_t bufferSize, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
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

VkCommandBuffer VKGraphicsService::OpenTemporaryCommandBuffer(VkCommandPool commandPool)
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

void VKGraphicsService::CloseTemporaryCommandBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer)
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

uint32_t VKGraphicsService::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

bool VKGraphicsService::CreateCommandPool(VkSurfaceKHR windowSurface, GPUEngineType GPUEngineType, VkCommandPool& commandPool)
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

bool VKGraphicsService::CreateBuffer(const VkBufferCreateInfo& bufferCInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
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

bool VKGraphicsService::CopyBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = OpenTemporaryCommandBuffer(commandPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	CloseTemporaryCommandBuffer(commandPool, commandQueue, commandBuffer);

	return true;
}

bool VKGraphicsService::CreateImage(const VkImageCreateInfo& imageCInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
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

bool VKGraphicsService::TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, ShaderStage shaderStage)
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

bool VKGraphicsService::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height)
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
