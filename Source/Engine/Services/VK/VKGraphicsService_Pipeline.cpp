#include "VKGraphicsService.h"
#include "../../Common/Array.h"
#include "../GraphicsResourceService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::CreatePipelineLayout(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_PipelineLayoutCInfo = {};
	l_PSO->m_PipelineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	l_PSO->m_PipelineLayoutCInfo.setLayoutCount = static_cast<uint32_t>(VKRenderPassComp->m_DescriptorSetLayouts.size());
	l_PSO->m_PipelineLayoutCInfo.pSetLayouts = &VKRenderPassComp->m_DescriptorSetLayouts[0];

	if (VKRenderPassComp->m_PushConstantRanges.size() > 0)
	{
		l_PSO->m_PipelineLayoutCInfo.pushConstantRangeCount = static_cast<uint32_t>(VKRenderPassComp->m_PushConstantRanges.size());
		l_PSO->m_PipelineLayoutCInfo.pPushConstantRanges = VKRenderPassComp->m_PushConstantRanges.data();
	}

	if (vkCreatePipelineLayout(m_device, &l_PSO->m_PipelineLayoutCInfo, nullptr, &l_PSO->m_PipelineLayout) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipelineLayout!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_PipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "PipelineLayout");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipelineLayout has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKGraphicsService::CreateGraphicsPipelines(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);
	size_t colorAttachmentCount = VKRenderPassComp->m_RenderPassDesc.m_UseMultiFrames ? 1 : VKRenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

	GenerateViewportState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);
	GenerateRasterizerState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
	GenerateDepthStencilState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
	GenerateBlendState(VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, colorAttachmentCount, l_PSO);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRenderPassComp->m_ShaderProgram);
	Inno::Array<VkPipelineShaderStageCreateInfo> l_shaderStageCInfos;
	l_shaderStageCInfos.reserve(6);

	if (l_VKSPC->m_ShaderFilePaths.m_VSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_VSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_HSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_HSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_DSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_DSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_GSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_GSCInfo);
	}
	if (l_VKSPC->m_ShaderFilePaths.m_PSPath != "")
	{
		l_shaderStageCInfos.emplace_back(l_VKSPC->m_PSCInfo);
	}

	l_PSO->m_GraphicsPipelineCInfo = {};
	l_PSO->m_GraphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	l_PSO->m_GraphicsPipelineCInfo.stageCount = (uint32_t)l_shaderStageCInfos.size();
	l_PSO->m_GraphicsPipelineCInfo.pStages = &l_shaderStageCInfos[0];
	l_PSO->m_GraphicsPipelineCInfo.pVertexInputState = &l_VKSPC->m_vertexInputStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pInputAssemblyState = &l_PSO->m_InputAssemblyStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pViewportState = &l_PSO->m_ViewportStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pRasterizationState = &l_PSO->m_RasterizationStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pMultisampleState = &l_PSO->m_MultisampleStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pDepthStencilState = &l_PSO->m_DepthStencilStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.pColorBlendState = &l_PSO->m_ColorBlendStateCInfo;
	l_PSO->m_GraphicsPipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_GraphicsPipelineCInfo.renderPass = l_PSO->m_RenderPass;
	l_PSO->m_GraphicsPipelineCInfo.subpass = 0;
	l_PSO->m_GraphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &l_PSO->m_GraphicsPipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipeline for GraphicsPipeline!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_Pipeline, VK_OBJECT_TYPE_PIPELINE, "GraphicsPipeline");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipeline for GraphicsPipeline has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKGraphicsService::CreateComputePipelines(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	// attach shader module and create pipeline
	auto l_VKSPC = reinterpret_cast<VKShaderProgramComponent *>(VKRenderPassComp->m_ShaderProgram);

	l_PSO->m_ComputePipelineCInfo = {};
	l_PSO->m_ComputePipelineCInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	l_PSO->m_ComputePipelineCInfo.stage = l_VKSPC->m_CSCInfo;
	l_PSO->m_ComputePipelineCInfo.layout = l_PSO->m_PipelineLayout;
	l_PSO->m_ComputePipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &l_PSO->m_ComputePipelineCInfo, nullptr, &l_PSO->m_Pipeline) != VK_SUCCESS)
	{
		Log(Error, "Failed to create VkPipeline for ComputePipeline!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_Pipeline, VK_OBJECT_TYPE_PIPELINE, "ComputePipeline");
#endif //  INNO_DEBUG

	Log(Verbose, "VkPipeline for ComputePipeline has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKGraphicsService::CreateCommandBuffers(VKRenderPassComponent *VKRenderPassComp)
{
	// In the new architecture, command buffers are allocated dynamically when CommandListBegin is called
	// This function now only needs to ensure the command pools are ready
	// The actual command buffer allocation happens in AddCommandList() and CommandListBegin()

	// Command pools should already be created in CreateCommandPools()
	if (VKRenderPassComp->m_GraphicsCommandPool == VK_NULL_HANDLE)
	{
		Log(Error, "Graphics command pool not created for render pass ", VKRenderPassComp->m_InstanceName.c_str());
		return false;
	}

	if (VKRenderPassComp->m_ComputeCommandPool == VK_NULL_HANDLE)
	{
		Log(Error, "Compute command pool not created for render pass ", VKRenderPassComp->m_InstanceName.c_str());
		return false;
	}

	Log(Verbose, "Command pools are ready for dynamic command buffer allocation for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKGraphicsService::CreateSyncPrimitives(VKRenderPassComponent *VKRenderPassComp)
{
	VkSemaphoreTypeCreateInfo l_timelineCreateInfo = {};
	l_timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	l_timelineCreateInfo.pNext = NULL;
	l_timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	l_timelineCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo l_semaphoreInfo;
	l_semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	l_semaphoreInfo.pNext = &l_timelineCreateInfo;
	l_semaphoreInfo.flags = 0;

	VKRenderPassComp->m_SubmitInfo = {};

	for (size_t i = 0; i < VKRenderPassComp->m_Semaphores.size(); i++)
	{
		auto l_VKSemaphore = reinterpret_cast<VKSemaphore *>(VKRenderPassComp->m_Semaphores[i]);

		if (vkCreateSemaphore(m_device, &l_semaphoreInfo, nullptr, &l_VKSemaphore->m_GraphicsSemaphore) != VK_SUCCESS)
		{
			Log(Error, "Failed to create Graphics semaphore!");
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	auto l_graphicsName = "GraphicsSemaphore_" + std::to_string(i);
	SetObjectName(VKRenderPassComp, l_VKSemaphore->m_GraphicsSemaphore, VK_OBJECT_TYPE_SEMAPHORE, l_graphicsName.c_str());
#endif //  INNO_DEBUG

		if (vkCreateSemaphore(m_device, &l_semaphoreInfo, nullptr, &l_VKSemaphore->m_ComputeSemaphore) != VK_SUCCESS)
		{
			Log(Error, "Failed to create Compute semaphore!");
			return false;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	auto l_computeName = "ComputeSemaphore_" + std::to_string(i);
	SetObjectName(VKRenderPassComp, l_VKSemaphore->m_ComputeSemaphore, VK_OBJECT_TYPE_SEMAPHORE, l_computeName.c_str());
#endif //  INNO_DEBUG
	}

	Log(Verbose, "Synchronization primitives has been created for ", VKRenderPassComp->m_InstanceName.c_str());

	return true;
}
