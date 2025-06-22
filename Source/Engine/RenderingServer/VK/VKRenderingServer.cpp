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

std::optional<uint32_t> VKRenderingServer::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility)
{
	// Vulkan texture indexing not implemented yet
	return std::nullopt;
}

bool VKRenderingServer::CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	
	if (!commandList)
		return false;

	// Set the command list type based on render pass GPU engine type
	commandList->m_Type = l_rhs->m_RenderPassDesc.m_GPUEngineType;
	
	// Use the appropriate command pool based on GPU engine type
	VkCommandPool l_commandPool;
	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_commandPool = l_rhs->m_ComputeCommandPool;
	}
	else
	{
		l_commandPool = l_rhs->m_GraphicsCommandPool;
	}

	// Allocate command buffer from the appropriate pool
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

	// Store the command buffer in the component
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

bool VKRenderingServer::BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	ChangeRenderTargetStates(l_rhs, commandList, Accessibility::WriteOnly);

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

		vkCmdBeginRenderPass(l_vkCommandBuffer, &l_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(l_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_PSO->m_Pipeline);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		vkCmdBindPipeline(l_vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_PSO->m_Pipeline);
	}

	ChangeRenderTargetStates(l_rhs, commandList, Accessibility::ReadOnly);

	return true;
}

bool VKRenderingServer::ClearRenderTargets(RenderPassComponent *rhs, CommandListComponent* commandList, size_t index)
{
	return true;
}

bool VKRenderingServer::BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
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

void VKRenderingServer::PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants)
{
}

bool VKRenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_mesh = reinterpret_cast<VKMeshComponent *>(mesh);

	VkBuffer vertexBuffers[] = {l_mesh->m_VBO};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(l_vkCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(l_vkCommandBuffer, l_mesh->m_IBO, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdDrawIndexed(l_vkCommandBuffer, static_cast<uint32_t>(l_mesh->m_IndexCount), 1, 0, 0, 0);

	return true;
}

bool VKRenderingServer::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

	vkCmdDraw(l_vkCommandBuffer, 1, static_cast<uint32_t>(instanceCount), 0, 0);

	return true;
}

bool VKRenderingServer::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	// Vulkan ExecuteIndirect not implemented yet
	// TODO: Implement vkCmdDrawIndirect functionality
	return true;
}

bool VKRenderingServer::UnbindGPUResource(RenderPassComponent *renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool VKRenderingServer::CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList)
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

bool VKRenderingServer::Execute(CommandListComponent* commandList, GPUEngineType GPUEngineType)
{
	if (!commandList)
		return false;
	
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

	// Simplified execution without semaphore management for now
	// TODO: Implement proper semaphore management for dynamic command lists
	VkSubmitInfo l_submitInfo = {};
	l_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	l_submitInfo.commandBufferCount = 1;

	VkQueue &queue = m_graphicsQueue;
	VkFence &fence = m_graphicsQueueFence;
	
	l_submitInfo.pCommandBuffers = &l_vkCommandBuffer;

	if (GPUEngineType == GPUEngineType::Compute)
	{
		queue = m_computeQueue;
		fence = m_computeQueueFence;
	}

	vkResetFences(m_device, 1, &fence);
	if (vkQueueSubmit(queue, 1, &l_submitInfo, fence) != VK_SUCCESS)
	{
		Log(Error, "Failed to submit command buffer!");
		return false;
	}

	// TODO: Command list lifecycle needs proper synchronization
	// Cannot delete immediately as GPU execution is asynchronous
	// Need to wait for fence completion before returning to pool
	// Delete(commandList);

	return true;
}

bool VKRenderingServer::WaitOnGPU(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != semaphoreType)
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	
	// In the new architecture, command lists are not stored in render passes
	// We need to use the semaphore-based synchronization instead
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

bool VKRenderingServer::TryToTransitState(TextureComponent *rhs, CommandListComponent *commandList, Accessibility accessibility)
{
	auto l_rhs = reinterpret_cast<VKTextureComponent *>(rhs);
	auto l_newImageLayout = accessibility == Accessibility::ReadOnly ? l_rhs->m_ReadImageLayout : l_rhs->m_WriteImageLayout;
	if (l_rhs->m_CurrentImageLayout == l_newImageLayout)
		return false;

	auto l_commandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
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

bool VKRenderingServer::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

	vkCmdDispatch(l_vkCommandBuffer, threadGroupX, threadGroupY, threadGroupZ);

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

bool VKRenderingServer::GenerateMipmap(TextureComponent *rhs, CommandListComponent* commandList)
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