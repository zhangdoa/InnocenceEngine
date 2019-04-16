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

	VkTextureDataDesc getVKTextureDataDesc(const TextureDataDesc& textureDataDesc);

	VkSamplerAddressMode getTextureWrapMethod(TextureWrapMethod rhs);
	VkSamplerMipmapMode getTextureFilterParam(TextureFilterMethod rhs);
	VkFormat getTextureInternalFormat(TextureColorComponentsFormat rhs);

	VkVertexInputBindingDescription m_vertexBindingDescription;
	std::array<VkVertexInputAttributeDescription, 5> m_vertexAttributeDescriptions;

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

bool VKRenderingSystemNS::createRenderTargets(VkTextureDataDesc vkTextureDataDesc, VKRenderPassComponent* VKRPC)
{
	for (size_t i = 0; i < VKRPC->m_VKTDCs.size(); i++)
	{
		VKRPC->m_VKTDCs[i]->m_VkTextureDataDesc = vkTextureDataDesc;

		// create image view
		VkImageViewCreateInfo l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_createInfo.image = VKRPC->m_VKTDCs[i]->m_image;
		l_createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_createInfo.format = VKRPC->m_VKTDCs[i]->m_VkTextureDataDesc.internalFormat;
		l_createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_createInfo.subresourceRange.baseMipLevel = 0;
		l_createInfo.subresourceRange.levelCount = 1;
		l_createInfo.subresourceRange.baseArrayLayer = 0;
		l_createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &VKRPC->m_VKTDCs[i]->m_imageView) != VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create ImageView!");
		}

		// create frame buffer and attach image view
		VkImageView attachments = { VKRPC->m_VKTDCs[i]->m_imageView };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = VKRPC->m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &attachments;
		framebufferInfo.width = vkTextureDataDesc.width;
		framebufferInfo.height = vkTextureDataDesc.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VKRenderingSystemComponent::get().m_device, &framebufferInfo, nullptr, &VKRPC->m_framebuffers[i]) != VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create Framebuffer!");
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: ImageView and Framebuffer has been created.");

	return true;
}

bool VKRenderingSystemNS::createRenderPass(VKRenderPassComponent* VKRPC)
{
	if (vkCreateRenderPass(VKRenderingSystemComponent::get().m_device, &VKRPC->m_infos.renderPassCInfo, nullptr, &VKRPC->m_renderPass) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create RenderPass!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: RenderPass has been created.");
	return true;
}

bool VKRenderingSystemNS::createPipelineLayout(VKRenderPassComponent* VKRPC)
{
	if (vkCreatePipelineLayout(VKRenderingSystemComponent::get().m_device, &VKRPC->m_infos.pipelineLayoutCInfo, nullptr, &VKRPC->m_pipelineLayout) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create PipelineLayout!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: PipelineLayout has been created.");
	return true;
}

bool VKRenderingSystemNS::createGraphicsPipelines(VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC)
{
	// attach shader module and create pipeline
	std::vector<VkPipelineShaderStageCreateInfo> l_shaderStages = { VKSPC->m_vertexShaderStageCInfo, VKSPC->m_fragmentShaderStageCInfo };

	VKRPC->m_infos.pipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	VKRPC->m_infos.pipelineCInfo.stageCount = (uint32_t)l_shaderStages.size();
	VKRPC->m_infos.pipelineCInfo.pStages = &l_shaderStages[0];
	VKRPC->m_infos.pipelineCInfo.pVertexInputState = &VKSPC->m_vertexInputStateCInfo;
	VKRPC->m_infos.pipelineCInfo.pInputAssemblyState = &VKRPC->m_infos.inputAssemblyStateCInfo;
	VKRPC->m_infos.pipelineCInfo.pViewportState = &VKRPC->m_infos.viewportStateCInfo;
	VKRPC->m_infos.pipelineCInfo.pRasterizationState = &VKRPC->m_infos.rasterizationStateCInfo;
	VKRPC->m_infos.pipelineCInfo.pMultisampleState = &VKRPC->m_infos.multisampleStateCInfo;
	VKRPC->m_infos.pipelineCInfo.pColorBlendState = &VKRPC->m_infos.colorBlendStateCInfo;
	VKRPC->m_infos.pipelineCInfo.layout = VKRPC->m_pipelineLayout;
	VKRPC->m_infos.pipelineCInfo.renderPass = VKRPC->m_renderPass;
	VKRPC->m_infos.pipelineCInfo.subpass = 0;
	VKRPC->m_infos.pipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(VKRenderingSystemComponent::get().m_device, VK_NULL_HANDLE, 1, &VKRPC->m_infos.pipelineCInfo, nullptr, &VKRPC->m_pipeline) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to to create GraphicsPipelines!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: GraphicsPipelines has been created.");
	return true;
}
VKRenderPassComponent* VKRenderingSystemNS::addVKRenderPassComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_VKRenderPassComponentPool, sizeof(VKRenderPassComponent));
	auto l_VKRPC = new(l_rawPtr)VKRenderPassComponent();

	return l_VKRPC;
}

bool VKRenderingSystemNS::reserveRenderTargets(RenderPassDesc renderPassDesc, VKRenderPassComponent* VKRPC)
{
	// reserve vectors and emplace empty objects
	VKRPC->m_framebuffers.reserve(renderPassDesc.RTNumber);
	for (size_t i = 0; i < renderPassDesc.RTNumber; i++)
	{
		VKRPC->m_framebuffers.emplace_back();
	}

	VKRPC->m_VKTDCs.reserve(renderPassDesc.RTNumber);
	for (size_t i = 0; i < renderPassDesc.RTNumber; i++)
	{
		VKRPC->m_VKTDCs.emplace_back();
	}

	for (size_t i = 0; i < renderPassDesc.RTNumber; i++)
	{
		VKRPC->m_VKTDCs[i] = addVKTextureDataComponent(VKRPC->m_parentEntity);
	}

	return true;
}

bool VKRenderingSystemNS::initializeVKRenderPassComponent(RenderPassDesc renderPassDesc, VKRenderPassComponent* VKRPC, VKShaderProgramComponent* VKSPC)
{
	bool result = true;

	result &= reserveRenderTargets(renderPassDesc, VKRPC);

	auto l_vkTextureDesc = getVKTextureDataDesc(renderPassDesc.RTDesc);

	result &= createRenderTargets(l_vkTextureDesc, VKRPC);

	result &= createRenderPass(VKRPC);

	result &= createPipelineLayout(VKRPC);

	result &= createGraphicsPipelines(VKRPC, VKSPC);

	return result;
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
		if (rhs->m_textureDataDesc.usageType == TextureUsageType::INVISIBLE)
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

bool VKRenderingSystemNS::activateVKShaderProgramComponent(VKShaderProgramComponent * rhs)
{
	return false;
}

VkTextureDataDesc VKRenderingSystemNS::getVKTextureDataDesc(const TextureDataDesc & textureDataDesc)
{
	VkTextureDataDesc l_result;

	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.wrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);
	l_result.internalFormat = getTextureInternalFormat(textureDataDesc.colorComponentsFormat);

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
	VkFormat result = VK_FORMAT_R8_UNORM;

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