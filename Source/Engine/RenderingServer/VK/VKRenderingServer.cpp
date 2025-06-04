#include "VKRenderingServer.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/EntityManager.h"

bool VKRenderingServer::Clear(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Copy(TextureComponent *lhs, TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Clear(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent *>(rhs);

	auto l_commandBuffer = OpenTemporaryCommandBuffer(m_globalCommandPool);

	vkCmdFillBuffer(l_commandBuffer, l_rhs->m_HostStagingBuffer, 0, 0, 0);
	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		vkCmdFillBuffer(l_commandBuffer, l_rhs->m_DeviceLocalBuffer, 0, 0, 0);
	}

	CloseTemporaryCommandBuffer(m_globalCommandPool, m_graphicsQueue, l_commandBuffer);

	return true;
}

bool VKRenderingServer::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	if (queueType == GPUEngineType::Graphics)
	{
		vkWaitForFences(m_device, 1, &m_graphicsQueueFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	}
	else if (queueType == GPUEngineType::Compute)
	{
		vkWaitForFences(m_device, 1, &m_computeQueueFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	}

	return true;
}

std::optional<uint32_t> VKRenderingServer::GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility)
{
	// Vulkan texture indexing not implemented yet
	return std::nullopt;
}

bool VKRenderingServer::CommandListBegin(RenderPassComponent *rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	VkCommandBufferBeginInfo l_beginInfo = {};
	l_beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	l_beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	auto l_commandBuffer = l_commandList->m_GraphicsCommandBuffer;
	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandBuffer = l_commandList->m_ComputeCommandBuffer;
	}

	if (vkBeginCommandBuffer(l_commandBuffer, &l_beginInfo) != VK_SUCCESS)
	{
		Log(Error, "Failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::BindRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	ChangeRenderTargetStates(l_rhs, l_commandList, Accessibility::WriteOnly);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		VkRenderPassBeginInfo l_renderPassBeginInfo = {};
		l_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		l_renderPassBeginInfo.renderPass = l_PSO->m_RenderPass;
		l_renderPassBeginInfo.framebuffer = l_rhs->m_Framebuffers[l_rhs->m_CurrentFrame];
		l_renderPassBeginInfo.renderArea.offset = {0, 0};
		l_renderPassBeginInfo.renderArea.extent = l_PSO->m_Scissor.extent;

		// @TODO: do not clear the buffers here
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

		vkCmdBeginRenderPass(l_commandList->m_GraphicsCommandBuffer, &l_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(l_commandList->m_GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_PSO->m_Pipeline);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		vkCmdBindPipeline(l_commandList->m_ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_PSO->m_Pipeline);
	}

	ChangeRenderTargetStates(l_rhs, l_commandList, Accessibility::ReadOnly);

	return true;
}

bool VKRenderingServer::ClearRenderTargets(RenderPassComponent *rhs, size_t index)
{
	return true;
}

bool VKRenderingServer::BindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	if (resource == nullptr)
	{
		Log(Warning, "Empty GPU resource in render pass: ", renderPass->m_InstanceName.c_str(), ", at: ", resourceBindingLayoutDescIndex);
		return false;
	}

	auto l_renderPass = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_bindingPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	if (l_renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_bindingPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	}
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_renderPass->m_PipelineStateObject);
	auto l_commandBuffer = l_commandList->m_GraphicsCommandBuffer;
	if (l_renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandBuffer = l_commandList->m_ComputeCommandBuffer;
	}

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
			// if (accessibility != Accessibility::ReadOnly)
			//{
			VkDescriptorBufferInfo l_descriptorBufferInfo = {};
			l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferComponent *>(resource)->m_DeviceLocalBuffer;
			l_descriptorBufferInfo.offset = startOffset;
			l_descriptorBufferInfo.range = elementCount;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
			UpdateDescriptorSet(&l_writeDescriptorSet, 1);
			// }
			// else
			// {
			// }
		}
		break;
	default:
		break;
	}

	if (resource->m_GPUResourceType == GPUResourceType::Image)
	{
		auto l_VKTextureComp = reinterpret_cast<VKTextureComponent *>(resource);

		l_descriptorImageInfo.imageView = l_VKTextureComp->m_imageView;
		if (accessibility != Accessibility::ReadOnly)
		{
			//TryToTransitImageLayout(l_VKTextureComp, l_commandBuffer, VK_IMAGE_LAYOUT_GENERAL, shaderStage);
		}
		else
		{
			//TryToTransitImageLayout(l_VKTextureComp, l_commandBuffer, l_VKTextureComp->m_ReadImageLayout, shaderStage);
		}
	}

	vkCmdBindDescriptorSets(l_commandBuffer,
							l_bindingPoint,
							l_PSO->m_PipelineLayout,
							l_descriptorSetIndex,
							1,
							&l_renderPass->m_DescriptorSets[l_descriptorSetIndex], 0, nullptr);

	return true;
}

void VKRenderingServer::PushRootConstants(RenderPassComponent* rhs, size_t rootConstants)
{
}

bool VKRenderingServer::DrawIndexedInstanced(RenderPassComponent *renderPass, MeshComponent *mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_mesh = reinterpret_cast<VKMeshComponent *>(mesh);

	VkBuffer vertexBuffers[] = {l_mesh->m_VBO};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(l_commandList->m_GraphicsCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(l_commandList->m_GraphicsCommandBuffer, l_mesh->m_IBO, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(l_commandList->m_GraphicsCommandBuffer, static_cast<uint32_t>(l_mesh->m_IndexCount), 1, 0, 0, 0);

	return true;
}

bool VKRenderingServer::DrawInstanced(RenderPassComponent *renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	vkCmdDraw(l_commandList->m_GraphicsCommandBuffer, 1, static_cast<uint32_t>(instanceCount), 0, 0);

	return true;
}

bool VKRenderingServer::UnbindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool VKRenderingServer::CommandListEnd(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		vkCmdEndRenderPass(l_commandList->m_GraphicsCommandBuffer);
	}

	auto l_commandBuffer = l_commandList->m_GraphicsCommandBuffer;
	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandBuffer = l_commandList->m_ComputeCommandBuffer;
	}

	if (vkEndCommandBuffer(l_commandBuffer) != VK_SUCCESS)
	{
		Log(Error, "Failed to end recording command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::ExecuteCommandList(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != GPUEngineType)
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<VKSemaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	VkTimelineSemaphoreSubmitInfo timelineInfo = {};
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 1;
	timelineInfo.signalSemaphoreValueCount = 1;

	VkSubmitInfo l_submitInfo = {};
	l_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	l_submitInfo.pNext = &timelineInfo;
	l_submitInfo.commandBufferCount = 1;
	l_submitInfo.signalSemaphoreCount = 1;
	l_submitInfo.waitSemaphoreCount = 1;

	auto l_commandBuffer = l_commandList->m_GraphicsCommandBuffer;
	VkQueue &queue = m_graphicsQueue;
	VkFence &fence = m_graphicsQueueFence;
	VkPipelineStageFlags waitDstStageMask[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
	l_submitInfo.pWaitDstStageMask = &waitDstStageMask[0];

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_semaphore->m_GraphicsWaitValue = l_semaphore->m_GraphicsSignalValue;
		l_semaphore->m_GraphicsSignalValue = l_semaphore->m_GraphicsWaitValue + 1;
		timelineInfo.pWaitSemaphoreValues = &l_semaphore->m_GraphicsWaitValue;
		timelineInfo.pSignalSemaphoreValues = &l_semaphore->m_GraphicsSignalValue;
		l_submitInfo.pSignalSemaphores = &l_semaphore->m_GraphicsSemaphore;
		l_submitInfo.pWaitSemaphores = &l_semaphore->m_GraphicsSemaphore;
		l_submitInfo.pCommandBuffers = &l_commandList->m_GraphicsCommandBuffer;
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_semaphore->m_ComputeWaitValue = l_semaphore->m_ComputeSignalValue;
		l_semaphore->m_ComputeSignalValue = l_semaphore->m_ComputeWaitValue + 1;
		timelineInfo.pWaitSemaphoreValues = &l_semaphore->m_ComputeWaitValue;
		timelineInfo.pSignalSemaphoreValues = &l_semaphore->m_ComputeSignalValue;
		l_submitInfo.pSignalSemaphores = &l_semaphore->m_ComputeSemaphore;
		l_submitInfo.pWaitSemaphores = &l_semaphore->m_ComputeSemaphore;
		l_submitInfo.pCommandBuffers = &l_commandList->m_ComputeCommandBuffer;

		queue = m_computeQueue;
		fence = m_computeQueueFence;

		waitDstStageMask[0] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}

	vkResetFences(m_device, 1, &fence);
	if (vkQueueSubmit(queue, 1, &l_submitInfo, fence) != VK_SUCCESS)
	{
		Log(Error, "Failed to submit command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::WaitOnGPU(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != semaphoreType)
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<VKSemaphore *>(l_rhs->m_Semaphores[l_rhs->m_CurrentFrame]);

	VkSemaphoreWaitInfo waitInfo = {};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.semaphoreCount = 1;
	if (semaphoreType == GPUEngineType::Graphics)
	{
		waitInfo.pSemaphores = &l_semaphore->m_GraphicsSemaphore;
		waitInfo.pValues = &l_semaphore->m_GraphicsSignalValue;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		waitInfo.pSemaphores = &l_semaphore->m_ComputeSemaphore;
		waitInfo.pValues = &l_semaphore->m_ComputeSignalValue;
	}

	vkWaitSemaphores(m_device, &waitInfo, std::numeric_limits<uint64_t>::max());

	return true;
}

bool VKRenderingServer::TryToTransitState(TextureComponent *rhs, ICommandList *commandList, Accessibility accessibility)
{
	auto l_rhs = reinterpret_cast<VKTextureComponent *>(rhs);
	auto l_newImageLayout = accessibility == Accessibility::ReadOnly ? l_rhs->m_ReadImageLayout : l_rhs->m_WriteImageLayout;
	if (l_rhs->m_CurrentImageLayout == l_newImageLayout)
		return false;

	auto l_commandBuffer = reinterpret_cast<VKCommandList *>(commandList)->m_GraphicsCommandBuffer;
	auto l_shaderStage = ShaderStage::Invalid; // @TODO: Nope this is not correct
	TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, l_rhs->m_CurrentImageLayout, l_newImageLayout, l_shaderStage);
	l_rhs->m_CurrentImageLayout = l_newImageLayout;
	
	return true;
}

bool VKRenderingServer::PresentImpl()
{
	// acquire an image from swap chain
	thread_local uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_device,
		m_swapChain,
		std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	// l_semaphore->m_GraphicsWaitValue = l_semaphore->m_GraphicsSignalValue;
	// l_semaphore->m_GraphicsSignalValue = l_semaphore->m_GraphicsWaitValue + 1;

	// const uint64_t signalSemaphoreValues[2] = {
	// 	l_semaphore->m_GraphicsSignalValue,
	// 	0 // ignored for the swapchain
	// };
	// const VkSemaphore signalSemaphores[2] = {
	// 	l_semaphore->m_GraphicsSemaphore,
	// 	m_swapChainRenderedSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame]};

	// VkTimelineSemaphoreSubmitInfo timelineInfo = {};
	// timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	// timelineInfo.pNext = NULL;
	// timelineInfo.signalSemaphoreValueCount = 2;
	// timelineInfo.pSignalSemaphoreValues = signalSemaphoreValues;

	// VkSubmitInfo l_submitInfo = {};
	// l_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	// l_submitInfo.pNext = &timelineInfo;
	// l_submitInfo.commandBufferCount = 1;
	// l_submitInfo.pCommandBuffers = &l_commandList->m_GraphicsCommandBuffer;
	// l_submitInfo.waitSemaphoreCount = 1;
	// l_submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame];
	// l_submitInfo.signalSemaphoreCount = 2;
	// l_submitInfo.pSignalSemaphores = &signalSemaphores[0];

	// VkPipelineStageFlags waitDstStageMask[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
	// l_submitInfo.pWaitDstStageMask = &waitDstStageMask[0];

	// VkQueue &queue = m_graphicsQueue;
	// VkFence &fence = m_graphicsQueueFence;

	// vkResetFences(m_device, 1, &fence);
	// if (vkQueueSubmit(queue, 1, &l_submitInfo, fence) != VK_SUCCESS)
	// {
	// 	Log(Error, "Failed to submit command buffer for the swap chain RenderPassComp!");
	// 	return false;
	// }

	// // present the swap chain image to the front screen
	// VkTimelineSemaphoreSubmitInfo swapChainTimelineInfo = {};
	// swapChainTimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	// swapChainTimelineInfo.pNext = NULL;
	// swapChainTimelineInfo.waitSemaphoreValueCount = 1;
	// swapChainTimelineInfo.pWaitSemaphoreValues = &l_semaphore->m_GraphicsWaitValue;

	// VkPresentInfoKHR presentInfo = {};
	// presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	// presentInfo.waitSemaphoreCount = 1;
	// presentInfo.pWaitSemaphores = &m_swapChainRenderedSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame];

	// // swap chain
	// VkSwapchainKHR swapChains[] = {m_swapChain};
	// presentInfo.swapchainCount = 1;
	// presentInfo.pSwapchains = swapChains;
	// presentInfo.pImageIndices = &imageIndex;

	// vkQueuePresentKHR(m_presentQueue, &presentInfo);

	// m_SwapChainRenderPassComp->m_CurrentFrame = imageIndex;

	return true;
}

bool Inno::VKRenderingServer::EndFrame()
{
    return false;
}

bool VKRenderingServer::Dispatch(RenderPassComponent *renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	vkCmdDispatch(l_commandList->m_ComputeCommandBuffer, threadGroupX, threadGroupY, threadGroupZ);

	return true;
}

Vec4 VKRenderingServer::ReadRenderTargetSample(RenderPassComponent *rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> VKRenderingServer::ReadTextureBackToCPU(RenderPassComponent *canvas, TextureComponent *TextureComp)
{
	return std::vector<Vec4>();
}

bool VKRenderingServer::GenerateMipmap(TextureComponent *rhs, ICommandList* commandList)
{
	// Currently Vulkan GenerateMipmap is not implemented but accepts command list parameter
	// for API compatibility with DX12 implementation
	return true;
}

bool VKRenderingServer::BeginCapture()
{
    return true;
}

bool VKRenderingServer::EndCapture()
{
	return true;
}

bool VKRenderingServer::ResizeImpl()
{
	return true;
}

void *VKRenderingServer::GetVkInstance()
{
	return m_instance;
}

void *VKRenderingServer::GetVkSurface()
{
	return &m_windowSurface;
}