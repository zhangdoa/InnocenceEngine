#pragma once
#include "VKRenderingSystemUtilities.h"
#include "../../component/VKRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	bool createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	bool createVBO(const std::vector<Vertex>& vertices, VkBuffer& VBO);
	bool createIBO(const std::vector<Index>& indices, VkBuffer& IBO);

	bool initializeVKMeshDataComponent(VKMeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
	bool initializeVKTextureDataComponent(VKTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData);

	VKTextureDataDesc getVKTextureDataDesc(const TextureDataDesc& textureDataDesc);

	VkSamplerAddressMode getTextureWrapMethod(TextureWrapMethod rhs);
	VkSamplerMipmapMode getTextureFilterParam(TextureFilterMethod rhs);
	VkFormat getTextureInternalFormat(TextureColorComponentsFormat rhs);

	VkVertexInputBindingDescription m_bindingDescription;
	std::array<VkVertexInputAttributeDescription, 5> m_attributeDescriptions;

	std::unordered_map<EntityID, VKMeshDataComponent*> m_initializedVKMDC;
	std::unordered_map<EntityID, VKTextureDataComponent*> m_initializedVKTDC;

	std::unordered_map<EntityID, VKMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, VKTextureDataComponent*> m_textureMap;

	void* m_VKMeshDataComponentPool;
	void* m_VKTextureDataComponentPool;
	void* m_VKRenderPassComponentPool;
	void* m_VKShaderProgramComponentPool;
}

bool VKRenderingSystemNS::initializeComponentPool()
{
	m_VKMeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKMeshDataComponent), 32768);
	m_VKTextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKTextureDataComponent), 32768);
	m_VKRenderPassComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKRenderPassComponent), 32);
	m_VKShaderProgramComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKShaderProgramComponent), 128);

	m_bindingDescription = {};
	m_bindingDescription.binding = 0;
	m_bindingDescription.stride = sizeof(Vertex);
	m_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_attributeDescriptions = {};

	m_attributeDescriptions[0].binding = 0;
	m_attributeDescriptions[0].location = 0;
	m_attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_attributeDescriptions[0].offset = offsetof(Vertex, m_pos);

	m_attributeDescriptions[1].binding = 0;
	m_attributeDescriptions[1].location = 1;
	m_attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	m_attributeDescriptions[1].offset = offsetof(Vertex, m_texCoord);

	m_attributeDescriptions[2].binding = 0;
	m_attributeDescriptions[2].location = 2;
	m_attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	m_attributeDescriptions[2].offset = offsetof(Vertex, m_pad1);

	m_attributeDescriptions[3].binding = 0;
	m_attributeDescriptions[3].location = 3;
	m_attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_attributeDescriptions[3].offset = offsetof(Vertex, m_normal);

	m_attributeDescriptions[4].binding = 0;
	m_attributeDescriptions[4].location = 4;
	m_attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_attributeDescriptions[4].offset = offsetof(Vertex, m_pad2);

	return true;
}

VKRenderPassComponent* VKRenderingSystemNS::addVKRenderPassComponent(unsigned int RTNum, TextureDataDesc RTDesc, const std::vector<VkImage>* VkImages, MeshPrimitiveTopology topology, VKShaderProgramComponent* VKSPC)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKRenderPassComponentPool, sizeof(VKRenderPassComponent));
	auto l_VKRPC = new(l_rawPtr)VKRenderPassComponent();
	auto l_VKRTDesc = getVKTextureDataDesc(RTDesc);

	l_VKRPC->m_framebuffers.reserve(RTNum);
	for (size_t i = 0; i < RTNum; i++)
	{
		l_VKRPC->m_framebuffers.emplace_back();
	}

	l_VKRPC->m_VKTDCs.reserve(RTNum);
	for (size_t i = 0; i < RTNum; i++)
	{
		l_VKRPC->m_VKTDCs.emplace_back();
	}

	for (size_t i = 0; i < RTNum; i++)
	{
		l_VKRPC->m_VKTDCs[i] = addVKTextureDataComponent(l_VKRPC->m_parentEntity);
	}

	// @TODO: ugly
	if (VkImages)
	{
		for (size_t i = 0; i < VkImages->size(); i++)
		{
			l_VKRPC->m_VKTDCs[i]->m_image = VkImages->data()[i];
			l_VKRPC->m_VKTDCs[i]->m_VKTextureDataDesc = l_VKRTDesc;
		}
	}

	for (size_t i = 0; i < RTNum; i++)
	{
		VkImageViewCreateInfo l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_createInfo.image = l_VKRPC->m_VKTDCs[i]->m_image;
		l_createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_createInfo.format = l_VKRPC->m_VKTDCs[i]->m_VKTextureDataDesc.internalFormat;
		l_createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_createInfo.subresourceRange.baseMipLevel = 0;
		l_createInfo.subresourceRange.levelCount = 1;
		l_createInfo.subresourceRange.baseArrayLayer = 0;
		l_createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &l_VKRPC->m_VKTDCs[i]->m_imageView) != VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create image view!");
			return false;
		}

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = l_VKRPC->m_VKTDCs[0]->m_VKTextureDataDesc.internalFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(VKRenderingSystemComponent::get().m_device, &renderPassInfo, nullptr, &l_VKRPC->m_renderPass) != VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create render pass!");
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: render pass has been created.");

		VkImageView attachments[] =
		{
			l_VKRPC->m_VKTDCs[i]->m_imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = l_VKRPC->m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = RTDesc.textureWidth;
		framebufferInfo.height = RTDesc.textureHeight;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VKRenderingSystemComponent::get().m_device, &framebufferInfo, nullptr, &l_VKRPC->m_framebuffers[i]) != VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create framebuffer!");
		}
	}

	VkPrimitiveTopology l_topology;

	switch (topology)
	{
	case MeshPrimitiveTopology::POINT:
		l_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case MeshPrimitiveTopology::LINE:
		l_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case MeshPrimitiveTopology::TRIANGLE:
		l_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case MeshPrimitiveTopology::TRIANGLE_STRIP:
		l_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	default:
		break;
	}

	l_VKRPC->m_inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	l_VKRPC->m_inputAssemblyStateCInfo.topology = l_topology;
	l_VKRPC->m_inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	l_VKRPC->m_extent = VkExtent2D();
	l_VKRPC->m_extent.width = RTDesc.textureWidth;
	l_VKRPC->m_extent.height = RTDesc.textureHeight;

	l_VKRPC->m_viewport.x = 0.0f;
	l_VKRPC->m_viewport.y = 0.0f;
	l_VKRPC->m_viewport.width = (float)RTDesc.textureWidth;
	l_VKRPC->m_viewport.height = (float)RTDesc.textureHeight;
	l_VKRPC->m_viewport.minDepth = 0.0f;
	l_VKRPC->m_viewport.maxDepth = 1.0f;

	l_VKRPC->m_scissor.offset = { 0, 0 };
	l_VKRPC->m_scissor.extent = l_VKRPC->m_extent;

	l_VKRPC->m_viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	l_VKRPC->m_viewportStateCInfo.viewportCount = 1;
	l_VKRPC->m_viewportStateCInfo.pViewports = &l_VKRPC->m_viewport;
	l_VKRPC->m_viewportStateCInfo.scissorCount = 1;
	l_VKRPC->m_viewportStateCInfo.pScissors = &l_VKRPC->m_scissor;

	l_VKRPC->m_rasterizationStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	l_VKRPC->m_rasterizationStateCInfo.depthClampEnable = VK_FALSE;
	l_VKRPC->m_rasterizationStateCInfo.rasterizerDiscardEnable = VK_FALSE;
	l_VKRPC->m_rasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	l_VKRPC->m_rasterizationStateCInfo.lineWidth = 1.0f;
	l_VKRPC->m_rasterizationStateCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	l_VKRPC->m_rasterizationStateCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	l_VKRPC->m_rasterizationStateCInfo.depthBiasEnable = VK_FALSE;

	l_VKRPC->m_multisampleStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	l_VKRPC->m_multisampleStateCInfo.sampleShadingEnable = VK_FALSE;
	l_VKRPC->m_multisampleStateCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	l_VKRPC->m_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_VKRPC->m_colorBlendAttachmentState.blendEnable = VK_FALSE;

	l_VKRPC->m_colorBlendStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	l_VKRPC->m_colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	l_VKRPC->m_colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	l_VKRPC->m_colorBlendStateCInfo.attachmentCount = 1;
	l_VKRPC->m_colorBlendStateCInfo.pAttachments = &l_VKRPC->m_colorBlendAttachmentState;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[0] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[1] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[2] = 0.0f;
	l_VKRPC->m_colorBlendStateCInfo.blendConstants[3] = 0.0f;

	l_VKRPC->m_pipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_VKRPC->m_pipelineLayoutCInfo.setLayoutCount = 0;
	l_VKRPC->m_pipelineLayoutCInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(VKRenderingSystemComponent::get().m_device, &l_VKRPC->m_pipelineLayoutCInfo, nullptr, &l_VKRPC->m_pipelineLayout) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create pipeline layout!");
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: pipeline layout has been created.");

	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStages = { VKSPC->m_vertexShaderStageCInfo, VKSPC->m_fragmentShaderStageCInfo };

	l_VKRPC->m_pipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_VKRPC->m_pipelineCInfo.stageCount = 2;
	l_VKRPC->m_pipelineCInfo.pStages = &l_shaderStages[0];
	l_VKRPC->m_pipelineCInfo.pVertexInputState = &VKSPC->m_vertexInputStateCInfo;
	l_VKRPC->m_pipelineCInfo.pInputAssemblyState = &l_VKRPC->m_inputAssemblyStateCInfo;
	l_VKRPC->m_pipelineCInfo.pViewportState = &l_VKRPC->m_viewportStateCInfo;
	l_VKRPC->m_pipelineCInfo.pRasterizationState = &l_VKRPC->m_rasterizationStateCInfo;
	l_VKRPC->m_pipelineCInfo.pMultisampleState = &l_VKRPC->m_multisampleStateCInfo;
	l_VKRPC->m_pipelineCInfo.pColorBlendState = &l_VKRPC->m_colorBlendStateCInfo;
	l_VKRPC->m_pipelineCInfo.layout = l_VKRPC->m_pipelineLayout;
	l_VKRPC->m_pipelineCInfo.renderPass = l_VKRPC->m_renderPass;
	l_VKRPC->m_pipelineCInfo.subpass = 0;
	l_VKRPC->m_pipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(VKRenderingSystemComponent::get().m_device, VK_NULL_HANDLE, 1, &l_VKRPC->m_pipelineCInfo, nullptr, &l_VKRPC->m_pipeline) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to to create graphics pipeline!");
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: graphics pipeline has been created.");

	return l_VKRPC;
}

bool VKRenderingSystemNS::destroyVKRenderPassComponent(VKRenderPassComponent* VKRPC)
{
	for (auto framebuffer : VKRPC->m_framebuffers)
	{
		vkDestroyFramebuffer(VKRenderingSystemComponent::get().m_device, framebuffer, nullptr);
	}
	vkDestroyPipeline(VKRenderingSystemComponent::get().m_device, VKRPC->m_pipeline, nullptr);
	vkDestroyPipelineLayout(VKRenderingSystemComponent::get().m_device, VKRPC->m_pipelineLayout, nullptr);
	vkDestroyRenderPass(VKRenderingSystemComponent::get().m_device, VKRPC->m_renderPass, nullptr);
	for (auto VKTDC : VKRPC->m_VKTDCs)
	{
		vkDestroyImageView(VKRenderingSystemComponent::get().m_device, VKTDC->m_imageView, nullptr);
	}

	return true;
}

VKMeshDataComponent* VKRenderingSystemNS::generateVKMeshDataComponent(MeshDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getVKMeshDataComponent(rhs->m_parentEntity);
	}
	else
	{
		auto l_ptr = addVKMeshDataComponent(rhs->m_parentEntity);

		initializeVKMeshDataComponent(l_ptr, rhs->m_vertices, rhs->m_indices);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return l_ptr;
	}
}

uint32_t VKRenderingSystemNS::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(VKRenderingSystemComponent::get().m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to find suitable memory type!!");
	return 0;
}

bool VKRenderingSystemNS::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(VKRenderingSystemComponent::get().m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create create buffer!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VKRenderingSystemComponent::get().m_device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(VKRenderingSystemComponent::get().m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to allocate buffer memory!");
		return false;
	}

	vkBindBufferMemory(VKRenderingSystemComponent::get().m_device, buffer, bufferMemory, 0);

	return true;
}

bool copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = VKRenderingSystemComponent::get().m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VKRenderingSystemComponent::get().m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VKRenderingSystemComponent::get().m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VKRenderingSystemComponent::get().m_graphicsQueue);

	vkFreeCommandBuffers(VKRenderingSystemComponent::get().m_device, VKRenderingSystemComponent::get().m_commandPool, 1, &commandBuffer);

	return true;
}

bool VKRenderingSystemNS::createVBO(const std::vector<Vertex>& vertices, VkBuffer& VBO)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VBO, VKRenderingSystemComponent::get().m_vertexBufferMemory);

	copyBuffer(stagingBuffer, VBO, bufferSize);

	vkDestroyBuffer(VKRenderingSystemComponent::get().m_device, stagingBuffer, nullptr);
	vkFreeMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory, nullptr);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingSystem: VBO " + InnoUtility::pointerToString(VBO) + " is initialized.");

	return true;
}

bool  VKRenderingSystemNS::createIBO(const std::vector<Index>& indices, VkBuffer& IBO)
{
	VkDeviceSize bufferSize = sizeof(Index) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IBO, VKRenderingSystemComponent::get().m_indexBufferMemory);

	copyBuffer(stagingBuffer, IBO, bufferSize);

	vkDestroyBuffer(VKRenderingSystemComponent::get().m_device, stagingBuffer, nullptr);
	vkFreeMemory(VKRenderingSystemComponent::get().m_device, stagingBufferMemory, nullptr);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingSystem: IBO " + InnoUtility::pointerToString(IBO) + " is initialized.");

	return true;
}

bool VKRenderingSystemNS::initializeVKMeshDataComponent(VKMeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices)
{
	createVBO(vertices, rhs->m_VBO);
	createIBO(indices, rhs->m_IBO);

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedVKMDC.emplace(rhs->m_parentEntity, rhs);

	return true;
}

VKTextureDataComponent* VKRenderingSystemNS::generateVKTextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getVKTextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureDataDesc.textureUsageType == TextureUsageType::INVISIBLE)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: TextureUsageType is TextureUsageType::INVISIBLE!");
			return nullptr;
		}
		else
		{
			auto l_ptr = addVKTextureDataComponent(rhs->m_parentEntity);

			initializeVKTextureDataComponent(l_ptr, rhs->m_textureDataDesc, rhs->m_textureData);

			rhs->m_objectStatus = ObjectStatus::ALIVE;

			return l_ptr;
		}
	}
}

bool VKRenderingSystemNS::initializeVKTextureDataComponent(VKTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
{
	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedVKTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VKRenderingSystem: SRV is initialized.");

	return true;
}

VKMeshDataComponent* VKRenderingSystemNS::addVKMeshDataComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKMeshDataComponentPool, sizeof(VKMeshDataComponent));
	auto l_VKMDC = new(l_rawPtr)VKMeshDataComponent();
	l_VKMDC->m_parentEntity = rhs;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, VKMeshDataComponent*>(rhs, l_VKMDC));
	return l_VKMDC;
}

VKTextureDataComponent* VKRenderingSystemNS::addVKTextureDataComponent(EntityID rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKTextureDataComponentPool, sizeof(VKTextureDataComponent));
	auto l_VKTDC = new(l_rawPtr)VKTextureDataComponent();
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, VKTextureDataComponent*>(rhs, l_VKTDC));
	return l_VKTDC;
}

VKMeshDataComponent * VKRenderingSystemNS::getVKMeshDataComponent(EntityID rhs)
{
	auto result = m_meshMap.find(rhs);
	if (result != m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

VKTextureDataComponent * VKRenderingSystemNS::getVKTextureDataComponent(EntityID rhs)
{
	auto result = m_textureMap.find(rhs);
	if (result != m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

void VKRenderingSystemNS::recordDrawCall(VkCommandBuffer commandBuffer, EntityID rhs)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(rhs);
	if (l_MDC)
	{
		recordDrawCall(commandBuffer, l_MDC);
	}
}

void VKRenderingSystemNS::recordDrawCall(VkCommandBuffer commandBuffer, MeshDataComponent * MDC)
{
	auto l_VKMDC = VKRenderingSystemNS::getVKMeshDataComponent(MDC->m_parentEntity);
	if (l_VKMDC)
	{
		if (MDC->m_objectStatus == ObjectStatus::ALIVE && l_VKMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			recordDrawCall(commandBuffer, MDC->m_indicesSize, l_VKMDC);
		}
	}
}

void VKRenderingSystemNS::recordDrawCall(VkCommandBuffer commandBuffer, size_t indicesSize, VKMeshDataComponent * VKMDC)
{
	VkBuffer vertexBuffers[] = { VKMDC->m_VBO };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, VKMDC->m_IBO, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicesSize), 1, 0, 0, 0);
}

bool VKRenderingSystemNS::createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath)
{
	auto l_binData = g_pCoreSystem->getFileSystem()->loadBinaryFile(shaderFilePath);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_binData.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t*>(l_binData.data());

	if (vkCreateShaderModule(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create shader module for: " + shaderFilePath + "!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: innoShader: " + shaderFilePath + " has been loaded.");
	return true;
}

VKShaderProgramComponent * VKRenderingSystemNS::addVKShaderProgramComponent(const EntityID & rhs)
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKShaderProgramComponentPool, sizeof(VKShaderProgramComponent));
	auto l_VKSPC = new(l_rawPtr)VKShaderProgramComponent();
	return l_VKSPC;
}

bool VKRenderingSystemNS::initializeVKShaderProgramComponent(VKShaderProgramComponent * rhs, const ShaderFilePaths & shaderFilePaths)
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
		rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
		rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_bindingDescription;
		rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();
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

bool VKRenderingSystemNS::activateVKShaderProgramComponent(VKShaderProgramComponent * rhs)
{
	return false;
}

VKTextureDataDesc VKRenderingSystemNS::getVKTextureDataDesc(const TextureDataDesc & textureDataDesc)
{
	VKTextureDataDesc l_result;

	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.textureWrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.textureMinFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.textureMagFilterMethod);
	l_result.internalFormat = getTextureInternalFormat(textureDataDesc.textureColorComponentsFormat);

	return l_result;
}

VkSamplerAddressMode VKRenderingSystemNS::getTextureWrapMethod(TextureWrapMethod rhs)
{
	VkSamplerAddressMode result;

	switch (rhs)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE:
		result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TextureWrapMethod::REPEAT:
		result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TextureWrapMethod::CLAMP_TO_BORDER:
		result = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}

	return result;
}

VkSamplerMipmapMode VKRenderingSystemNS::getTextureFilterParam(TextureFilterMethod rhs)
{
	VkSamplerMipmapMode result;

	switch (rhs)
	{
	case TextureFilterMethod::NEAREST:
		result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case TextureFilterMethod::LINEAR:
		result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR:
		result = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_MAX_ENUM; // ????????
		break;
	default:
		break;
	}

	return result;
}

VkFormat VKRenderingSystemNS::getTextureInternalFormat(TextureColorComponentsFormat rhs)
{
	VkFormat result;

	// @TODO: with pixel format together
	switch (rhs)
	{
	case TextureColorComponentsFormat::RED:
		result = VkFormat::VK_FORMAT_R8_UNORM;
		break;
	case TextureColorComponentsFormat::RG:
		result = VkFormat::VK_FORMAT_R8G8_UNORM;
		break;
	case TextureColorComponentsFormat::RGB:
		result = VkFormat::VK_FORMAT_R8G8B8_UNORM;
		break;
	case TextureColorComponentsFormat::RGBA:
		result = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureColorComponentsFormat::R8:
		break;
	case TextureColorComponentsFormat::RG8:
		break;
	case TextureColorComponentsFormat::RGB8:
		break;
	case TextureColorComponentsFormat::RGBA8:
		break;
	case TextureColorComponentsFormat::R8I:
		break;
	case TextureColorComponentsFormat::RG8I:
		break;
	case TextureColorComponentsFormat::RGB8I:
		break;
	case TextureColorComponentsFormat::RGBA8I:
		break;
	case TextureColorComponentsFormat::R8UI:
		break;
	case TextureColorComponentsFormat::RG8UI:
		break;
	case TextureColorComponentsFormat::RGB8UI:
		break;
	case TextureColorComponentsFormat::RGBA8UI:
		break;
	case TextureColorComponentsFormat::R16:
		break;
	case TextureColorComponentsFormat::RG16:
		break;
	case TextureColorComponentsFormat::RGB16:
		break;
	case TextureColorComponentsFormat::RGBA16:
		break;
	case TextureColorComponentsFormat::R16I:
		break;
	case TextureColorComponentsFormat::RG16I:
		break;
	case TextureColorComponentsFormat::RGB16I:
		break;
	case TextureColorComponentsFormat::RGBA16I:
		break;
	case TextureColorComponentsFormat::R16UI:
		break;
	case TextureColorComponentsFormat::RG16UI:
		break;
	case TextureColorComponentsFormat::RGB16UI:
		break;
	case TextureColorComponentsFormat::RGBA16UI:
		break;
	case TextureColorComponentsFormat::R16F:
		break;
	case TextureColorComponentsFormat::RG16F:
		break;
	case TextureColorComponentsFormat::RGB16F:
		break;
	case TextureColorComponentsFormat::RGBA16F:
		break;
	case TextureColorComponentsFormat::R32I:
		break;
	case TextureColorComponentsFormat::RG32I:
		break;
	case TextureColorComponentsFormat::RGB32I:
		break;
	case TextureColorComponentsFormat::RGBA32I:
		break;
	case TextureColorComponentsFormat::R32UI:
		break;
	case TextureColorComponentsFormat::RG32UI:
		break;
	case TextureColorComponentsFormat::RGB32UI:
		break;
	case TextureColorComponentsFormat::RGBA32UI:
		break;
	case TextureColorComponentsFormat::R32F:
		break;
	case TextureColorComponentsFormat::RG32F:
		break;
	case TextureColorComponentsFormat::RGB32F:
		break;
	case TextureColorComponentsFormat::RGBA32F:
		break;
	case TextureColorComponentsFormat::SRGB:
		break;
	case TextureColorComponentsFormat::SRGBA:
		break;
	case TextureColorComponentsFormat::SRGB8:
		break;
	case TextureColorComponentsFormat::SRGBA8:
		break;
	case TextureColorComponentsFormat::DEPTH_COMPONENT:
		break;
	case TextureColorComponentsFormat::BGR:
		result = VkFormat::VK_FORMAT_B8G8R8_UNORM;
		break;
	case TextureColorComponentsFormat::BGRA:
		result = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
		break;
	default:
		break;
	}

	return result;
}