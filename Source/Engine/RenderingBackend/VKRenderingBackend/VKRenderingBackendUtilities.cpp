#include "VKRenderingBackendUtilities.h"
#include "../../Component/VKRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE VKRenderingBackendNS
{
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	bool copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	bool createShaderModule(VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath);

	bool submitGPUData(VKMeshDataComponent * rhs);

	bool createVBO(const InnoArray<Vertex>& vertices, VkBuffer& VBO);
	bool createIBO(const InnoArray<Index>& indices, VkBuffer& IBO);

	bool submitGPUData(VKTextureDataComponent * rhs);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);

	VkImageType getImageType(TextureSamplerType rhs);
	VkSamplerAddressMode getSamplerAddressMode(TextureWrapMethod rhs);
	VkSamplerMipmapMode getTextureFilterParam(TextureFilterMethod rhs);
	VkFormat getTextureFormat(TextureDataDesc textureDataDesc);
	VkImageAspectFlagBits getImageAspectFlags(TextureDataDesc textureDataDesc);

	VkVertexInputBindingDescription m_vertexBindingDescription;
	std::array<VkVertexInputAttributeDescription, 5> m_vertexAttributeDescriptions;

	std::unordered_map<InnoEntity*, VKMeshDataComponent*> m_initializedVKMDC;
	std::unordered_map<InnoEntity*, VKTextureDataComponent*> m_initializedVKTDC;

	void* m_VKRenderPassComponentPool;
	void* m_VKShaderProgramComponentPool;

	const std::string m_shaderRelativePath = std::string{ "Res//Shaders//" };
}

bool VKRenderingBackendNS::initializeComponentPool()
{
	m_VKRenderPassComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(VKRenderPassComponent), 32);
	m_VKShaderProgramComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(VKShaderProgramComponent), 128);

	m_vertexBindingDescription = {};
	m_vertexBindingDescription.binding = 0;
	m_vertexBindingDescription.stride = sizeof(Vertex);
	m_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttributeDescriptions = {};

	m_vertexAttributeDescriptions[0].binding = 0;
	m_vertexAttributeDescriptions[0].location = 0;
	m_vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[0].offset = offsetof(Vertex, m_pos);

	m_vertexAttributeDescriptions[1].binding = 0;
	m_vertexAttributeDescriptions[1].location = 1;
	m_vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[1].offset = offsetof(Vertex, m_texCoord);

	m_vertexAttributeDescriptions[2].binding = 0;
	m_vertexAttributeDescriptions[2].location = 2;
	m_vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[2].offset = offsetof(Vertex, m_pad1);

	m_vertexAttributeDescriptions[3].binding = 0;
	m_vertexAttributeDescriptions[3].location = 3;
	m_vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[3].offset = offsetof(Vertex, m_normal);

	m_vertexAttributeDescriptions[4].binding = 0;
	m_vertexAttributeDescriptions[4].location = 4;
	m_vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[4].offset = offsetof(Vertex, m_pad2);

	return true;
}

bool VKRenderingBackendNS::checkValidationLayerSupport()
{
	uint32_t l_layerCount;
	vkEnumerateInstanceLayerProperties(&l_layerCount, nullptr);

	std::vector<VkLayerProperties> l_availableLayers(l_layerCount);
	vkEnumerateInstanceLayerProperties(&l_layerCount, l_availableLayers.data());

	for (const char* layerName : VKRenderingBackendComponent::get().m_validationLayers)
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

bool VKRenderingBackendNS::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(VKRenderingBackendComponent::get().m_deviceExtensions.begin(), VKRenderingBackendComponent::get().m_deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VKRenderingBackendNS::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		VkBool32 l_presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VKRenderingBackendComponent::get().m_windowSurface, &l_presentSupport);

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

VkSurfaceFormatKHR VKRenderingBackendNS::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR VKRenderingBackendNS::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
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

VkExtent2D VKRenderingBackendNS::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

SwapChainSupportDetails VKRenderingBackendNS::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails l_details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VKRenderingBackendComponent::get().m_windowSurface, &l_details.m_capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKRenderingBackendComponent::get().m_windowSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		l_details.m_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKRenderingBackendComponent::get().m_windowSurface, &formatCount, l_details.m_formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKRenderingBackendComponent::get().m_windowSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		l_details.m_presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKRenderingBackendComponent::get().m_windowSurface, &presentModeCount, l_details.m_presentModes.data());
	}

	return l_details;
}

bool VKRenderingBackendNS::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

VKRenderPassComponent* VKRenderingBackendNS::addVKRenderPassComponent()
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_VKRenderPassComponentPool, sizeof(VKRenderPassComponent));
	auto l_VKRPC = new(l_rawPtr)VKRenderPassComponent();

	return l_VKRPC;
}

bool VKRenderingBackendNS::initializeVKRenderPassComponent(VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC)
{
	bool l_result = true;

	l_result &= reserveRenderTargets(VKRPC);

	l_result &= createRenderTargets(VKRPC);

	l_result &= createRenderPass(VKRPC);

	if (VKRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		l_result &= createMultipleFramebuffers(VKRPC);
	}
	else
	{
		l_result &= createSingleFramebuffer(VKRPC);
	}

	l_result &= createDescriptorSetLayout(VKRPC);

	l_result &= createPipelineLayout(VKRPC);

	l_result &= createGraphicsPipelines(VKRPC, VKSPC);

	l_result &= createCommandBuffers(VKRPC);

	l_result &= createSyncPrimitives(VKRPC);

	return l_result;
}

bool VKRenderingBackendNS::reserveRenderTargets(VKRenderPassComponent* VKRPC)
{
	size_t l_framebufferNumber = 0;
	if (VKRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		l_framebufferNumber = VKRPC->m_renderPassDesc.RTNumber;
	}
	else
	{
		l_framebufferNumber = 1;
	}

	// reserve vectors and emplace empty objects
	VKRPC->m_framebuffers.reserve(l_framebufferNumber);
	for (size_t i = 0; i < l_framebufferNumber; i++)
	{
		VKRPC->m_framebuffers.emplace_back();
	}

	VKRPC->m_VKTDCs.reserve(VKRPC->m_renderPassDesc.RTNumber);
	for (size_t i = 0; i < VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		VKRPC->m_VKTDCs.emplace_back();
	}

	for (size_t i = 0; i < VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		VKRPC->m_VKTDCs[i] = addVKTextureDataComponent();
	}

	if (VKRPC->m_renderPassDesc.useDepthAttachment)
	{
		VKRPC->m_depthVKTDC = addVKTextureDataComponent();
	}

	return true;
}

bool VKRenderingBackendNS::createRenderTargets(VKRenderPassComponent* VKRPC)
{
	for (size_t i = 0; i < VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		auto l_TDC = VKRPC->m_VKTDCs[i];

		l_TDC->m_textureDataDesc = VKRPC->m_renderPassDesc.RTDesc;

		l_TDC->m_textureData = nullptr;

		initializeVKTextureDataComponent(l_TDC);
	}

	if (VKRPC->m_renderPassDesc.useDepthAttachment)
	{
		VKRPC->m_depthVKTDC->m_textureDataDesc = VKRPC->m_renderPassDesc.RTDesc;
		VKRPC->m_depthVKTDC->m_textureDataDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
		VKRPC->m_depthVKTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
		VKRPC->m_depthVKTDC->m_textureData = { nullptr };

		initializeVKTextureDataComponent(VKRPC->m_depthVKTDC);
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Render targets have been created.");

	return true;
}

bool VKRenderingBackendNS::createRenderPass(VKRenderPassComponent* VKRPC)
{
	VKRPC->renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VKRPC->subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (VKRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		VKRPC->subpassDesc.colorAttachmentCount = 1;
		VKRPC->renderPassCInfo.attachmentCount = 1;
	}
	else
	{
		VKRPC->subpassDesc.colorAttachmentCount = VKRPC->m_renderPassDesc.RTNumber;
		VKRPC->renderPassCInfo.attachmentCount = (unsigned int)VKRPC->attachmentDescs.size();
	}

	VKRPC->subpassDesc.pColorAttachments = &VKRPC->colorAttachmentRefs[0];
	if (VKRPC->depthAttachmentRef.attachment)
	{
		VKRPC->subpassDesc.pDepthStencilAttachment = &VKRPC->depthAttachmentRef;
	}

	VKRPC->renderPassCInfo.pSubpasses = &VKRPC->subpassDesc;

	VKRPC->renderPassCInfo.pAttachments = &VKRPC->attachmentDescs[0];

	if (vkCreateRenderPass(VKRenderingBackendComponent::get().m_device, &VKRPC->renderPassCInfo, nullptr, &VKRPC->m_renderPass) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkRenderPass!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkRenderPass has been created.");
	return true;
}

bool VKRenderingBackendNS::createSingleFramebuffer(VKRenderPassComponent* VKRPC)
{
	// create frame buffer and attach image view

	std::vector<VkImageView> attachments(VKRPC->attachmentDescs.size());

	for (size_t i = 0; i < VKRPC->m_VKTDCs.size(); i++)
	{
		attachments[i] = VKRPC->m_VKTDCs[i]->m_imageView;
	}

	if (VKRPC->m_renderPassDesc.useDepthAttachment)
	{
		attachments[VKRPC->attachmentDescs.size() - 1] = VKRPC->m_depthVKTDC->m_imageView;
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = VKRPC->m_renderPass;
	framebufferInfo.attachmentCount = (uint32_t)attachments.size();
	framebufferInfo.pAttachments = &attachments[0];
	framebufferInfo.width = (uint32_t)VKRPC->viewport.width;
	framebufferInfo.height = (uint32_t)VKRPC->viewport.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(VKRenderingBackendComponent::get().m_device, &framebufferInfo, nullptr, &VKRPC->m_framebuffers[0]) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkFramebuffer!");
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Single VkFramebuffer has been created.");
	return true;
}

bool VKRenderingBackendNS::createMultipleFramebuffers(VKRenderPassComponent* VKRPC)
{
	for (size_t i = 0; i < VKRPC->m_VKTDCs.size(); i++)
	{
		// create frame buffer and attach image view
		VkImageView attachments[] = { VKRPC->m_VKTDCs[i]->m_imageView };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = VKRPC->m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = (uint32_t)VKRPC->viewport.width;
		framebufferInfo.height = (uint32_t)VKRPC->viewport.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VKRenderingBackendComponent::get().m_device, &framebufferInfo, nullptr, &VKRPC->m_framebuffers[i]) != VK_SUCCESS)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkFramebuffer!");
		}
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Multiple VkFramebuffers have been created.");
	return true;
}

bool VKRenderingBackendNS::createDescriptorPool(VkDescriptorPoolSize* poolSize, unsigned int poolSizeCount, unsigned int maxSets, VkDescriptorPool& poolHandle)
{
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizeCount;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(VKRenderingBackendComponent::get().m_device, &poolInfo, nullptr, &poolHandle) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDescriptorPool!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorPool has been created.");
	return true;
}

bool VKRenderingBackendNS::createDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout& setLayout, VkDescriptorSet& setHandle, unsigned int count)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = &setLayout;

	if (vkAllocateDescriptorSets(VKRenderingBackendComponent::get().m_device, &allocInfo, &setHandle) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to allocate VkDescriptorSet!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorSet has been allocated.");
	return true;
}

bool VKRenderingBackendNS::createDescriptorSetLayout(VKRenderPassComponent* VKRPC)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(VKRPC->descriptorSetLayoutBindings.size());
	layoutInfo.pBindings = VKRPC->descriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(VKRenderingBackendComponent::get().m_device, &layoutInfo, nullptr, &VKRPC->descriptorSetLayout) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDescriptorSetLayout!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorSetLayout has been created.");
	return true;
}

bool VKRenderingBackendNS::updateDescriptorSet(VKRenderPassComponent* VKRPC)
{
	vkUpdateDescriptorSets(
		VKRenderingBackendComponent::get().m_device,
		static_cast<uint32_t>(VKRPC->writeDescriptorSets.size()),
		VKRPC->writeDescriptorSets.data(),
		0,
		nullptr);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Write VkDescriptorSet has been updated.");
	return true;
}

bool VKRenderingBackendNS::createPipelineLayout(VKRenderPassComponent* VKRPC)
{
	VKRPC->inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	VKRPC->pipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VKRPC->pipelineLayoutCInfo.setLayoutCount = 1;
	VKRPC->pipelineLayoutCInfo.pSetLayouts = &VKRPC->descriptorSetLayout;

	if (VKRPC->pushConstantRanges.size() > 0)
	{
		VKRPC->pipelineLayoutCInfo.pushConstantRangeCount = (uint32_t)VKRPC->pushConstantRanges.size();
		VKRPC->pipelineLayoutCInfo.pPushConstantRanges = &VKRPC->pushConstantRanges[0];
	}

	if (vkCreatePipelineLayout(VKRenderingBackendComponent::get().m_device, &VKRPC->pipelineLayoutCInfo, nullptr, &VKRPC->m_pipelineLayout) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkPipelineLayout!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkPipelineLayout has been created.");
	return true;
}

bool VKRenderingBackendNS::createGraphicsPipelines(VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC)
{
	VKRPC->viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	VKRPC->viewportStateCInfo.pViewports = &VKRPC->viewport;
	VKRPC->viewportStateCInfo.pScissors = &VKRPC->scissor;

	VKRPC->rasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	VKRPC->multisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	VKRPC->colorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VKRPC->colorBlendStateCInfo.attachmentCount = (uint32_t)VKRPC->colorBlendAttachmentStates.size();
	VKRPC->colorBlendStateCInfo.pAttachments = &VKRPC->colorBlendAttachmentStates[0];

	// attach shader module and create pipeline
	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStages = { VKSPC->m_vertexShaderStageCInfo, VKSPC->m_fragmentShaderStageCInfo };

	VKRPC->pipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	VKRPC->pipelineCInfo.stageCount = (uint32_t)l_shaderStages.size();
	VKRPC->pipelineCInfo.pStages = &l_shaderStages[0];
	VKRPC->pipelineCInfo.pVertexInputState = &VKSPC->m_vertexInputStateCInfo;
	VKRPC->pipelineCInfo.pInputAssemblyState = &VKRPC->inputAssemblyStateCInfo;
	VKRPC->pipelineCInfo.pViewportState = &VKRPC->viewportStateCInfo;
	VKRPC->pipelineCInfo.pRasterizationState = &VKRPC->rasterizationStateCInfo;
	VKRPC->pipelineCInfo.pMultisampleState = &VKRPC->multisampleStateCInfo;
	VKRPC->pipelineCInfo.pColorBlendState = &VKRPC->colorBlendStateCInfo;
	VKRPC->pipelineCInfo.layout = VKRPC->m_pipelineLayout;
	VKRPC->pipelineCInfo.renderPass = VKRPC->m_renderPass;
	VKRPC->pipelineCInfo.subpass = 0;
	VKRPC->pipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(VKRenderingBackendComponent::get().m_device, VK_NULL_HANDLE, 1, &VKRPC->pipelineCInfo, nullptr, &VKRPC->m_pipeline) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to to create VkPipeline!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkPipeline has been created.");
	return true;
}

bool VKRenderingBackendNS::createCommandBuffers(VKRenderPassComponent* VKRPC)
{
	VKRPC->m_commandBuffers.resize(VKRPC->m_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = VKRenderingBackendComponent::get().m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)VKRPC->m_commandBuffers.size();

	if (vkAllocateCommandBuffers(VKRenderingBackendComponent::get().m_device, &allocInfo, VKRPC->m_commandBuffers.data()) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to allocate VkCommandBuffer!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkCommandBuffer has been created.");
	return true;
}

bool VKRenderingBackendNS::createSyncPrimitives(VKRenderPassComponent* VKRPC)
{
	VKRPC->m_renderFinishedSemaphores.resize(VKRPC->m_maxFramesInFlight);
	VKRPC->m_inFlightFences.resize(VKRPC->m_maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VKRPC->submitInfo = submitInfo;

	for (size_t i = 0; i < VKRPC->m_maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(VKRenderingBackendComponent::get().m_device, &semaphoreInfo, nullptr, &VKRPC->m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(VKRenderingBackendComponent::get().m_device, &fenceInfo, nullptr, &VKRPC->m_inFlightFences[i]) != VK_SUCCESS)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create synchronization primitives!");
			return false;
		}
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Synchronization primitives has been created.");

	return true;
}

bool VKRenderingBackendNS::destroyVKRenderPassComponent(VKRenderPassComponent* VKRPC)
{
	for (size_t i = 0; i < VKRPC->m_maxFramesInFlight; i++)
	{
		vkDestroySemaphore(VKRenderingBackendComponent::get().m_device, VKRPC->m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(VKRenderingBackendComponent::get().m_device, VKRPC->m_inFlightFences[i], nullptr);
	}

	vkDestroyDescriptorPool(VKRenderingBackendComponent::get().m_device, VKRPC->m_descriptorPool, nullptr);

	vkFreeCommandBuffers(VKRenderingBackendComponent::get().m_device,
		VKRenderingBackendComponent::get().m_commandPool,
		static_cast<uint32_t>(VKRPC->m_commandBuffers.size()), VKRPC->m_commandBuffers.data());

	vkDestroyPipeline(VKRenderingBackendComponent::get().m_device, VKRPC->m_pipeline, nullptr);
	vkDestroyPipelineLayout(VKRenderingBackendComponent::get().m_device, VKRPC->m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(VKRenderingBackendComponent::get().m_device, VKRPC->descriptorSetLayout, nullptr);

	for (auto framebuffer : VKRPC->m_framebuffers)
	{
		vkDestroyFramebuffer(VKRenderingBackendComponent::get().m_device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(VKRenderingBackendComponent::get().m_device, VKRPC->m_renderPass, nullptr);

	return true;
}

VkCommandBuffer VKRenderingBackendNS::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = VKRenderingBackendComponent::get().m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VKRenderingBackendComponent::get().m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VKRenderingBackendNS::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VKRenderingBackendComponent::get().m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VKRenderingBackendComponent::get().m_graphicsQueue);

	vkFreeCommandBuffers(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_commandPool, 1, &commandBuffer);
}

uint32_t VKRenderingBackendNS::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(VKRenderingBackendComponent::get().m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to find suitable memory type!!");
	return 0;
}

bool VKRenderingBackendNS::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(VKRenderingBackendComponent::get().m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkBuffer!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VKRenderingBackendComponent::get().m_device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(VKRenderingBackendComponent::get().m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to allocate VkDeviceMemory for VkBuffer!");
		return false;
	}

	vkBindBufferMemory(VKRenderingBackendComponent::get().m_device, buffer, bufferMemory, 0);

	return true;
}

bool VKRenderingBackendNS::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);

	return true;
}

bool VKRenderingBackendNS::initializeVKMeshDataComponent(VKMeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		submitGPUData(rhs);

		return true;
	}
}

bool VKRenderingBackendNS::submitGPUData(VKMeshDataComponent * rhs)
{
	createVBO(rhs->m_vertices, rhs->m_VBO);
	createIBO(rhs->m_indices, rhs->m_IBO);

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedVKMDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

bool VKRenderingBackendNS::createVBO(const InnoArray<Vertex>& vertices, VkBuffer& VBO)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, &vertices[0], (size_t)bufferSize);
	vkUnmapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VBO, VKRenderingBackendComponent::get().m_vertexBufferMemory);

	copyBuffer(stagingBuffer, VBO, bufferSize);

	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, stagingBuffer, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, nullptr);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingBackend: VBO " + InnoUtility::pointerToString(VBO) + " is initialized.");

	return true;
}

bool  VKRenderingBackendNS::createIBO(const InnoArray<Index>& indices, VkBuffer& IBO)
{
	VkDeviceSize bufferSize = sizeof(Index) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, &indices[0], (size_t)bufferSize);
	vkUnmapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IBO, VKRenderingBackendComponent::get().m_indexBufferMemory);

	copyBuffer(stagingBuffer, IBO, bufferSize);

	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, stagingBuffer, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, nullptr);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingBackend: IBO " + InnoUtility::pointerToString(IBO) + " is initialized.");

	return true;
}

bool VKRenderingBackendNS::initializeVKTextureDataComponent(VKTextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "VKRenderingBackend: try to generate VKTextureDataComponent for TextureUsageType::INVISIBLE type!");
			return false;
		}
		else if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT
			|| rhs->m_textureDataDesc.usageType == TextureUsageType::RAW_IMAGE)
		{
			submitGPUData(rhs);
			return true;
		}
		else
		{
			if (rhs->m_textureData)
			{
				submitGPUData(rhs);

				if (rhs->m_textureDataDesc.usageType != TextureUsageType::COLOR_ATTACHMENT)
				{
					// @TODO: release raw data in heap memory
				}

				return true;
			}
			else
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "VKRenderingBackend: try to generate VKTextureDataComponent without raw data!");
				return false;
			}
		}
	}
}

bool VKRenderingBackendNS::submitGPUData(VKTextureDataComponent * rhs)
{
	rhs->m_VkTextureDataDesc = getVKTextureDataDesc(rhs->m_textureDataDesc);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(rhs->m_VkTextureDataDesc.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	auto l_srcData = rhs->m_textureData;
	if (l_srcData != nullptr)
	{
		void* l_dstData;
		vkMapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, 0, rhs->m_VkTextureDataDesc.imageSize, 0, &l_dstData);
		memcpy(l_dstData, rhs->m_textureData, static_cast<size_t>(rhs->m_VkTextureDataDesc.imageSize));
		vkUnmapMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory);
	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = rhs->m_VkTextureDataDesc.imageType;
	imageInfo.extent.width = rhs->m_textureDataDesc.width;
	imageInfo.extent.height = rhs->m_textureDataDesc.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = rhs->m_VkTextureDataDesc.format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	else if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	else if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	else
	{
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (vkCreateImage(VKRenderingBackendComponent::get().m_device, &imageInfo, nullptr, &rhs->m_image) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkImage!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(VKRenderingBackendComponent::get().m_device, rhs->m_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(VKRenderingBackendComponent::get().m_device, &allocInfo, nullptr, &VKRenderingBackendComponent::get().m_textureImageMemory) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to allocate VkDeviceMemory for VkImage!");
		return false;
	}

	vkBindImageMemory(VKRenderingBackendComponent::get().m_device, rhs->m_image, VKRenderingBackendComponent::get().m_textureImageMemory, 0);

	if (rhs->m_textureDataDesc.usageType == TextureUsageType::COLOR_ATTACHMENT)
	{
		transitionImageLayout(rhs->m_image, imageInfo.format, rhs->m_VkTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		transitionImageLayout(rhs->m_image, imageInfo.format, rhs->m_VkTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		transitionImageLayout(rhs->m_image, imageInfo.format, rhs->m_VkTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else
	{
		transitionImageLayout(rhs->m_image, imageInfo.format, rhs->m_VkTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		if (l_srcData != nullptr)
		{
			copyBufferToImage(stagingBuffer, rhs->m_image, rhs->m_VkTextureDataDesc.aspectFlags, static_cast<uint32_t>(imageInfo.extent.width), static_cast<uint32_t>(imageInfo.extent.height));
		}
		transitionImageLayout(rhs->m_image, imageInfo.format, rhs->m_VkTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, stagingBuffer, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, stagingBufferMemory, nullptr);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingBackend: VkImage " + InnoUtility::pointerToString(rhs->m_image) + " is initialized.");

	createImageView(rhs);

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedVKTDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

void VKRenderingBackendNS::transitionImageLayout(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Unsupported transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void VKRenderingBackendNS::copyBufferToImage(VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = aspectFlags;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

bool VKRenderingBackendNS::createImageView(VKTextureDataComponent* VKTDC)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = VKTDC->m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VKTDC->m_VkTextureDataDesc.format;
	viewInfo.subresourceRange.aspectMask = VKTDC->m_VkTextureDataDesc.aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(VKRenderingBackendComponent::get().m_device, &viewInfo, nullptr, &VKTDC->m_imageView) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkImageView!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingBackend: VkImageView " + InnoUtility::pointerToString(VKTDC->m_imageView) + " is initialized.");
	return true;
}

bool VKRenderingBackendNS::destroyAllGraphicPrimitiveComponents()
{
	for (auto i : m_initializedVKMDC)
	{
		vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, i.second->m_IBO, nullptr);
		vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, i.second->m_VBO, nullptr);
	}
	for (auto i : m_initializedVKTDC)
	{
		vkDestroyImage(VKRenderingBackendComponent::get().m_device, i.second->m_image, nullptr);
		vkDestroyImageView(VKRenderingBackendComponent::get().m_device, i.second->m_imageView, nullptr);
	}

	return true;
}

bool VKRenderingBackendNS::recordCommand(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex, const std::function<void()>& commands)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(VKRPC->m_commandBuffers[commandBufferIndex], &beginInfo) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to begin recording command buffer!");
		return false;
	}

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = VKRPC->m_renderPass;
	renderPassInfo.framebuffer = VKRPC->m_framebuffers[commandBufferIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = VKRPC->scissor.extent;

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	std::vector<VkClearValue> clearValues;

	for (size_t i = 0; i < VKRPC->m_renderPassDesc.RTNumber; i++)
	{
		clearValues.emplace_back(clearColor);
	}

	if (VKRPC->m_renderPassDesc.useDepthAttachment)
	{
		clearValues.emplace_back();
		clearValues[VKRPC->m_renderPassDesc.RTNumber].depthStencil = { 1.0f, 0 };
	}

	renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
	renderPassInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(VKRPC->m_commandBuffers[commandBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(VKRPC->m_commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, VKRPC->m_pipeline);

	commands();

	vkCmdEndRenderPass(VKRPC->m_commandBuffers[commandBufferIndex]);

	if (vkEndCommandBuffer(VKRPC->m_commandBuffers[commandBufferIndex]) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to record command!");
		return false;
	}

	return true;
}

bool VKRenderingBackendNS::recordDrawCall(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex, VKMeshDataComponent * VKMDC)
{
	VkBuffer vertexBuffers[] = { VKMDC->m_VBO };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(VKRPC->m_commandBuffers[commandBufferIndex], 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(VKRPC->m_commandBuffers[commandBufferIndex], VKMDC->m_IBO, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(VKRPC->m_commandBuffers[commandBufferIndex], static_cast<uint32_t>(VKMDC->m_indicesSize), 1, 0, 0, 0);
	return true;
}

bool VKRenderingBackendNS::waitForFence(VKRenderPassComponent* VKRPC)
{
	vkWaitForFences(VKRenderingBackendComponent::get().m_device,
		1,
		&VKRPC->m_inFlightFences[VKRPC->m_currentFrame],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max()
	);
	return true;
}

bool VKRenderingBackendNS::submitCommand(VKRenderPassComponent* VKRPC, unsigned int commandBufferIndex)
{
	// submit the draw command buffer with a rendering finished signal semaphore
	// command buffer
	VKRPC->submitInfo.commandBufferCount = 1;
	VKRPC->submitInfo.pCommandBuffers = &VKRPC->m_commandBuffers[commandBufferIndex];

	// signal semaphore
	VKRPC->submitInfo.signalSemaphoreCount = 1;
	VKRPC->submitInfo.pSignalSemaphores = &VKRPC->m_renderFinishedSemaphores[VKRPC->m_currentFrame];

	vkResetFences(VKRenderingBackendComponent::get().m_device, 1, &VKRPC->m_inFlightFences[VKRPC->m_currentFrame]);

	// submit to queue
	if (vkQueueSubmit(VKRenderingBackendComponent::get().m_graphicsQueue, 1, &VKRPC->submitInfo, VKRPC->m_inFlightFences[VKRPC->m_currentFrame]) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to submit command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingBackendNS::updateUBOImpl(VkDeviceMemory& UBOMemory, size_t size, const void* UBOValue)
{
	void* data;
	vkMapMemory(VKRenderingBackendComponent::get().m_device, UBOMemory, 0, size, 0, &data);
	std::memcpy(data, UBOValue, size);
	vkUnmapMemory(VKRenderingBackendComponent::get().m_device, UBOMemory);

	return true;
}

VKShaderProgramComponent * VKRenderingBackendNS::addVKShaderProgramComponent(const EntityID & rhs)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_VKShaderProgramComponentPool, sizeof(VKShaderProgramComponent));
	auto l_VKSPC = new(l_rawPtr)VKShaderProgramComponent();
	return l_VKSPC;
}

bool VKRenderingBackendNS::initializeVKShaderProgramComponent(VKShaderProgramComponent * rhs, const ShaderFilePaths & shaderFilePaths)
{
	bool l_result = true;
	if (shaderFilePaths.m_VSPath != "")
	{
		l_result &= createShaderModule(rhs->m_vertexShaderModule, shaderFilePaths.m_VSPath);
		rhs->m_vertexShaderStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rhs->m_vertexShaderStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rhs->m_vertexShaderStageCInfo.module = rhs->m_vertexShaderModule;
		rhs->m_vertexShaderStageCInfo.pName = "main";

		rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 1;
		rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size());
		rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_vertexBindingDescription;
		rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();
	}
	if (shaderFilePaths.m_FSPath != "")
	{
		l_result &= createShaderModule(rhs->m_fragmentShaderModule, shaderFilePaths.m_FSPath);
		rhs->m_fragmentShaderStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rhs->m_fragmentShaderStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		rhs->m_fragmentShaderStageCInfo.module = rhs->m_fragmentShaderModule;
		rhs->m_fragmentShaderStageCInfo.pName = "main";
	}

	return l_result;
}

bool VKRenderingBackendNS::createShaderModule(VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath)
{
	auto l_binData = g_pModuleManager->getFileSystem()->loadBinaryFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()));

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_binData.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t*>(l_binData.data());

	if (vkCreateShaderModule(VKRenderingBackendComponent::get().m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkShaderModule for: " + std::string(shaderFilePath.c_str()) + "!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: innoShader: " + std::string(shaderFilePath.c_str()) + " has been loaded.");
	return true;
}

bool VKRenderingBackendNS::destroyVKShaderProgramComponent(VKShaderProgramComponent* VKSPC)
{
	vkDestroyShaderModule(VKRenderingBackendComponent::get().m_device, VKSPC->m_fragmentShaderModule, nullptr);
	vkDestroyShaderModule(VKRenderingBackendComponent::get().m_device, VKSPC->m_vertexShaderModule, nullptr);

	return true;
}

bool VKRenderingBackendNS::generateUBO(VkBuffer& UBO, VkDeviceSize UBOSize, VkDeviceMemory& UBOMemory)
{
	auto l_result = createBuffer(UBOSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		UBO,
		UBOMemory);

	if (!l_result)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create UBO!");
		return false;
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: UBO has been created.");
	return true;
}

VkTextureDataDesc VKRenderingBackendNS::getVKTextureDataDesc(TextureDataDesc textureDataDesc)
{
	VkTextureDataDesc l_result;
	l_result.imageType = getImageType(textureDataDesc.samplerType);
	l_result.samplerAddressMode = getSamplerAddressMode(textureDataDesc.wrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);
	l_result.format = getTextureFormat(textureDataDesc);
	l_result.imageSize = textureDataDesc.width * textureDataDesc.height * ((unsigned int)textureDataDesc.pixelDataFormat + 1);
	l_result.aspectFlags = getImageAspectFlags(textureDataDesc);
	return l_result;
}

VkImageType VKRenderingBackendNS::getImageType(TextureSamplerType rhs)
{
	VkImageType l_result;

	switch (rhs)
	{
	case TextureSamplerType::SAMPLER_1D:
		l_result = VkImageType::VK_IMAGE_TYPE_1D;
		break;
	case TextureSamplerType::SAMPLER_2D:
		l_result = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	case TextureSamplerType::SAMPLER_3D:
		l_result = VkImageType::VK_IMAGE_TYPE_3D;
		break;
	case TextureSamplerType::CUBEMAP:
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerAddressMode VKRenderingBackendNS::getSamplerAddressMode(TextureWrapMethod rhs)
{
	VkSamplerAddressMode l_result;

	switch (rhs)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TextureWrapMethod::REPEAT:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TextureWrapMethod::CLAMP_TO_BORDER:
		l_result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}

	return l_result;
}

VkSamplerMipmapMode VKRenderingBackendNS::getTextureFilterParam(TextureFilterMethod rhs)
{
	VkSamplerMipmapMode l_result;

	switch (rhs)
	{
	case TextureFilterMethod::NEAREST:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case TextureFilterMethod::LINEAR:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR:
		l_result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_MAX_ENUM; // ????????
		break;
	default:
		break;
	}

	return l_result;
}

VkFormat VKRenderingBackendNS::getTextureFormat(TextureDataDesc textureDataDesc)
{
	VkFormat l_internalFormat = VK_FORMAT_R8_UNORM;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		l_internalFormat = VK_FORMAT_R8G8B8A8_SRGB;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D32_SFLOAT;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_SNORM; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_UNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_UNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_UINT; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R8_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R8G8_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R8G8B8A8_SINT; break;
			case TexturePixelDataFormat::BGRA: l_internalFormat = VK_FORMAT_B8G8R8A8_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R32_UINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R32G32_UINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R32G32B32A32_UINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R32G32B32A32_UINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R32_SINT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R32G32_SINT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R32G32B32A32_SINT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R32G32B32A32_SINT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = VK_FORMAT_R16_SFLOAT; break;
			case TexturePixelDataFormat::RG: l_internalFormat = VK_FORMAT_R16G16_SFLOAT; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
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

VkImageAspectFlagBits VKRenderingBackendNS::getImageAspectFlags(TextureDataDesc textureDataDesc)
{
	VkImageAspectFlagBits l_result;

	if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		l_result = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	}

	return l_result;
}