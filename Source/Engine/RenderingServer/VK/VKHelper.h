#pragma once
#include "../../Common/STL17.h"
#include "../../Component/VKTextureDataComponent.h"
#include "../../Component/VKRenderPassDataComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../IRenderingServer.h"

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;

	bool isComplete()
	{
		return m_graphicsFamily.has_value() && m_presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

namespace VKHelper
{
	bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR windowSurface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR windowSurface);
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);

	VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer);

	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	bool createCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, VkDevice device, VkCommandPool& commandPool);

	bool createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	bool copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VKTextureDataDesc getVKTextureDataDesc(TextureDataDesc textureDataDesc);
	VkImageType getImageType(TextureSamplerType textureSamplerType);
	VkImageViewType getImageViewType(TextureSamplerType textureSamplerType);
	VkImageUsageFlags getImageUsageFlags(TextureUsageType textureUsageType);
	VkSamplerAddressMode getSamplerAddressMode(TextureWrapMethod textureWrapMethod);
	VkSamplerMipmapMode getTextureFilterParam(TextureFilterMethod textureFilterMethod);
	VkFormat getTextureFormat(TextureDataDesc textureDataDesc);
	VkDeviceSize getImageSize(TextureDataDesc textureDataDesc);
	VkImageAspectFlagBits getImageAspectFlags(TextureUsageType textureUsageType);
	VkImageCreateInfo getImageCreateInfo(TextureDataDesc textureDataDesc, VKTextureDataDesc vKTextureDataDesc);

	bool transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);
	bool copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);
	bool createImageView(VkDevice device, VKTextureDataComponent* VKTDC);

	bool createDescriptorSetLayoutBindings(VKRenderPassDataComponent* VKRPDC);
	bool createDescriptorPool(VkDevice device, VkDescriptorPoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool& poolHandle);
	bool createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout);
	bool createDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout& setLayout, VkDescriptorSet& setHandle, uint32_t count);
	bool updateDescriptorSet(VkDevice device, VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount);

	bool reserveRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer);
	bool createRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer);
	bool createRenderPass(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool createViewportAndScissor(VKRenderPassDataComponent* VKRPDC);
	bool createSingleFramebuffer(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool createMultipleFramebuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC);

	VkCompareOp GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
	VkStencilOp GetStencilOperationEnum(StencilOperation stencilOperation);
	VkBlendFactor GetBlendFactorEnum(BlendFactor blendFactor);
	VkBlendOp GetBlendOperation(BlendOperation blendOperation);

	bool createPipelineLayout(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject* PSO);
	bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject* PSO);
	bool GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject* PSO);
	bool GenerateBlendState(BlendDesc blendDesc, size_t RTCount, VKPipelineStateObject* PSO);
	bool createGraphicsPipelines(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool createComputePipelines(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool createCommandBuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC);
	bool createSyncPrimitives(VkDevice device, VKRenderPassDataComponent* VKRPDC);

	bool createShaderModule(VkDevice device, VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath);
}