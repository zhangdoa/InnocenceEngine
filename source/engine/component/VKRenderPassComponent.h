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

	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkViewport m_viewport = {};
	VkRect2D m_scissor = {};
	VkPipelineViewportStateCreateInfo m_viewportStateCInfo = {};
	VkPipelineRasterizationStateCreateInfo m_rasterizationStateCInfo = {};
	VkPipelineMultisampleStateCreateInfo m_multisampleStateCInfo = {};
	VkPipelineColorBlendAttachmentState m_colorBlendAttachmentState = {};
	VkPipelineColorBlendStateCreateInfo m_colorBlendStateCInfo = {};
	VkPipelineLayoutCreateInfo m_pipelineLayoutCInfo = {};
	VkGraphicsPipelineCreateInfo m_pipelineCInfo = {};

	std::vector<TextureDataComponent*> m_TDCs;
	std::vector<VKTextureDataComponent*> m_VKTDCs;
};