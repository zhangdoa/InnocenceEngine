#include "VKGraphicsService.h"
#include "../../Common/Array.h"

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
	return std::nullopt;
}

bool VKGraphicsService::Execute(CommandListComponent* commandList, GPUEngineType GPUEngineType)
{
	if (!commandList)
		return false;

	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

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

	return true;
}

bool VKGraphicsService::WaitOnGPU(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != semaphoreType)
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);

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
	auto l_shaderStage = ShaderStage::Invalid;
	TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, l_currentImageLayout, l_newImageLayout, l_shaderStage);
	l_rhs->m_CurrentImageLayout = l_newImageLayout;
	
	return true;
}

bool VKGraphicsService::TryToTransitState(GPUBufferComponent *gpuBuffer, CommandListComponent *commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility)
{
	return true;
}

bool VKGraphicsService::PresentImpl()
{
	thread_local uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_device,
		m_swapChain,
		std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

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

Inno::Array<Vec4> VKGraphicsService::ReadTextureBackToCPU(RenderPassComponent *canvas, TextureComponent *TextureComp)
{
	return Inno::Array<Vec4>();
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
