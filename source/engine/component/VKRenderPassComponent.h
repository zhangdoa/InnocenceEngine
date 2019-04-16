#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "TextureDataComponent.h"
#include "VKTextureDataComponent.h"

class VKRenderPassComponent
{
public:
	VKRenderPassComponent() {};
	~VKRenderPassComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	RenderPassDesc m_renderPassDesc;

	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkAttachmentDescription attachmentDesc = {};
	VkAttachmentReference attachmentRef = {};
	VkSubpassDescription subpassDesc = {};
	VkRenderPassCreateInfo renderPassCInfo = {};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCInfo = {};
	VkPipelineViewportStateCreateInfo viewportStateCInfo = {};
	VkPipelineRasterizationStateCreateInfo rasterizationStateCInfo = {};
	VkPipelineMultisampleStateCreateInfo multisampleStateCInfo = {};
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCInfo = {};
	VkPipelineLayoutCreateInfo pipelineLayoutCInfo = {};
	VkGraphicsPipelineCreateInfo pipelineCInfo = {};
	VkViewport viewport = {};
	VkRect2D scissor = {};

	std::vector<VkFramebuffer> m_framebuffers;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<VKTextureDataComponent*> m_VKTDCs;
};