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

	VkAttachmentReference attachmentRef = {};
	VkSubpassDescription subpassDesc = {};

	VkAttachmentDescription attachmentDesc = {};
	VkRenderPassCreateInfo renderPassCInfo = {};

	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

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
	std::vector<VKTextureDataComponent*> m_VKTDCs;
};