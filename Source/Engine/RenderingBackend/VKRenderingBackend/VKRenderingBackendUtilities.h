#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/STL17.h"

#include "../../Component/MeshDataComponent.h"
#include "../../Component/VKMaterialDataComponent.h"
#include "../../Component/TextureDataComponent.h"
#include "../../Component/VKMeshDataComponent.h"
#include "../../Component/VKTextureDataComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"

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

INNO_PRIVATE_SCOPE VKRenderingBackendNS
{
	bool setup();
	bool initialize();
	bool update();
	bool render();
	bool terminate();
	bool initializeComponentPool();
	bool resize();

	void loadDefaultAssets();
	bool generateGPUBuffers();

	VKMeshDataComponent* addVKMeshDataComponent();
	VKTextureDataComponent* addVKTextureDataComponent();
	VKMaterialDataComponent* addVKMaterialDataComponent();

	VKMeshDataComponent* getVKMeshDataComponent(MeshShapeType MeshShapeType);
	VKTextureDataComponent* getVKTextureDataComponent(TextureUsageType TextureUsageType);
	VKTextureDataComponent* getVKTextureDataComponent(FileExplorerIconType iconType);
	VKTextureDataComponent* getVKTextureDataComponent(WorldEditorIconType iconType);
	VKMaterialDataComponent* getDefaultMaterialDataComponent();

	bool checkValidationLayerSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);

	bool createDescriptorPool(VkDescriptorPoolSize* poolSize, unsigned int poolSizeCount, unsigned int maxSets, VkDescriptorPool& poolHandle);
	bool createDescriptorSetLayout(VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout);
	bool createDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout& setLayout, VkDescriptorSet& setHandle, unsigned int count);
	bool updateDescriptorSet(VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount);

	VKRenderPassComponent* addVKRenderPassComponent();
	bool initializeVKRenderPassComponent(VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC);

	bool reserveRenderTargets(VKRenderPassComponent* VKRPC);
	bool createRenderTargets(VKRenderPassComponent* VKRPC);
	bool createSingleFramebuffer(VKRenderPassComponent* VKRPC);
	bool createMultipleFramebuffers(VKRenderPassComponent* VKRPC);
	bool createRenderPass(VKRenderPassComponent* VKRPC);

	bool createPipelineLayout(VKRenderPassComponent* VKRPC);
	bool createGraphicsPipelines(VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC);
	bool createCommandBuffers(VKRenderPassComponent* VKRPC);
	bool createSyncPrimitives(VKRenderPassComponent* VKRPC);

	bool destroyVKRenderPassComponent(VKRenderPassComponent* VKRPC);

	bool initializeVKMeshDataComponent(VKMeshDataComponent* rhs);
	bool initializeVKTextureDataComponent(VKTextureDataComponent* rhs);
	bool createImageView(VKTextureDataComponent* VKTDC);
	bool initializeVKMaterialDataComponent(VKMaterialDataComponent* rhs);

	bool destroyAllGraphicPrimitiveComponents();

	bool recordCommand(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex, const std::function<void()>& commands);

	bool recordDrawCall(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex, VKMeshDataComponent * VKMDC);

	bool waitForFence(VKRenderPassComponent* VKRPC);
	bool submitCommand(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex);

	bool updateUBOImpl(VkDeviceMemory&  UBOMemory, size_t size, const void* UBOValue);

	template<typename T>
	bool updateUBO(VkDeviceMemory&  UBOMemory, const T& UBOValue)
	{
		return updateUBOImpl(UBOMemory, sizeof(T), &UBOValue);
	}

	template<typename T>
	bool updateUBO(VkDeviceMemory&  UBOMemory, const std::vector<T>& UBOValue)
	{
		return updateUBOImpl(UBOMemory, sizeof(T) * UBOValue.size(), &UBOValue[0]);
	}

	VkTextureDataDesc getVKTextureDataDesc(TextureDataDesc textureDataDesc);

	VKShaderProgramComponent* addVKShaderProgramComponent(const EntityID& rhs);

	bool initializeVKShaderProgramComponent(VKShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);
	bool destroyVKShaderProgramComponent(VKShaderProgramComponent* VKSPC);

	bool generateUBO(VkBuffer& UBO, VkDeviceSize UBOSize, VkDeviceMemory& UBOMemory);
}
