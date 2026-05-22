#pragma once
#include "../Common/Array.h"
#include "RenderPassComponent.h"
#include "../Services/VK/VKHeaders.h"
#include "VKTextureComponent.h"

namespace Inno
{
	class VKPipelineStateObject : public IPipelineStateObject
	{
	public:
		Inno::Array<VkAttachmentReference> m_ColorAttachmentRefs = {};
		VkAttachmentReference m_DepthAttachmentRef = {};
		VkSubpassDescription m_SubpassDesc = {};
		Inno::Array<VkSubpassDependency> m_SubpassDeps = {};
		Inno::Array<VkAttachmentDescription> m_AttachmentDescs;
		VkRenderPassCreateInfo m_RenderPassCInfo = {};

		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;
		VkVertexInputBindingDescription m_VertexBindingDescription;
		Inno::Array<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions;
		VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCInfo = {};
		VkPipelineViewportStateCreateInfo m_ViewportStateCInfo = {};
		VkPipelineRasterizationStateCreateInfo m_RasterizationStateCInfo = {};
		VkPipelineMultisampleStateCreateInfo m_MultisampleStateCInfo = {};
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateCInfo = {};
		Inno::Array<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates = {};
		VkPipelineColorBlendStateCreateInfo m_ColorBlendStateCInfo = {};
		VkPipelineLayoutCreateInfo m_PipelineLayoutCInfo = {};
		VkGraphicsPipelineCreateInfo m_GraphicsPipelineCInfo = {};
		VkComputePipelineCreateInfo m_ComputePipelineCInfo = {};
		VkViewport m_Viewport = {};
		VkRect2D m_Scissor = {};
	};

	class VKSemaphore : public ISemaphore
	{
	public:
		VkSemaphore m_GraphicsSemaphore;
		VkSemaphore m_ComputeSemaphore;
		uint64_t m_GraphicsWaitValue = 0;
		uint64_t m_GraphicsSignalValue = 0;
		uint64_t m_ComputeWaitValue = 0;
		uint64_t m_ComputeSignalValue = 0;
	};

	struct VKDescriptorSetLayoutBindingIndex
	{
		size_t m_SetIndex = 0;
		size_t m_LayoutBindingOffset = 0;
		size_t m_BindingCount = 0;
	};

	class VKRenderPassComponent : public RenderPassComponent
	{
	public:
		Inno::Array<VkFramebuffer> m_Framebuffers;

		VkDescriptorPool m_DescriptorPool;
		Inno::Array<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
		Inno::Array<VKDescriptorSetLayoutBindingIndex> m_DescriptorSetLayoutBindingIndices;
		Inno::Array<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		Inno::Array<VkDescriptorSet> m_DescriptorSets;
		Inno::Array<VkPushConstantRange> m_PushConstantRanges;

		VkCommandPool m_GraphicsCommandPool;
		VkCommandPool m_ComputeCommandPool;
		VkSubmitInfo m_SubmitInfo;
	};
}