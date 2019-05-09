#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "RenderPassComponent.h"
#include "VKTextureDataComponent.h"

class VKRenderPassComponent : public RenderPassComponent
{
public:
	VKRenderPassComponent() {};
	~VKRenderPassComponent() {};

	std::vector<VkAttachmentReference> colorAttachmentRefs = {};
	VkAttachmentReference depthAttachmentRef = {};
	VkSubpassDescription subpassDesc = {};

	std::vector<VkAttachmentDescription> attachmentDescs;
	VkRenderPassCreateInfo renderPassCInfo = {};

	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	std::vector<VkPushConstantRange> pushConstantRanges;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCInfo = {};
	VkPipelineViewportStateCreateInfo viewportStateCInfo = {};
	VkPipelineRasterizationStateCreateInfo rasterizationStateCInfo = {};
	VkPipelineMultisampleStateCreateInfo multisampleStateCInfo = {};
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates = {};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCInfo = {};
	VkPipelineLayoutCreateInfo pipelineLayoutCInfo = {};
	VkGraphicsPipelineCreateInfo pipelineCInfo = {};
	VkViewport viewport = {};
	VkRect2D scissor = {};

	std::vector<VkFramebuffer> m_framebuffers;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VKTextureDataComponent*> m_VKTDCs;
	VKTextureDataComponent* m_depthVKTDC;

	VkSubmitInfo submitInfo;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	size_t m_maxFramesInFlight = 1;
	size_t m_currentFrame = 0;
};