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

bool VKGraphicsService::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
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

std::optional<uint32_t> VKGraphicsService::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility)
{
	// Vulkan texture indexing not implemented yet
	return std::nullopt;
}

bool VKGraphicsService::Execute(CommandListComponent* commandList, GPUEngineType GPUEngineType)
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

bool VKGraphicsService::WaitOnGPU(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
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

bool VKGraphicsService::TryToTransitState(TextureComponent *rhs, CommandListComponent *commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	auto l_rhs = reinterpret_cast<VKTextureComponent *>(rhs);
	auto l_currentImageLayout = sourceAccessibility == Accessibility::ReadOnly ? l_rhs->m_ReadImageLayout : l_rhs->m_WriteImageLayout;
	auto l_newImageLayout = targetAccessibility == Accessibility::ReadOnly ? l_rhs->m_ReadImageLayout : l_rhs->m_WriteImageLayout;
	if (l_currentImageLayout == l_newImageLayout)
		return false;

	auto l_commandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_shaderStage = ShaderStage::Invalid; // @TODO: Nope this is not correct
	TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, l_currentImageLayout, l_newImageLayout, l_shaderStage);
	l_rhs->m_CurrentImageLayout = l_newImageLayout;
	
	return true;
}

bool VKGraphicsService::TryToTransitState(GPUBufferComponent *gpuBuffer, CommandListComponent *commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	// Vulkan doesn't require explicit buffer state transitions like D3D12
	// Buffer memory barriers are handled differently
	return true;
}

bool VKGraphicsService::PresentImpl()
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

bool Inno::VKGraphicsService::EndFrame()
{
    return false;
}

Vec4 VKGraphicsService::ReadRenderTargetSample(RenderPassComponent *rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> VKGraphicsService::ReadTextureBackToCPU(RenderPassComponent *canvas, TextureComponent *TextureComp)
{
	return std::vector<Vec4>();
}

bool VKGraphicsService::BeginCapture()
{
    return true;
}

bool VKGraphicsService::EndCapture()
{
	return true;
}

bool VKGraphicsService::ResizeImpl()
{
	return true;
}

void *VKGraphicsService::GetVkInstance()
{
	return m_instance;
}

void *VKGraphicsService::GetVkSurface()
{
	return &m_windowSurface;
