#pragma once
#include "../../Common/STL17.h"
#include "../../Common/LogService.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKRenderPassComponent.h"
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
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback);
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks *pAllocator);
		VkResult SetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo);

		template <typename U, typename T>
		bool SetObjectName(VkDevice device, U *owner, const T &rhs, VkObjectType objectType, const char *objectTypeSuffix)
		{
			auto l_Name = std::string(owner->m_InstanceName.c_str());
			l_Name += "_";
			l_Name += objectTypeSuffix;

			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.pNext = nullptr;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = (uint64_t)rhs;
			nameInfo.pObjectName = l_Name.c_str();

			auto l_result = SetDebugUtilsObjectNameEXT(device, &nameInfo);
			if (l_result != VK_SUCCESS)
			{
				Log(Warning, "Can't name ", objectType, " with ", l_Name.c_str());
				return false;
			}
			return true;
		}

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

		bool CreateCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, VkDevice device, GPUEngineType GPUEngineType, VkCommandPool &commandPool);

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
		
		VkImageLayout GetTextureWriteImageLayout(TextureDesc textureDesc);
		VkImageLayout GetTextureReadImageLayout(TextureDesc textureDesc);
		VkAccessFlagBits GetAccessMask(const VkImageLayout& imageLayout);
		VkPipelineStageFlags GetPipelineStageFlags(const VkImageLayout& imageLayout, ShaderStage shaderStage);
		bool TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, ShaderStage shaderStage = ShaderStage::Invalid);
		bool CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);
		bool CreateImageView(VkDevice device, VKTextureComponent* VKTextureComp);

		bool CreateDescriptorSetLayoutBindings(VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorPool(VkDevice device, VkDescriptorPoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool& poolHandle);
		bool CreateDescriptorPool(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout);
		bool CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayout& dummyEmptyDescriptorLayout, VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorSets(VkDevice device, VkDescriptorPool pool, const VkDescriptorSetLayout* setLayout, VkDescriptorSet& setHandle, uint32_t count);
		bool CreateDescriptorSets(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool UpdateDescriptorSet(VkDevice device, VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount);
		bool ReserveFramebuffer(VKRenderPassComponent* VKRenderPassComp);
		bool CreateRenderPass(VkDevice device, VKRenderPassComponent* VKRenderPassComp, VkFormat* overrideFormat = nullptr);
		bool CreateViewportAndScissor(VKRenderPassComponent* VKRenderPassComp);
		bool CreateSingleFramebuffer(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool CreateMultipleFramebuffers(VkDevice device, VKRenderPassComponent* VKRenderPassComp);

		VkCompareOp GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
		VkStencilOp GetStencilOperationEnum(StencilOperation stencilOperation);
		VkBlendFactor GetBlendFactorEnum(BlendFactor blendFactor);
		VkBlendOp GetBlendOperation(BlendOperation blendOperation);

		bool CreatePipelineLayout(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject* PSO);
		bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject* PSO);
		bool GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject* PSO);
		bool GenerateBlendState(BlendDesc blendDesc, size_t RTCount, VKPipelineStateObject* PSO);
		bool CreateGraphicsPipelines(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool CreateComputePipelines(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool CreateCommandBuffers(VkDevice device, VKRenderPassComponent* VKRenderPassComp);
		bool CreateSyncPrimitives(VkDevice device, VKRenderPassComponent* VKRenderPassComp);

		bool CreateShaderModule(VkDevice device, VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath);

		VkWriteDescriptorSet GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorImageInfo& imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorBufferInfo& bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
	}
}