#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "RenderPassDataComponent.h"
#include "VKTextureDataComponent.h"

namespace Inno
{
	class VKPipelineStateObject : public IPipelineStateObject
	{
	public:
		std::vector<VkAttachmentReference> m_ColorAttachmentRefs = {};
		VkAttachmentReference m_DepthAttachmentRef = {};
		VkSubpassDescription m_SubpassDesc = {};
		std::vector<VkSubpassDependency> m_SubpassDeps = {};
		std::vector<VkAttachmentDescription> m_AttachmentDescs;
		VkRenderPassCreateInfo m_RenderPassCInfo = {};

		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;
		VkVertexInputBindingDescription m_VertexBindingDescription;
		std::vector<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions;
		VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCInfo = {};
		VkPipelineViewportStateCreateInfo m_ViewportStateCInfo = {};
		VkPipelineRasterizationStateCreateInfo m_RasterizationStateCInfo = {};
		VkPipelineMultisampleStateCreateInfo m_MultisampleStateCInfo = {};
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateCInfo = {};
		std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates = {};
		VkPipelineColorBlendStateCreateInfo m_ColorBlendStateCInfo = {};
		VkPipelineLayoutCreateInfo m_PipelineLayoutCInfo = {};
		VkGraphicsPipelineCreateInfo m_GraphicsPipelineCInfo = {};
		VkComputePipelineCreateInfo m_ComputePipelineCInfo = {};
		VkViewport m_Viewport = {};
		VkRect2D m_Scissor = {};
	};

	class VKCommandList : public ICommandList
	{
	public:
		VkCommandBuffer m_CommandBuffer;
	};

	class VKSemaphore : public ISemaphore
	{
	public:
		VkSemaphore m_Semaphore;
		uint64_t m_WaitValue = 0;
		uint64_t m_SignalValue = 0;
	};

	struct VKDescriptorSetLayoutBindingIndex
	{
		size_t m_SetIndex = 0;
		size_t m_LayoutBindingOffset = 0;
		size_t m_BindingCount = 0;
	};

	class VKRenderPassDataComponent : public RenderPassDataComponent
	{
	public:
		std::vector<VkFramebuffer> m_Framebuffers;

		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
		std::vector<VKDescriptorSetLayoutBindingIndex> m_DescriptorSetLayoutBindingIndices;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::vector<VkPushConstantRange> m_PushConstantRanges;

		VkCommandPool m_GraphicsCommandPool;
		VkCommandPool m_ComputeCommandPool;
		VkSubmitInfo m_SubmitInfo;
	};
}