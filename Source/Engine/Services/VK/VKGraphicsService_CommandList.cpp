#include "VKGraphicsService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent*>(renderPass);

	if (!commandList)
		return false;

	commandList->m_Type = l_rhs->m_RenderPassDesc.m_GPUEngineType;

	VkCommandPool l_commandPool;
	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandPool = l_rhs->m_ComputeCommandPool;
	}
	else
	{
		l_commandPool = l_rhs->m_GraphicsCommandPool;
	}

	VkCommandBufferAllocateInfo l_allocInfo = {};
	l_allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	l_allocInfo.commandPool = l_commandPool;
	l_allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	l_allocInfo.commandBufferCount = 1;

	VkCommandBuffer l_vkCommandBuffer;
	if (vkAllocateCommandBuffers(m_device, &l_allocInfo, &l_vkCommandBuffer) != VK_SUCCESS)
	{
		Log(Error, "Failed to allocate command buffer!");
		return false;
	}

	commandList->m_CommandList = reinterpret_cast<uint64_t>(l_vkCommandBuffer);

	VkCommandBufferBeginInfo l_beginInfo = {};
	l_beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	l_beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(l_vkCommandBuffer, &l_beginInfo) != VK_SUCCESS)
	{
		Log(Error, "Failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool VKGraphicsService::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	ChangeRenderTargetStates(l_rhs, commandList, Accessibility::ReadOnly, Accessibility::WriteOnly);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		VkRenderPassBeginInfo l_renderPassBeginInfo = {};
		l_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		l_renderPassBeginInfo.renderPass = l_PSO->m_RenderPass;
		l_renderPassBeginInfo.framebuffer = l_rhs->m_Framebuffers[l_rhs->m_CurrentFrame];
		l_renderPassBeginInfo.renderArea.offset = {0, 0};
		l_renderPassBeginInfo.renderArea.extent = l_PSO->m_Scissor.extent;

		VkClearValue l_clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

		std::vector<VkClearValue> l_clearValues;

		for (size_t i = 0; i < l_rhs->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			l_clearValues.emplace_back(l_clearColor);
		}

		if (l_rhs->m_RenderPassDesc.m_UseDepthBuffer)
		{
			l_clearValues.emplace_back();
			l_clearValues[l_rhs->m_RenderPassDesc.m_RenderTargetCount].depthStencil = {1.0f, 0};
		}

		l_renderPassBeginInfo.clearValueCount = (uint32_t)l_clearValues.size();
		if (l_clearValues.size())
		{
			l_renderPassBeginInfo.pClearValues = &l_clearValues[0];
		}

		vkCmdBeginRenderPass(l_vkCommandBuffer, &l_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(l_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_PSO->m_Pipeline);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		vkCmdBindPipeline(l_vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_PSO->m_Pipeline);
	}

	ChangeRenderTargetStates(l_rhs, commandList, Accessibility::WriteOnly, Accessibility::ReadOnly);

	return true;
}

bool VKGraphicsService::ClearRenderTargets(RenderPassComponent *rhs, CommandListComponent* commandList, size_t index)
{
	return true;
}

bool VKGraphicsService::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (resource == nullptr)
	{
		Log(Warning, "Empty GPU resource in render pass: ", renderPass->m_InstanceName.c_str(), ", at: ", resourceBindingLayoutDescIndex);
		return false;
	}

	auto l_renderPass = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_bindingPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	if (l_renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_bindingPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	}
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_renderPass->m_PipelineStateObject);
	auto l_commandBuffer = l_vkCommandBuffer;

	VkWriteDescriptorSet l_writeDescriptorSet = {};
	VkDescriptorImageInfo l_descriptorImageInfo = {};
	VkDescriptorBufferInfo l_descriptorBufferInfo = {};
	auto l_descriptorSetIndex = (uint32_t)l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_DescriptorSetIndex;
	auto l_descriptorIndex = (uint32_t)l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_DescriptorIndex;
	auto accessibility = l_renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_BindingAccessibility;
	
	switch (resource->m_GPUResourceType)
	{
	case GPUResourceType::Sampler:
	{
		l_descriptorImageInfo.sampler = reinterpret_cast<VKSamplerComponent *>(resource)->m_sampler;
		l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_SAMPLER, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
		UpdateDescriptorSet(&l_writeDescriptorSet, 1);
		break;
	}
	case GPUResourceType::Image:
	{
		auto l_VKTextureComp = reinterpret_cast<VKTextureComponent *>(resource);

		l_descriptorImageInfo.imageView = l_VKTextureComp->m_imageView;
		if (accessibility != Accessibility::ReadOnly)
		{
			l_descriptorImageInfo.imageLayout = l_VKTextureComp->m_WriteImageLayout;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
		}
		else
		{
			l_descriptorImageInfo.imageLayout = l_VKTextureComp->m_ReadImageLayout;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
		}
		UpdateDescriptorSet(&l_writeDescriptorSet, 1);
		break;
	}
	case GPUResourceType::Buffer:
		if (resource->m_GPUAccessibility == Accessibility::ReadOnly)
		{
			if (accessibility != Accessibility::ReadOnly)
			{
				Log(Warning, "Not allow GPU write to Constant Buffer!");
			}
			else
			{
				l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferComponent *>(resource)->m_HostStagingBuffer;
				l_descriptorBufferInfo.offset = startOffset;
				l_descriptorBufferInfo.range = elementCount;
				l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
				UpdateDescriptorSet(&l_writeDescriptorSet, 1);
			}
		}
		else
		{
			VkDescriptorBufferInfo l_descriptorBufferInfo = {};
			l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferComponent *>(resource)->m_DeviceLocalBuffer;
			l_descriptorBufferInfo.offset = startOffset;
			l_descriptorBufferInfo.range = elementCount;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
			UpdateDescriptorSet(&l_writeDescriptorSet, 1);
		}
		break;
	default:
		break;
	}

	vkCmdBindDescriptorSets(l_commandBuffer,
							l_bindingPoint,
							l_PSO->m_PipelineLayout,
							l_descriptorSetIndex,
							1,
							&l_renderPass->m_DescriptorSets[l_descriptorSetIndex], 0, nullptr);

	return true;
}

void VKGraphicsService::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
}

bool VKGraphicsService::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		vkCmdEndRenderPass(l_vkCommandBuffer);
	}

	if (vkEndCommandBuffer(l_vkCommandBuffer) != VK_SUCCESS)
	{
		Log(Error, "Failed to end recording command buffer!");
		return false;
	}

	return true;
}

bool VKGraphicsService::GenerateMipmap(TextureComponent *rhs, CommandListComponent* commandList)
{
	return true;
}
