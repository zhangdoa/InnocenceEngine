#include "VKGraphicsService.h"
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

bool VKGraphicsService::ReserveFramebuffer(VKRenderPassComponent* VKRenderPassComp)
{
	auto l_framebufferCount = GetSwapChainImageCount();
	VKRenderPassComp->m_Framebuffers.reserve(l_framebufferCount);
	for (size_t i = 0; i < l_framebufferCount; i++)
	{
		VKRenderPassComp->m_Framebuffers.emplace_back();
	}
	return true;
}

bool VKGraphicsService::CreateRenderPass(VKRenderPassComponent *VKRenderPassComp, VkFormat* overrideFormat)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_RenderPassCInfo = {};
	l_PSO->m_RenderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	l_PSO->m_RenderPassCInfo.subpassCount = 1;

	l_PSO->m_SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (VKRenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
	{
		size_t colorAttachmentCount = VKRenderPassComp->m_RenderPassDesc.m_UseMultiFrames ? 1 : VKRenderPassComp->m_RenderPassDesc.m_RenderTargetCount;
		l_PSO->m_ColorAttachmentRefs.reserve(colorAttachmentCount);

		for (size_t i = 0; i < colorAttachmentCount; i++)
		{
			VkAttachmentReference l_colorAttachmentRef = {};
			l_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_colorAttachmentRef.attachment = (uint32_t)i;

			l_PSO->m_ColorAttachmentRefs.emplace_back(l_colorAttachmentRef);
		}

		VkAttachmentDescription l_colorAttachmentDesc = {};
		l_colorAttachmentDesc.format = overrideFormat ? *overrideFormat : GetTextureFormat(VKRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc);
		l_colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		l_colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		l_colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		l_colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		for (size_t i = 0; i < colorAttachmentCount; i++)
		{
			l_PSO->m_AttachmentDescs.emplace_back(l_colorAttachmentDesc);
		}

		if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			l_PSO->m_DepthAttachmentRef.attachment = (uint32_t)colorAttachmentCount;
			l_PSO->m_DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription l_depthAttachmentDesc = {};

			if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable)
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D24_UNORM_S8_UINT;
			}
			else
			{
				l_depthAttachmentDesc.format = VK_FORMAT_D32_SFLOAT;
			}
			l_depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			l_depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			l_depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			l_depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			l_PSO->m_AttachmentDescs.emplace_back(l_depthAttachmentDesc);
		}

		l_PSO->m_SubpassDesc.colorAttachmentCount = (uint32_t)colorAttachmentCount;
		l_PSO->m_RenderPassCInfo.attachmentCount = (uint32_t)l_PSO->m_AttachmentDescs.size();

		if (l_PSO->m_SubpassDesc.colorAttachmentCount)
		{
			l_PSO->m_SubpassDesc.pColorAttachments = &l_PSO->m_ColorAttachmentRefs[0];
		}

		if (l_PSO->m_DepthAttachmentRef.attachment)
		{
			l_PSO->m_SubpassDesc.pDepthStencilAttachment = &l_PSO->m_DepthAttachmentRef;
		}

		if (l_PSO->m_RenderPassCInfo.attachmentCount)
		{
			l_PSO->m_RenderPassCInfo.pAttachments = &l_PSO->m_AttachmentDescs[0];
		}
	}

	l_PSO->m_RenderPassCInfo.pSubpasses = &l_PSO->m_SubpassDesc;

	l_PSO->m_SubpassDeps.resize(4);

	l_PSO->m_SubpassDeps[0].srcSubpass = 0;
	l_PSO->m_SubpassDeps[0].dstSubpass = 0;
	l_PSO->m_SubpassDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	l_PSO->m_SubpassDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_SubpassDeps[1].srcSubpass = 0;
	l_PSO->m_SubpassDeps[1].dstSubpass = 0;
	l_PSO->m_SubpassDeps[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	l_PSO->m_SubpassDeps[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_SubpassDeps[2].srcSubpass = 0;
	l_PSO->m_SubpassDeps[2].dstSubpass = 0;
	l_PSO->m_SubpassDeps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	l_PSO->m_SubpassDeps[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	l_PSO->m_SubpassDeps[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_SubpassDeps[3].srcSubpass = VK_SUBPASS_EXTERNAL;
	l_PSO->m_SubpassDeps[3].dstSubpass = 0;
	l_PSO->m_SubpassDeps[3].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	l_PSO->m_SubpassDeps[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	l_PSO->m_SubpassDeps[3].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	l_PSO->m_SubpassDeps[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	l_PSO->m_RenderPassCInfo.dependencyCount = l_PSO->m_SubpassDeps.size();
	l_PSO->m_RenderPassCInfo.pDependencies = &l_PSO->m_SubpassDeps[0];

	if (vkCreateRenderPass(m_device, &l_PSO->m_RenderPassCInfo, nullptr, &l_PSO->m_RenderPass) != VK_SUCCESS)
	{
		Log(Error, "", VKRenderPassComp->m_InstanceName.c_str(), " failed to create VkRenderPass!");
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	SetObjectName(VKRenderPassComp, l_PSO->m_RenderPass, VK_OBJECT_TYPE_RENDER_PASS, "RenderPass");
#endif //  INNO_DEBUG

	Log(Verbose, "VkRenderPass has been created for ", VKRenderPassComp->m_InstanceName.c_str());
	return true;
}

bool VKGraphicsService::CreateViewportAndScissor(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	l_PSO->m_Viewport.width = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width;
	l_PSO->m_Viewport.height = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height;
	l_PSO->m_Viewport.maxDepth = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MaxDepth;
	l_PSO->m_Viewport.minDepth = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MinDepth;
	l_PSO->m_Viewport.x = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginX;
	l_PSO->m_Viewport.y = VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_OriginY;

	l_PSO->m_Scissor.offset = {0, 0};
	l_PSO->m_Scissor.extent.width = (uint32_t)l_PSO->m_Viewport.width;
	l_PSO->m_Scissor.extent.height = (uint32_t)l_PSO->m_Viewport.height;

	return true;
}

bool VKGraphicsService::CreateFramebuffers(VKRenderPassComponent *VKRenderPassComp)
{
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(VKRenderPassComp->m_PipelineStateObject);

	VkFramebufferCreateInfo l_framebufferCInfo = {};
	l_framebufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	l_framebufferCInfo.renderPass = l_PSO->m_RenderPass;
	l_framebufferCInfo.width = (uint32_t)l_PSO->m_Viewport.width;
	l_framebufferCInfo.height = (uint32_t)l_PSO->m_Viewport.height;
	l_framebufferCInfo.layers = 1;

	auto l_outputMergerTarget = VKRenderPassComp->m_OutputMergerTarget;
	auto l_attachmentCount = l_outputMergerTarget->m_ColorOutputs.size();

	if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		l_attachmentCount += 1;

	auto l_swapChainImageCount = GetSwapChainImageCount();
	for (size_t i = 0; i < l_swapChainImageCount; i++)
	{
		std::vector<VkImageView> l_attachments(l_attachmentCount);

		if (VKRenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
		{
			for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
			{
				auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_ColorOutputs[j]);

				l_attachments[j] = l_VKTextureComp->m_imageView;
			}
		}
		else
		{
			l_framebufferCInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
		}

		if (VKRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
			l_attachments[l_attachmentCount - 1] = l_VKTextureComp->m_imageView;
		}

		l_framebufferCInfo.attachmentCount = (uint32_t)l_attachments.size();
		l_framebufferCInfo.pAttachments = &l_attachments[0];
		if (vkCreateFramebuffer(m_device, &l_framebufferCInfo, nullptr, &VKRenderPassComp->m_Framebuffers[i]) != VK_SUCCESS)
		{
			Log(Error, "", VKRenderPassComp->m_InstanceName.c_str(), " failed to create VkFramebuffer!");
			continue;
		}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		auto l_name = "FrameBuffer_" + std::to_string(i);
		SetObjectName(VKRenderPassComp, VKRenderPassComp->m_Framebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, l_name.c_str());
	#endif //  INNO_DEBUG
	}

	Log(Verbose, "VkFramebuffers have been created for ", VKRenderPassComp->m_InstanceName.c_str());

	return true;
}
