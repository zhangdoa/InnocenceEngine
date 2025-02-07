#include "VKRenderingServer.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

#include "../CommonFunctionDefinationMacro.inl"

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
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/EntityManager.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
	LogLevel l_logLevel = LogLevel::Verbose;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		l_logLevel = LogLevel::Warning;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		l_logLevel = LogLevel::Error;
	}

	g_Engine->Get<LogService>()->Print(l_logLevel, "VKRenderingServer: Validation Layer: ", pCallbackData->pMessage);
	return VK_FALSE;
}

bool VKRenderingServer::CreateHardwareResources()
{
    bool l_result = true;

    l_result &= CreateVkInstance();
#ifdef INNO_DEBUG
	l_result &= CreateDebugCallback();
#endif

	l_result &= CreatePhysicalDevice();
	l_result &= CreateLogicalDevice();

	l_result &= CreateVertexInputAttributions();
	l_result &= CreateTextureSamplers();
	l_result &= CreateMaterialDescriptorPool();
	l_result &= CreateGlobalCommandPool();
	l_result &= CreateSwapChain();
	l_result &= CreateSyncPrimitives();

    return l_result;
}

bool VKRenderingServer::ReleaseHardwareResources()
{
	return true;
}

bool VKRenderingServer::GetSwapChainImages()
{
    // Currently handled in CreateSwapChain
    return true;
}

bool VKRenderingServer::AssignSwapChainImages()
{
	if (!m_SwapChainRenderPassComp->m_OutputMergerTarget)
		Add(m_SwapChainRenderPassComp->m_OutputMergerTarget);

	auto l_outputMergerTarget = m_SwapChainRenderPassComp->m_OutputMergerTarget;
	if (l_outputMergerTarget->m_ColorOutputs.size() == 0)
	{
		l_outputMergerTarget->m_ColorOutputs.resize(1);
		l_outputMergerTarget->m_ColorOutputs[0] = AddTextureComponent((m_SwapChainRenderPassComp->m_InstanceName.c_str() + std::string("_RT/")).c_str());
	}

	auto l_VKTextureComp = reinterpret_cast<VKTextureComponent*>(l_outputMergerTarget->m_ColorOutputs[0]);
	
	// @TODO: Create image views for different back buffers
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

bool VKRenderingServer::ReleaseSwapChainImages()
{
   return true;
}

std::vector<const char *> VKRenderingServer::GetRequiredExtensions()
{
#if defined INNO_PLATFORM_WIN
	std::vector<const char *> l_extensions = {"VK_KHR_surface", "VK_KHR_win32_surface"};
#elif defined INNO_PLATFORM_MAC
	std::vector<const char *> extensions = {"VK_KHR_surface", "VK_MVK_macos_surface"};
#elif defined INNO_PLATFORM_LINUX
	std::vector<const char *> extensions = {"VK_KHR_surface", "VK_KHR_xcb_surface"};
#endif

	if (m_enableValidationLayers)
	{
		l_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		l_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return l_extensions;
}

bool VKRenderingServer::CreateVkInstance()
{
	// check support for validation layer
	if (m_enableValidationLayers && !CheckValidationLayerSupport(m_validationLayers))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo l_appInfo = {};
	l_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_appInfo.pApplicationName = g_Engine->GetApplicationName().c_str();
	l_appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 9);
	l_appInfo.pEngineName = "cence Engine";
	l_appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 9);
	l_appInfo.apiVersion = VK_API_VERSION_1_2;

	// set Vulkan instance create info with app info
	VkInstanceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	l_createInfo.pApplicationInfo = &l_appInfo;

	// set window extension info
	auto l_extensions = GetRequiredExtensions();
	l_createInfo.enabledExtensionCount = static_cast<uint32_t>(l_extensions.size());
	l_createInfo.ppEnabledExtensionNames = l_extensions.data();

	if (m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	// create Vulkan instance
	if (vkCreateInstance(&l_createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkInstance!");
		return false;
	}

	Log(Success, "VkInstance has been created.");
	return true;
}

bool VKRenderingServer::CreateDebugCallback()
{
	if (m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		l_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		l_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		l_createInfo.pfnUserCallback = DebugCallback;

		if (CreateDebugUtilsMessengerEXT(&l_createInfo, nullptr, &m_messengerCallback) != VK_SUCCESS)
		{
			m_ObjectStatus = ObjectStatus::Suspended;
			Log(Error, "Failed to create DebugUtilsMessenger!");
			return false;
		}

		Log(Success, "Validation Layer has been created.");
		return true;
	}
	else
	{
		return true;
	}
}

bool VKRenderingServer::CreatePhysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	std::vector<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, l_devices.data());

	for (const auto &l_device : l_devices)
	{
		if (IsDeviceSuitable(l_device, m_windowSurface, m_deviceExtensions))
		{
			m_physicalDevice = l_device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to find a suitable GPU!");
		return false;
	}

	Log(Success, "VkPhysicalDevice has been created.");
	return true;
}

bool VKRenderingServer::CreateLogicalDevice()
{
	QueueFamilyIndices l_indices = FindQueueFamilies(m_physicalDevice, m_windowSurface);

	std::vector<VkDeviceQueueCreateInfo> l_queueCreateInfos;
	std::set<uint32_t> l_uniqueQueueFamilies = {l_indices.m_graphicsFamily.value(), l_indices.m_presentFamily.value()};

	float l_queuePriority = 1.0f;
	for (uint32_t l_queueFamily : l_uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo l_queueCreateInfo = {};
		l_queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		l_queueCreateInfo.queueFamilyIndex = l_queueFamily;
		l_queueCreateInfo.queueCount = 1;
		l_queueCreateInfo.pQueuePriorities = &l_queuePriority;
		l_queueCreateInfos.push_back(l_queueCreateInfo);
	}

	VkPhysicalDeviceFeatures l_deviceFeatures = {};
	l_deviceFeatures.depthBiasClamp = VK_TRUE;
	l_deviceFeatures.depthClamp = VK_TRUE;
	l_deviceFeatures.dualSrcBlend = VK_TRUE;
	l_deviceFeatures.fillModeNonSolid = VK_TRUE;
	l_deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
	l_deviceFeatures.geometryShader = VK_TRUE;
	l_deviceFeatures.samplerAnisotropy = VK_TRUE;
	l_deviceFeatures.tessellationShader = VK_TRUE;
	l_deviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;

	VkDeviceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	l_createInfo.queueCreateInfoCount = static_cast<uint32_t>(l_queueCreateInfos.size());
	l_createInfo.pQueueCreateInfos = l_queueCreateInfos.data();

	l_createInfo.pEnabledFeatures = &l_deviceFeatures;
	VkPhysicalDeviceImagelessFramebufferFeatures l_imagelessFramebufferFeatures = {};
	l_imagelessFramebufferFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
	l_imagelessFramebufferFeatures.imagelessFramebuffer = true;
	l_createInfo.pNext = &l_imagelessFramebufferFeatures;

	VkPhysicalDeviceCustomBorderColorFeaturesEXT l_CustomBorderColorFeaturesEXT = {};
	l_CustomBorderColorFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
	l_CustomBorderColorFeaturesEXT.customBorderColors = true;
	l_CustomBorderColorFeaturesEXT.customBorderColorWithoutFormat = true;
	l_imagelessFramebufferFeatures.pNext = &l_CustomBorderColorFeaturesEXT;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR l_TimelineSemaphoreFeaturesKHR = {};
	l_TimelineSemaphoreFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
	l_TimelineSemaphoreFeaturesKHR.timelineSemaphore = true;
	l_CustomBorderColorFeaturesEXT.pNext = &l_TimelineSemaphoreFeaturesKHR;

	VkPhysicalDeviceSynchronization2Features l_Synchronization2Features = {};
	l_Synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	l_Synchronization2Features.synchronization2 = true;
	l_TimelineSemaphoreFeaturesKHR.pNext = &l_Synchronization2Features;

	l_createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	l_createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physicalDevice, &l_createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkDevice!");
		return false;
	}

	vkGetDeviceQueue(m_device, l_indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, l_indices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, l_indices.m_computeFamily.value(), 0, &m_computeQueue);

	Log(Success, "VkDevice has been created.");
	return true;
}

bool VKRenderingServer::CreateTextureSamplers()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	// if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_deferredRTSampler) != VK_SUCCESS)
	//{
	//	m_ObjectStatus = ObjectStatus::Suspended;
	//	Log(Error, "Failed to create VkSampler for deferred pass render target sampling!");
	//	return false;
	// }

	// Log(Success, "VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingServer::CreateVertexInputAttributions()
{
	m_vertexBindingDescription = {};
	m_vertexBindingDescription.binding = 0;
	m_vertexBindingDescription.stride = sizeof(Vertex);
	m_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttributeDescriptions = {};

	m_vertexAttributeDescriptions[0].binding = 0;
	m_vertexAttributeDescriptions[0].location = 0;
	m_vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[0].offset = offsetof(Vertex, m_pos);

	m_vertexAttributeDescriptions[1].binding = 0;
	m_vertexAttributeDescriptions[1].location = 1;
	m_vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[1].offset = offsetof(Vertex, m_normal);

	m_vertexAttributeDescriptions[2].binding = 0;
	m_vertexAttributeDescriptions[2].location = 2;
	m_vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[2].offset = offsetof(Vertex, m_tangent);

	m_vertexAttributeDescriptions[3].binding = 0;
	m_vertexAttributeDescriptions[3].location = 3;
	m_vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[3].offset = offsetof(Vertex, m_texCoord);

	m_vertexAttributeDescriptions[4].binding = 0;
	m_vertexAttributeDescriptions[4].location = 4;
	m_vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[4].offset = offsetof(Vertex, m_pad1);

	m_vertexAttributeDescriptions[5].binding = 0;
	m_vertexAttributeDescriptions[5].location = 5;
	m_vertexAttributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
	m_vertexAttributeDescriptions[5].offset = offsetof(Vertex, m_pad2);

	return true;
}

bool VKRenderingServer::CreateMaterialDescriptorPool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 8;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = {l_descriptorPoolSize};

	if (!CreateDescriptorPool(l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkDescriptorPool for material!");
		return false;
	}

	Log(Success, "VkDescriptorPool for material has been created.");

	std::vector<VkDescriptorSetLayoutBinding> l_textureLayoutBindings(8);
	for (size_t i = 0; i < l_textureLayoutBindings.size(); i++)
	{
		VkDescriptorSetLayoutBinding l_textureLayoutBinding = {};
		l_textureLayoutBinding.binding = (uint32_t)i;
		l_textureLayoutBinding.descriptorCount = 1;
		l_textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		l_textureLayoutBinding.pImmutableSamplers = nullptr;
		l_textureLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
		l_textureLayoutBindings[i] = l_textureLayoutBinding;
	}

	if (!CreateDescriptorSetLayout(&l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), m_materialDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	Log(Success, "VkDescriptorSetLayout for material has been created.");

	if (!CreateDescriptorSetLayout(nullptr, 0, m_dummyEmptyDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create DummyEmptyDescriptorLayout!");
		return false;
	}

	return true;
}

bool VKRenderingServer::CreateGlobalCommandPool()
{
	return CreateCommandPool(m_windowSurface, GPUEngineType::Graphics, m_globalCommandPool);
}

bool VKRenderingServer::CreateSwapChain()
{
	// choose device supported formats, modes and maximum back buffers
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

	// get swap chain VkImages
	// get count
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

	// get real VkImages
	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, m_swapChainImages.data()) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to acquire swap chain images!");
		return false;
	}

	Log(Success, "Swap chain images has been acquired.");

	return true;
}

bool VKRenderingServer::CreateSyncPrimitives()
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