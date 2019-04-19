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

	std::vector<VkAttachmentDescription> attachmentDescs;
	VkRenderPassCreateInfo renderPassCInfo = {};

	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
	std::vector<VkDescriptorImageInfo> descriptorImageInfos;
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	std::vector<VkPushConstantRange> pushConstantRanges;

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

	VkSubmitInfo submitInfo;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	size_t m_maxFramesInFlight = 1;
	size_t m_currentFrame = 0;
};