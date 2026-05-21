#include "VKGraphicsService.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::GetSwapChainImages()
{
    return true;
}

bool VKGraphicsService::AssignSwapChainImages()
{
	if (!m_SwapChainRenderPassComp->m_OutputMergerTarget)
		Add(m_SwapChainRenderPassComp->m_OutputMergerTarget);

	auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
	if (l_outputMergerTarget->m_ColorOutputs.size() == 0)
	{
		l_outputMergerTarget->m_ColorOutputs.resize(1);
		l_outputMergerTarget->m_ColorOutputs[0] = AddTextureComponent((m_SwapChainRenderPassComp->m_InstanceName.c_str() + std::string("_RT")).c_str());
	}

	auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);

	for (size_t i = 0; i < m_swapChainImageCount; i++)
	{
		l_VKTextureComp->m_TextureDesc = m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;
		l_VKTextureComp->m_image = m_swapChainImages[i];
		l_VKTextureComp->m_VKTextureDesc = GetVKTextureDesc(l_VKTextureComp->m_TextureDesc);
		l_VKTextureComp->m_VKTextureDesc.format = m_presentSurfaceFormat;
		l_VKTextureComp->m_WriteImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_VKTextureComp->m_ReadImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		CreateImageView(l_VKTextureComp);
		l_VKTextureComp->m_GPUResourceType = GPUResourceType::Image;
		l_VKTextureComp->m_ObjectStatus = ObjectStatus::Activated;
	}

    return true;
}

bool VKGraphicsService::ReleaseSwapChainImages()
{
   return true;
}

bool VKGraphicsService::CreateSwapChain()
{
	auto l_swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_windowSurface);
	auto l_windowSurfaceExtent = ChooseSwapExtent(l_swapChainSupport.m_capabilities);
	auto l_windowSurfaceFormat = ChooseSwapSurfaceFormat(l_swapChainSupport.m_formats);
	auto l_presentMode = ChooseSwapPresentMode(l_swapChainSupport.m_presentModes);
	m_presentSurfaceExtent = l_windowSurfaceExtent;
	m_presentSurfaceFormat = l_windowSurfaceFormat.format;

	uint32_t l_imageCount = l_swapChainSupport.m_capabilities.minImageCount + 1;
	if (l_swapChainSupport.m_capabilities.maxImageCount > 0 && l_imageCount > l_swapChainSupport.m_capabilities.maxImageCount)
	{
		l_imageCount = l_swapChainSupport.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	l_createInfo.surface = m_windowSurface;
	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = m_presentSurfaceFormat;
	l_createInfo.imageColorSpace = l_windowSurfaceFormat.colorSpace;
	l_createInfo.imageExtent = m_presentSurfaceExtent;
	l_createInfo.imageArrayLayers = 1;
	l_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices l_indices = FindQueueFamilies(m_physicalDevice, m_windowSurface);
	uint32_t l_queueFamilyIndices[] = {l_indices.m_graphicsFamily.value(), l_indices.m_presentFamily.value()};

	if (l_indices.m_graphicsFamily.value() != l_indices.m_presentFamily.value())
	{
		l_createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		l_createInfo.queueFamilyIndexCount = 2;
		l_createInfo.pQueueFamilyIndices = l_queueFamilyIndices;
	}
	else
	{
		l_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	l_createInfo.preTransform = l_swapChainSupport.m_capabilities.currentTransform;
	l_createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	l_createInfo.presentMode = l_presentMode;
	l_createInfo.clipped = VK_TRUE;

	l_createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device, &l_createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkSwapChainKHR!");
		return false;
	}

	Log(Success, "VkSwapChainKHR has been created.");

	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, nullptr) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to query swap chain image count!");
		return false;
	}

	Log(Success, "Swap chain has ", l_imageCount, " image(s).");

	m_swapChainImages.reserve(l_imageCount);
	for (size_t i = 0; i < l_imageCount; i++)
	{
		m_swapChainImages.emplace_back();
	}

	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, m_swapChainImages.data()) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to acquire swap chain images!");
		return false;
	}

	Log(Success, "Swap chain images has been acquired.");

	return true;
}

bool VKGraphicsService::CreateSyncPrimitives()
{
	VkFenceCreateInfo l_fenceInfo = {};
	l_fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	l_fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(m_device, &l_fenceInfo, nullptr, &m_graphicsQueueFence) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create fence for GraphicsQueue!");
		return false;
	}

	if (vkCreateFence(m_device, &l_fenceInfo, nullptr, &m_computeQueueFence) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create fence for ComputeQueue!");
		return false;
	}

	m_imageAvailableSemaphores.resize(m_swapChainImages.size());
	m_swapChainRenderedSemaphores.resize(m_swapChainImages.size());

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		if (vkCreateSemaphore(
				m_device,
				&semaphoreInfo,
				nullptr,
				&m_imageAvailableSemaphores[i]) != VK_SUCCESS)
		{
			Log(Error, "Failed to create swap chain image available semaphores!");
			return false;
		}

		if (vkCreateSemaphore(
				m_device,
				&semaphoreInfo,
				nullptr,
				&m_swapChainRenderedSemaphores[i]) != VK_SUCCESS)
		{
			Log(Error, "Failed to create swap chain image rendered semaphores!");
			return false;
		}
	}

	return true;
}
