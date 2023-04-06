#pragma once
#include "../../Common/STL17.h"
#include "../../Component/VKTextureDataComponent.h"
#include "../../Component/VKRenderPassDataComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../IRenderingServer.h"

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
		std::vector<VkSurfaceFormatKHR> m_formats;
		std::vector<VkPresentModeKHR> m_presentModes;
	};

	namespace VKHelper
	{
		bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR windowSurface);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR windowSurface);
		bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);

		VkCommandBuffer OpenTemporaryCommandBuffer(VkDevice device, VkCommandPool commandPool);
		void CloseTemporaryCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer);

		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

		bool CreateCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, VkDevice device, VkCommandPool& commandPool);

		bool CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkBufferCreateInfo& bufferCInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		bool CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		bool CreateImage(VkPhysicalDevice physicalDevice, VkDevice device, const VkImageCreateInfo& imageCInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

		VKTextureDesc GetVKTextureDesc(TextureDesc textureDesc);
		VkImageType GetImageType(TextureSampler textureSampler);
		VkImageViewType GetImageViewType(TextureSampler textureSampler);
		VkImageUsageFlags GetImageUsageFlags(TextureUsage textureUsage);
		VkSamplerAddressMode GetSamplerAddressMode(TextureWrapMethod textureWrapMethod);
		VkFilter GetFilter(TextureFilterMethod textureFilterMethod);
		VkSamplerMipmapMode GetSamplerMipmapMode(TextureFilterMethod minFilterMethod);
		VkFormat GetTextureFormat(TextureDesc textureDesc);
		VkDeviceSize GetImageSize(TextureDesc textureDesc);
		VkImageAspectFlagBits GetImageAspectFlags(TextureUsage textureUsage);
		VkImageCreateInfo GetImageCreateInfo(TextureDesc textureDesc, VKTextureDesc vKTextureDesc);

		bool TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);
		bool CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);
		bool CreateImageView(VkDevice device, VKTextureDataComponent* VKTDC);

		bool CreateDescriptorSetLayoutBindings(VKRenderPassDataComponent* VKRPDC);
		bool CreateDescriptorPool(VkDevice device, VkDescriptorPoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool& poolHandle);
		bool CreateDescriptorPool(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout);
		bool CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayout& dummyEmptyDescriptorLayout, VKRenderPassDataComponent* VKRPDC);
		bool CreateDescriptorSets(VkDevice device, VkDescriptorPool pool, const VkDescriptorSetLayout* setLayout, VkDescriptorSet& setHandle, uint32_t count);
		bool CreateDescriptorSets(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool UpdateDescriptorSet(VkDevice device, VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount);
		bool ReserveRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer);
		bool CreateRenderTargets(VKRenderPassDataComponent* VKRPDC, IRenderingServer* renderingServer);
		bool CreateRenderPass(VkDevice device, VKRenderPassDataComponent* VKRPDC, VkFormat* overrideFormat = nullptr);
		bool CreateViewportAndScissor(VKRenderPassDataComponent* VKRPDC);
		bool CreateSingleFramebuffer(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool CreateMultipleFramebuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC);

		VkCompareOp GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
		VkStencilOp GetStencilOperationEnum(StencilOperation stencilOperation);
		VkBlendFactor GetBlendFactorEnum(BlendFactor blendFactor);
		VkBlendOp GetBlendOperation(BlendOperation blendOperation);

		bool CreatePipelineLayout(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject* PSO);
		bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject* PSO);
		bool GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject* PSO);
		bool GenerateBlendState(BlendDesc blendDesc, size_t RTCount, VKPipelineStateObject* PSO);
		bool CreateGraphicsPipelines(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool CreateComputePipelines(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool CreateCommandBuffers(VkDevice device, VKRenderPassDataComponent* VKRPDC);
		bool CreateSyncPrimitives(VkDevice device, VKRenderPassDataComponent* VKRPDC);

		bool CreateShaderModule(VkDevice device, VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath);

		VkWriteDescriptorSet GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorImageInfo& imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorBufferInfo& bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
	}
}