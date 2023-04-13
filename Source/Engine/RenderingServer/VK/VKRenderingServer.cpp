#include "VKRenderingServer.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine *g_Engine;

#include "VKHelper.h"
using namespace VKHelper;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"
#include "../../Core/InnoRandomizer.h"
#include "../../Template/ObjectPool.h"

namespace VKRenderingServerNS
{
	std::vector<const char *> GetRequiredExtensions();

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

		InnoLogger::Log(l_logLevel, "VKRenderingServer: Validation Layer: ", pCallbackData->pMessage);
		return VK_FALSE;
	}

	bool CreateVkInstance();
	bool CreateDebugCallback();

	bool CreatePhysicalDevice();
	bool CreateLogicalDevice();

	bool CreateTextureSamplers();
	bool CreateVertexInputAttributions();
	bool CreateMaterialDescriptorPool();
	bool CreateGlobalCommandPool();
	bool CreateSwapChain();
	bool CreateSyncPrimitives();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	TObjectPool<VKMeshComponent> *m_MeshComponentPool = 0;
	TObjectPool<VKMaterialComponent> *m_MaterialComponentPool = 0;
	TObjectPool<VKTextureComponent> *m_TextureComponentPool = 0;
	TObjectPool<VKRenderPassComponent> *m_RenderPassComponentPool = 0;
	TObjectPool<VKPipelineStateObject> *m_PSOPool = 0;
	TObjectPool<VKCommandList> *m_CommandListPool = 0;
	TObjectPool<VKSemaphore> *m_SemaphorePool = 0;
	TObjectPool<VKShaderProgramComponent> *m_ShaderProgramComponentPool = 0;
	TObjectPool<VKSamplerComponent> *m_SamplerComponentPool = 0;
	TObjectPool<VKGPUBufferComponent> *m_GPUBufferComponentPool = 0;

	std::unordered_set<MeshComponent *> m_initializedMeshes;
	std::unordered_set<TextureComponent *> m_initializedTextures;
	std::unordered_set<MaterialComponent *> m_initializedMaterials;

	VkInstance m_instance;
	VkSurfaceKHR m_windowSurface;
	std::vector<VkImage> m_swapChainImages;
	VkQueue m_presentQueue;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkCommandPool m_globalCommandPool;
	VkQueue m_graphicsQueue;
	VkFence m_graphicsQueueFence;
	VkQueue m_computeQueue;
	VkFence m_computeQueueFence;
	std::atomic<uint64_t> m_graphicCommandQueueSemaphore = 0;
	std::atomic<uint64_t> m_computeCommandQueueSemaphore = 0;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_swapChainRenderedSemaphores;
	VkSwapchainKHR m_swapChain = 0;
	VkExtent2D m_presentSurfaceExtent = {};
	VkFormat m_presentSurfaceFormat = {};

	const std::vector<const char *> m_deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_MAINTENANCE2_EXTENSION_NAME,		 // For imageless framebuffer
			VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME, // For imageless framebuffer
			VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
			VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME};

#ifdef INNO_DEBUG
	const bool m_enableValidationLayers = true;
#else
	const bool m_enableValidationLayers = false;
#endif

	const std::vector<const char *> m_validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"};

	VkDebugUtilsMessengerEXT m_messengerCallback;

	VkVertexInputBindingDescription m_vertexBindingDescription;
	std::array<VkVertexInputAttributeDescription, 5> m_vertexAttributeDescriptions;

	VkDescriptorPool m_materialDescriptorPool;
	VkDescriptorSetLayout m_materialDescriptorLayout;
	VkDescriptorSetLayout m_dummyEmptyDescriptorLayout;

	GPUResourceComponent *m_userPipelineOutput = 0;
	VKRenderPassComponent *m_SwapChainRenderPassComp = 0;
	VKShaderProgramComponent *m_SwapChainSPC = 0;
	VKSamplerComponent *m_SwapChainSamplerComp = 0;
} // namespace VKRenderingServerNS

std::vector<const char *> VKRenderingServerNS::GetRequiredExtensions()
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

bool VKRenderingServerNS::CreateVkInstance()
{
	// check support for validation layer
	if (m_enableValidationLayers && !CheckValidationLayerSupport(m_validationLayers))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo l_appInfo = {};
	l_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_appInfo.pApplicationName = g_Engine->getApplicationName().c_str();
	l_appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 9);
	l_appInfo.pEngineName = "Innocence Engine";
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkInstance!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkInstance has been created.");
	return true;
}

bool VKRenderingServerNS::CreateDebugCallback()
{
	if (m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		l_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		l_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		l_createInfo.pfnUserCallback = DebugCallback;

		if (VKHelper::CreateDebugUtilsMessengerEXT(m_instance, &l_createInfo, nullptr, &m_messengerCallback) != VK_SUCCESS)
		{
			m_ObjectStatus = ObjectStatus::Suspended;
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create DebugUtilsMessenger!");
			return false;
		}

		InnoLogger::Log(LogLevel::Success, "VKRenderingServer: Validation Layer has been created.");
		return true;
	}
	else
	{
		return true;
	}
}

bool VKRenderingServerNS::CreatePhysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to find GPUs with Vulkan support!");
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to find a suitable GPU!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkPhysicalDevice has been created.");
	return true;
}

bool VKRenderingServerNS::CreateLogicalDevice()
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDevice!");
		return false;
	}

	vkGetDeviceQueue(m_device, l_indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, l_indices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, l_indices.m_computeFamily.value(), 0, &m_computeQueue);

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDevice has been created.");
	return true;
}

bool VKRenderingServerNS::CreateTextureSamplers()
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
	//	InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkSampler for deferred pass render target sampling!");
	//	return false;
	// }

	// InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingServerNS::CreateVertexInputAttributions()
{
	m_vertexBindingDescription = {};
	m_vertexBindingDescription.binding = 0;
	m_vertexBindingDescription.stride = sizeof(Vertex);
	m_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttributeDescriptions = {};

	m_vertexAttributeDescriptions[0].binding = 0;
	m_vertexAttributeDescriptions[0].location = 0;
	m_vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[0].offset = offsetof(Vertex, m_pos);

	m_vertexAttributeDescriptions[1].binding = 0;
	m_vertexAttributeDescriptions[1].location = 1;
	m_vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[1].offset = offsetof(Vertex, m_texCoord);

	m_vertexAttributeDescriptions[2].binding = 0;
	m_vertexAttributeDescriptions[2].location = 2;
	m_vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[2].offset = offsetof(Vertex, m_pad1);

	m_vertexAttributeDescriptions[3].binding = 0;
	m_vertexAttributeDescriptions[3].location = 3;
	m_vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[3].offset = offsetof(Vertex, m_normal);

	m_vertexAttributeDescriptions[4].binding = 0;
	m_vertexAttributeDescriptions[4].location = 4;
	m_vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[4].offset = offsetof(Vertex, m_pad2);

	return true;
}

bool VKRenderingServerNS::CreateMaterialDescriptorPool()
{
	auto l_renderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 8;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = {l_descriptorPoolSize};

	if (!CreateDescriptorPool(m_device, l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorPool for material!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDescriptorPool for material has been created.");

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

	if (!CreateDescriptorSetLayout(m_device, &l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), m_materialDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDescriptorSetLayout for material has been created.");

	if (!CreateDescriptorSetLayout(m_device, nullptr, 0, m_dummyEmptyDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create DummyEmptyDescriptorLayout!");
		return false;
	}

	return true;
}

bool VKRenderingServerNS::CreateGlobalCommandPool()
{
	return CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, GPUEngineType::Graphics, m_globalCommandPool);
}

bool VKRenderingServerNS::CreateSyncPrimitives()
{
	VkFenceCreateInfo l_fenceInfo = {};
	l_fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	l_fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(m_device, &l_fenceInfo, nullptr, &m_graphicsQueueFence) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create fence for GraphicsQueue!");
		return false;
	}

	if (vkCreateFence(m_device, &l_fenceInfo, nullptr, &m_computeQueueFence) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create fence for ComputeQueue!");
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
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create swap chain image available semaphores!");
			return false;
		}

		if (vkCreateSemaphore(
				m_device,
				&semaphoreInfo,
				nullptr,
				&m_swapChainRenderedSemaphores[i]) != VK_SUCCESS)
		{
			InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create swap chain image rendered semaphores!");
			return false;
		}
	}

	return true;
}

bool VKRenderingServerNS::CreateSwapChain()
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkSwapChainKHR!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkSwapChainKHR has been created.");

	// get swap chain VkImages
	// get count
	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, nullptr) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to query swap chain image count!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: Swap chain has ", l_imageCount, " image(s).");

	m_swapChainImages.reserve(l_imageCount);
	for (size_t i = 0; i < l_imageCount; i++)
	{
		m_swapChainImages.emplace_back();
	}

	// get real VkImages
	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, m_swapChainImages.data()) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to acquire swap chain images!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: Swap chain images has been acquired.");

	return true;
}

using namespace VKRenderingServerNS;

VKPipelineStateObject *addPSO()
{
	return m_PSOPool->Spawn();
}

VKCommandList *addCommandList()
{
	return m_CommandListPool->Spawn();
}

VKSemaphore *addSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool VKRenderingServer::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	m_MeshComponentPool = TObjectPool<VKMeshComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureComponentPool = TObjectPool<VKTextureComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialComponentPool = TObjectPool<VKMaterialComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassComponentPool = TObjectPool<VKRenderPassComponent>::Create(128);
	m_PSOPool = TObjectPool<VKPipelineStateObject>::Create(128);
	m_CommandListPool = TObjectPool<VKCommandList>::Create(256);
	m_SemaphorePool = TObjectPool<VKSemaphore>::Create(512);
	m_ShaderProgramComponentPool = TObjectPool<VKShaderProgramComponent>::Create(256);
	m_SamplerComponentPool = TObjectPool<VKSamplerComponent>::Create(256);
	m_GPUBufferComponentPool = TObjectPool<VKGPUBufferComponent>::Create(256);

	m_SwapChainRenderPassComp = reinterpret_cast<VKRenderPassComponent *>(AddRenderPassComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<VKShaderProgramComponent *>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSamplerComp = reinterpret_cast<VKSamplerComponent *>(AddSamplerComponent("SwapChain/"));

	CreateVkInstance();
	CreateDebugCallback();

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer Setup finished.");

	return true;
}

bool VKRenderingServer::Initialize()
{
	bool l_result = true;

	l_result &= CreatePhysicalDevice();
	l_result &= CreateLogicalDevice();

	l_result &= CreateVertexInputAttributions();
	l_result &= CreateTextureSamplers();
	l_result &= CreateMaterialDescriptorPool();
	l_result &= CreateGlobalCommandPool();
	l_result &= CreateSwapChain();
	l_result &= CreateSyncPrimitives();

	m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

	l_result &= InitializeShaderProgramComponent(m_SwapChainSPC);

	l_result &= InitializeSamplerComponent(m_SwapChainSamplerComp);

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = m_swapChainImages.size();

	m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_UseMultiFrames = true;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_SubresourceCount = 1;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainSPC;

	l_result &= ReserveRenderTargets(m_SwapChainRenderPassComp, this);

	// use device created swap chain textures
	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		auto l_VKTextureComp = reinterpret_cast<VKTextureComponent *>(m_SwapChainRenderPassComp->m_RenderTargets[i]);
		l_VKTextureComp->m_TextureDesc = m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;
		l_VKTextureComp->m_image = m_swapChainImages[i];
		l_VKTextureComp->m_VKTextureDesc = GetVKTextureDesc(l_VKTextureComp->m_TextureDesc);
		l_VKTextureComp->m_VKTextureDesc.format = m_presentSurfaceFormat;
		l_VKTextureComp->m_WriteImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_VKTextureComp->m_ReadImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		CreateImageView(m_device, l_VKTextureComp);
		l_VKTextureComp->m_GPUResourceType = GPUResourceType::Image;
		l_VKTextureComp->m_ObjectStatus = ObjectStatus::Activated;
	}

	m_SwapChainRenderPassComp->m_PipelineStateObject = addPSO();
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = static_cast<float>(m_presentSurfaceExtent.width);
	m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = static_cast<float>(m_presentSurfaceExtent.height);

	l_result &= CreateRenderPass(m_device, m_SwapChainRenderPassComp, &m_presentSurfaceFormat);
	l_result &= CreateViewportAndScissor(m_SwapChainRenderPassComp);

	l_result &= CreateMultipleFramebuffers(m_device, m_SwapChainRenderPassComp);

	l_result &= CreateDescriptorSetLayout(m_device, m_dummyEmptyDescriptorLayout, m_SwapChainRenderPassComp);

	l_result &= CreateDescriptorPool(m_device, m_SwapChainRenderPassComp);

	l_result &= CreateDescriptorSets(m_device, m_SwapChainRenderPassComp);

	l_result &= CreatePipelineLayout(m_device, m_SwapChainRenderPassComp);

	l_result &= CreateGraphicsPipelines(m_device, m_SwapChainRenderPassComp);

	l_result &= CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, GPUEngineType::Graphics, m_SwapChainRenderPassComp->m_GraphicsCommandPool);
	l_result &= CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, GPUEngineType::Compute, m_SwapChainRenderPassComp->m_ComputeCommandPool);

	m_SwapChainRenderPassComp->m_CommandLists.resize(m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetCount);

	for (size_t i = 0; i < m_SwapChainRenderPassComp->m_CommandLists.size(); i++)
	{
		m_SwapChainRenderPassComp->m_CommandLists[i] = addCommandList();
	}

	l_result &= CreateCommandBuffers(m_device, m_SwapChainRenderPassComp);

	m_SwapChainRenderPassComp->m_Semaphores.resize(m_SwapChainRenderPassComp->m_Framebuffers.size());

	for (size_t i = 0; i < m_SwapChainRenderPassComp->m_Semaphores.size(); i++)
	{
		m_SwapChainRenderPassComp->m_Semaphores[i] = addSemaphore();
	}

	l_result &= CreateSyncPrimitives(m_device, m_SwapChainRenderPassComp);
	m_SwapChainRenderPassComp->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool VKRenderingServer::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer has been terminated.");

	return true;
}

ObjectStatus VKRenderingServer::GetStatus()
{
	return m_ObjectStatus;
}

AddComponent(VK, Mesh);
AddComponent(VK, Texture);
AddComponent(VK, Material);
AddComponent(VK, RenderPass);
AddComponent(VK, ShaderProgram);
AddComponent(VK, Sampler);
AddComponent(VK, GPUBuffer);

bool CreateHostStagingBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBufferCreateInfo l_stagingBufferCInfo = {};
	l_stagingBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_stagingBufferCInfo.size = bufferSize;
	l_stagingBufferCInfo.usage = usageFlags;
	l_stagingBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(m_physicalDevice, m_device, l_stagingBufferCInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMemory);
}

bool CreateDeviceLocalBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBufferCreateInfo l_localBufferCInfo = {};
	l_localBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_localBufferCInfo.size = bufferSize;
	l_localBufferCInfo.usage = usageFlags;
	l_localBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(m_physicalDevice, m_device, l_localBufferCInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);
}

bool CopyHostMemoryToDeviceMemory(void *hostMemory, size_t bufferSize, VkDeviceMemory &deviceMemory)
{
	void *l_mappedMemory;
	vkMapMemory(m_device, deviceMemory, 0, bufferSize, 0, &l_mappedMemory);
	std::memcpy(l_mappedMemory, hostMemory, (size_t)bufferSize);
	vkUnmapMemory(m_device, deviceMemory);

	return true;
}
bool InitializeDeviceLocalBuffer(void *hostMemory, size_t bufferSize, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;

	CreateHostStagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

	CopyHostMemoryToDeviceMemory(hostMemory, bufferSize, l_stagingBufferMemory);

	CopyBuffer(m_device, m_globalCommandPool, m_graphicsQueue, l_stagingBuffer, buffer, bufferSize);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	return true;
}

template <typename T>
bool InitializeDeviceLocalBuffer(Array<T> &hostMemory, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	auto l_bufferSize = sizeof(T) * hostMemory.size();

	return InitializeDeviceLocalBuffer(&hostMemory[0], l_bufferSize, buffer, deviceMemory);
}

bool VKRenderingServer::InitializeMeshComponent(MeshComponent *rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMeshComponent *>(rhs);

	auto l_VBSize = sizeof(Vertex) * l_rhs->m_vertices.size();
	auto l_IBSize = sizeof(Index) * l_rhs->m_indices.size();

	CreateDeviceLocalBuffer(l_VBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_VBO, l_rhs->m_VBMemory);
	CreateDeviceLocalBuffer(l_IBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_IBO, l_rhs->m_IBMemory);

	InitializeDeviceLocalBuffer(l_rhs->m_vertices, l_rhs->m_VBO, l_rhs->m_VBMemory);
	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	InitializeDeviceLocalBuffer(l_rhs->m_indices, l_rhs->m_IBO, l_rhs->m_IBMemory);
	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

#ifdef INNO_DEBUG
	SetObjectName(m_device, l_rhs, l_rhs->m_VBO, VK_OBJECT_TYPE_BUFFER, "VB");
	SetObjectName(m_device, l_rhs, l_rhs->m_IBO, VK_OBJECT_TYPE_BUFFER, "IB");
#endif //  INNO_DEBUG

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeTextureComponent(TextureComponent *rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKTextureComponent *>(rhs);
	l_rhs->m_VKTextureDesc = GetVKTextureDesc(rhs->m_TextureDesc);
	l_rhs->m_ImageCreateInfo = GetImageCreateInfo(rhs->m_TextureDesc, l_rhs->m_VKTextureDesc);
	l_rhs->m_WriteImageLayout = GetTextureWriteImageLayout(l_rhs->m_TextureDesc);
	l_rhs->m_ReadImageLayout = GetTextureReadImageLayout(l_rhs->m_TextureDesc);
	if (l_rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment || l_rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		l_rhs->m_CurrentImageLayout = l_rhs->m_WriteImageLayout;
	}
	else
	{
		l_rhs->m_CurrentImageLayout = l_rhs->m_ReadImageLayout;
	}

	CreateImage(m_physicalDevice, m_device, l_rhs->m_ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, l_rhs->m_image, l_rhs->m_imageMemory);

#ifdef INNO_DEBUG
	SetObjectName(m_device, l_rhs, l_rhs->m_image, VK_OBJECT_TYPE_IMAGE, "Image");
#endif //  INNO_DEBUG

	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;
	if (l_rhs->m_TextureData != nullptr)
	{
		CreateHostStagingBuffer(l_rhs->m_VKTextureDesc.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

		void *l_mappedMemory;
		vkMapMemory(m_device, l_stagingBufferMemory, 0, l_rhs->m_VKTextureDesc.imageSize, 0, &l_mappedMemory);
		std::memcpy(l_mappedMemory, l_rhs->m_TextureData, static_cast<size_t>(l_rhs->m_VKTextureDesc.imageSize));
		vkUnmapMemory(m_device, l_stagingBufferMemory);
	}

	VkCommandBuffer l_commandBuffer = OpenTemporaryCommandBuffer(m_device, m_globalCommandPool);

	if (l_rhs->m_TextureData != nullptr)
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(l_commandBuffer, l_stagingBuffer, l_rhs->m_image, l_rhs->m_VKTextureDesc.aspectFlags, static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.width), static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.height));
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, l_rhs->m_CurrentImageLayout);
	}
	else
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, l_rhs->m_CurrentImageLayout);
	}

	CloseTemporaryCommandBuffer(m_device, m_globalCommandPool, m_graphicsQueue, l_commandBuffer);

	if (l_rhs->m_TextureData != nullptr)
	{
		vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
		vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);
	}

	CreateImageView(m_device, l_rhs);

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VkImage ", l_rhs->m_image, " is initialized.");

	return true;
}

bool VKRenderingServer::InitializeMaterialComponent(MaterialComponent *rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMaterialComponent *>(rhs);

	auto l_defaultMaterial = g_Engine->getRenderingFrontend()->getDefaultMaterialComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<VKTextureComponent *>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureComponent(l_texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
			l_rhs->m_TextureSlots[i].m_Activate = true;
		}
		else
		{
			auto l_texture = reinterpret_cast<VKTextureComponent *>(l_defaultMaterial->m_TextureSlots[i].m_Texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
		}
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);

	bool l_result = true;

	l_result &= ReserveRenderTargets(l_rhs, this);

	l_result &= CreateRenderTargets(l_rhs, this);

	l_rhs->m_PipelineStateObject = addPSO();

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= CreateRenderPass(m_device, l_rhs);
		l_result &= CreateViewportAndScissor(l_rhs);
		if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
		{
			l_result &= CreateMultipleFramebuffers(m_device, l_rhs);
		}
		else
		{
			l_result &= CreateSingleFramebuffer(m_device, l_rhs);
		}
	}

	l_result &= CreateDescriptorSetLayout(m_device, m_dummyEmptyDescriptorLayout, l_rhs);

	l_result &= CreateDescriptorPool(m_device, l_rhs);

	l_result &= CreateDescriptorSets(m_device, l_rhs);

	l_result &= CreatePipelineLayout(m_device, l_rhs);

	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		l_result &= CreateGraphicsPipelines(m_device, l_rhs);
	}
	else if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute)
	{
		l_result &= CreateComputePipelines(m_device, l_rhs);
	}

	l_result &= CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, GPUEngineType::Graphics, l_rhs->m_GraphicsCommandPool);
	l_result &= CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, GPUEngineType::Compute, l_rhs->m_ComputeCommandPool);

	if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
	{
		l_rhs->m_CommandLists.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);
	}
	else
	{
		l_rhs->m_CommandLists.resize(1);
	}

	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		l_rhs->m_CommandLists[i] = addCommandList();
	}

	l_result &= CreateCommandBuffers(m_device, l_rhs);

	l_rhs->m_Semaphores.resize(l_rhs->m_Framebuffers.size());

	for (size_t i = 0; i < l_rhs->m_Semaphores.size(); i++)
	{
		l_rhs->m_Semaphores[i] = addSemaphore();
	}

	l_result &= CreateSyncPrimitives(m_device, l_rhs);
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool VKRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKShaderProgramComponent *>(rhs);

	bool l_result = true;

	l_rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	l_rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 1;
	l_rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size());
	l_rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_vertexBindingDescription;
	l_rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_VSHandle, l_rhs->m_ShaderFilePaths.m_VSPath);
		l_rhs->m_VSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_VSCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_rhs->m_VSCInfo.module = l_rhs->m_VSHandle;
		l_rhs->m_VSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_HSHandle, l_rhs->m_ShaderFilePaths.m_HSPath);
		l_rhs->m_HSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_HSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		l_rhs->m_HSCInfo.module = l_rhs->m_HSHandle;
		l_rhs->m_HSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_DSHandle, l_rhs->m_ShaderFilePaths.m_DSPath);
		l_rhs->m_DSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_DSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		l_rhs->m_DSCInfo.module = l_rhs->m_DSHandle;
		l_rhs->m_DSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_GSHandle, l_rhs->m_ShaderFilePaths.m_GSPath);
		l_rhs->m_GSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_GSCInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		l_rhs->m_GSCInfo.module = l_rhs->m_GSHandle;
		l_rhs->m_GSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_PSHandle, l_rhs->m_ShaderFilePaths.m_PSPath);
		l_rhs->m_PSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_PSCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_rhs->m_PSCInfo.module = l_rhs->m_PSHandle;
		l_rhs->m_PSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		l_result &= CreateShaderModule(m_device, l_rhs->m_CSHandle, l_rhs->m_ShaderFilePaths.m_CSPath);
		l_rhs->m_CSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_CSCInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		l_rhs->m_CSCInfo.module = l_rhs->m_CSHandle;
		l_rhs->m_CSCInfo.pName = "main";
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool VKRenderingServer::InitializeSamplerComponent(SamplerComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKSamplerComponent *>(rhs);

	l_rhs->m_samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	l_rhs->m_samplerCInfo.minFilter = GetFilter(l_rhs->m_SamplerDesc.m_MinFilterMethod);
	l_rhs->m_samplerCInfo.magFilter = GetFilter(l_rhs->m_SamplerDesc.m_MagFilterMethod);
	l_rhs->m_samplerCInfo.mipmapMode = GetSamplerMipmapMode(l_rhs->m_SamplerDesc.m_MinFilterMethod);
	l_rhs->m_samplerCInfo.addressModeU = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodU);
	l_rhs->m_samplerCInfo.addressModeV = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodV);
	l_rhs->m_samplerCInfo.addressModeW = GetSamplerAddressMode(l_rhs->m_SamplerDesc.m_WrapMethodW);
	l_rhs->m_samplerCInfo.mipLodBias = 0.0f;
	l_rhs->m_samplerCInfo.maxAnisotropy = float(l_rhs->m_SamplerDesc.m_MaxAnisotropy);
	l_rhs->m_samplerCInfo.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
	l_rhs->m_samplerCInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
	l_rhs->m_samplerCInfo.minLod = l_rhs->m_SamplerDesc.m_MinLOD;
	l_rhs->m_samplerCInfo.maxLod = l_rhs->m_SamplerDesc.m_MaxLOD;

	VkSamplerCustomBorderColorCreateInfoEXT l_samplerCustomBorderColorCInfoEXT = {};
	l_samplerCustomBorderColorCInfoEXT.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[0] = l_rhs->m_SamplerDesc.m_BorderColor[0];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[1] = l_rhs->m_SamplerDesc.m_BorderColor[1];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[2] = l_rhs->m_SamplerDesc.m_BorderColor[2];
	l_samplerCustomBorderColorCInfoEXT.customBorderColor.float32[3] = l_rhs->m_SamplerDesc.m_BorderColor[3];
	l_samplerCustomBorderColorCInfoEXT.format = VK_FORMAT_UNDEFINED;

	l_rhs->m_samplerCInfo.pNext = &l_samplerCustomBorderColorCInfoEXT;

	if (vkCreateSampler(m_device, &l_rhs->m_samplerCInfo, nullptr, &l_rhs->m_sampler) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create sampler!");
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VKRenderingServer::InitializeGPUBufferComponent(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent *>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	CreateHostStagingBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_HostStagingBuffer, l_rhs->m_HostStagingMemory);

	if (l_rhs->m_InitialData != nullptr)
	{
		CopyHostMemoryToDeviceMemory(l_rhs->m_InitialData, l_rhs->m_TotalSize, l_rhs->m_HostStagingMemory);
	}

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			CreateDeviceLocalBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_DeviceLocalBuffer, l_rhs->m_DeviceLocalMemory);
			if (l_rhs->m_InitialData != nullptr)
			{
				CopyBuffer(m_device, m_globalCommandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_rhs->m_TotalSize);
			}
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Not support CPU-readable default heap GPU buffer currently.");
		}
	}

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VKRenderingServer::DeleteMeshComponent(MeshComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteTextureComponent(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMaterialComponent(MaterialComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteRenderPassComponent(RenderPassComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteSamplerComponent(SamplerComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteGPUBufferComponent(GPUBufferComponent *rhs)
{
	return true;
}

bool VKRenderingServer::ClearTextureComponent(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::CopyTextureComponent(TextureComponent *lhs, TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent *rhs, const void *GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent *>(rhs);

	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}

	void *l_mappedMemory;
	vkMapMemory(m_device, l_rhs->m_HostStagingMemory, startOffset, l_size, 0, &l_mappedMemory);
	std::memcpy(l_mappedMemory, GPUBufferValue, l_size);
	vkUnmapMemory(m_device, l_rhs->m_HostStagingMemory);

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		CopyBuffer(m_device, m_globalCommandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_size);
	}

	return true;
}

bool VKRenderingServer::ClearGPUBufferComponent(GPUBufferComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferComponent *>(rhs);

	auto l_commandBuffer = OpenTemporaryCommandBuffer(m_device, m_globalCommandPool);

	vkCmdFillBuffer(l_commandBuffer, l_rhs->m_HostStagingBuffer, 0, 0, 0);
	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		vkCmdFillBuffer(l_commandBuffer, l_rhs->m_DeviceLocalBuffer, 0, 0, 0);
	}

	CloseTemporaryCommandBuffer(m_device, m_globalCommandPool, m_graphicsQueue, l_commandBuffer);

	return true;
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool TryToTransitImageLayout(VKTextureComponent *rhs, const VkCommandBuffer &commandBuffer, const VkImageLayout &newImageLayout, ShaderStage shaderStage = ShaderStage::Invalid)
{
	if (rhs->m_CurrentImageLayout != newImageLayout)
	{
		TransitImageLayout(commandBuffer, rhs->m_image, rhs->m_ImageCreateInfo.format, rhs->m_VKTextureDesc.aspectFlags, rhs->m_CurrentImageLayout, newImageLayout, shaderStage);
		rhs->m_CurrentImageLayout = newImageLayout;
		return true;
	}

	return false;
}

bool PrepareRenderTargets(VKRenderPassComponent *renderPass, VKCommandList *commandList)
{
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_UseMultiFrames)
		{
			auto l_rhs = reinterpret_cast<VKTextureComponent *>(renderPass->m_RenderTargets[renderPass->m_CurrentFrame]);

			TryToTransitImageLayout(l_rhs, commandList->m_GraphicsCommandBuffer, l_rhs->m_WriteImageLayout);
		}
		else
		{
			for (size_t i = 0; i < renderPass->m_RenderPassDesc.m_RenderTargetCount; i++)
			{
				auto l_rhs = reinterpret_cast<VKTextureComponent *>(renderPass->m_RenderTargets[i]);

				TryToTransitImageLayout(l_rhs, commandList->m_GraphicsCommandBuffer, l_rhs->m_WriteImageLayout);
			}
		}

		if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite)
		{
			auto l_rhs = reinterpret_cast<VKTextureComponent *>(renderPass->m_DepthStencilRenderTarget);

			TryToTransitImageLayout(l_rhs, commandList->m_GraphicsCommandBuffer, l_rhs->m_WriteImageLayout);
		}
	}

	return true;
}

bool VKRenderingServer::BindRenderPassComponent(RenderPassComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	PrepareRenderTargets(l_rhs, l_commandList);

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

	return true;
}

bool VKRenderingServer::CleanRenderTargets(RenderPassComponent *rhs)
{
	return true;
}

bool VKRenderingServer::BindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	if (resource == nullptr)
	{
		InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Empty GPU resource in render pass: ", renderPass->m_InstanceName.c_str(), ", at: ", resourceBindingLayoutDescIndex);
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

	switch (resource->m_GPUResourceType)
	{
	case GPUResourceType::Sampler:
	{
		l_descriptorImageInfo.sampler = reinterpret_cast<VKSamplerComponent *>(resource)->m_sampler;
		l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_SAMPLER, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
		UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
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
		UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
		break;
	}
	case GPUResourceType::Buffer:
		if (resource->m_GPUAccessibility == Accessibility::ReadOnly)
		{
			if (accessibility != Accessibility::ReadOnly)
			{
				InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Not allow GPU write to Constant Buffer!");
			}
			else
			{
				l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferComponent *>(resource)->m_HostStagingBuffer;
				l_descriptorBufferInfo.offset = startOffset;
				l_descriptorBufferInfo.range = elementCount;
				l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, l_descriptorIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[l_descriptorSetIndex]);
				UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
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
			UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
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
			TryToTransitImageLayout(l_VKTextureComp, l_commandBuffer, VK_IMAGE_LAYOUT_GENERAL, shaderStage);
		}
		else
		{
			TryToTransitImageLayout(l_VKTextureComp, l_commandBuffer, l_VKTextureComp->m_ReadImageLayout, shaderStage);
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

bool VKRenderingServer::DrawIndexedInstanced(RenderPassComponent *renderPass, MeshComponent *mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_mesh = reinterpret_cast<VKMeshComponent *>(mesh);

	VkBuffer vertexBuffers[] = {l_mesh->m_VBO};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(l_commandList->m_GraphicsCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(l_commandList->m_GraphicsCommandBuffer, l_mesh->m_IBO, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(l_commandList->m_GraphicsCommandBuffer, static_cast<uint32_t>(l_mesh->m_indicesSize), 1, 0, 0, 0);

	return true;
}

bool VKRenderingServer::DrawInstanced(RenderPassComponent *renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	vkCmdDraw(l_commandList->m_GraphicsCommandBuffer, 1, static_cast<uint32_t>(instanceCount), 0, 0);

	return true;
}

bool VKRenderingServer::UnbindGPUResource(RenderPassComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility, size_t startOffset, size_t elementCount)
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to end recording command buffer!");
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
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to submit command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::WaitCommandQueue(RenderPassComponent *rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
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

bool VKRenderingServer::WaitFence(RenderPassComponent *rhs, GPUEngineType GPUEngineType)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType != GPUEngineType)
	{
		return true;
	}

	if (GPUEngineType == GPUEngineType::Graphics)
	{
		vkWaitForFences(m_device, 1, &m_graphicsQueueFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	}
	else if (GPUEngineType == GPUEngineType::Compute)
	{
		vkWaitForFences(m_device, 1, &m_computeQueueFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	}

	return true;
}

bool VKRenderingServer::SetUserPipelineOutput(GPUResourceComponent *rhs)
{
	m_userPipelineOutput = rhs;
	return true;
}

GPUResourceComponent *VKRenderingServer::GetUserPipelineOutput()
{
	return m_userPipelineOutput;
}

bool VKRenderingServer::Present()
{
	auto l_commandList = reinterpret_cast<VKCommandList *>(m_SwapChainRenderPassComp->m_CommandLists[m_SwapChainRenderPassComp->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(m_SwapChainRenderPassComp->m_PipelineStateObject);
	auto l_VKTextureComp = reinterpret_cast<VKTextureComponent *>(m_SwapChainRenderPassComp->m_RenderTargets[m_SwapChainRenderPassComp->m_CurrentFrame]);
	auto l_semaphore = reinterpret_cast<VKSemaphore *>(m_SwapChainRenderPassComp->m_Semaphores[m_SwapChainRenderPassComp->m_CurrentFrame]);

	CommandListBegin(m_SwapChainRenderPassComp, m_SwapChainRenderPassComp->m_CurrentFrame);

	BindRenderPassComponent(m_SwapChainRenderPassComp);

	CleanRenderTargets(m_SwapChainRenderPassComp);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_SwapChainSamplerComp, 1, Accessibility::ReadOnly, 0, SIZE_MAX);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_userPipelineOutput, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	auto l_mesh = g_Engine->getRenderingFrontend()->getMeshComponent(ProceduralMeshShape::Square);

	DrawIndexedInstanced(m_SwapChainRenderPassComp, l_mesh, 1);

	UnbindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_userPipelineOutput, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	TryToTransitImageLayout(l_VKTextureComp, l_commandList->m_GraphicsCommandBuffer, l_VKTextureComp->m_ReadImageLayout);

	CommandListEnd(m_SwapChainRenderPassComp);

	// acquire an image from swap chain
	thread_local uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_device,
		m_swapChain,
		std::numeric_limits<uint64_t>::max(),
		m_imageAvailableSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	l_semaphore->m_GraphicsWaitValue = l_semaphore->m_GraphicsSignalValue;
	l_semaphore->m_GraphicsSignalValue = l_semaphore->m_GraphicsWaitValue + 1;

	const uint64_t signalSemaphoreValues[2] = {
		l_semaphore->m_GraphicsSignalValue,
		0 // ignored for the swapchain
	};
	const VkSemaphore signalSemaphores[2] = {
		l_semaphore->m_GraphicsSemaphore,
		m_swapChainRenderedSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame]};

	VkTimelineSemaphoreSubmitInfo timelineInfo = {};
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.signalSemaphoreValueCount = 2;
	timelineInfo.pSignalSemaphoreValues = signalSemaphoreValues;

	VkSubmitInfo l_submitInfo = {};
	l_submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	l_submitInfo.pNext = &timelineInfo;
	l_submitInfo.commandBufferCount = 1;
	l_submitInfo.pCommandBuffers = &l_commandList->m_GraphicsCommandBuffer;
	l_submitInfo.waitSemaphoreCount = 1;
	l_submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame];
	l_submitInfo.signalSemaphoreCount = 2;
	l_submitInfo.pSignalSemaphores = &signalSemaphores[0];

	VkPipelineStageFlags waitDstStageMask[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};
	l_submitInfo.pWaitDstStageMask = &waitDstStageMask[0];

	VkQueue &queue = m_graphicsQueue;
	VkFence &fence = m_graphicsQueueFence;

	vkResetFences(m_device, 1, &fence);
	if (vkQueueSubmit(queue, 1, &l_submitInfo, fence) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to submit command buffer for the swap chain RenderPassComp!");
		return false;
	}

	// present the swap chain image to the front screen
	VkTimelineSemaphoreSubmitInfo swapChainTimelineInfo = {};
	swapChainTimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	swapChainTimelineInfo.pNext = NULL;
	swapChainTimelineInfo.waitSemaphoreValueCount = 1;
	swapChainTimelineInfo.pWaitSemaphoreValues = &l_semaphore->m_GraphicsWaitValue;

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_swapChainRenderedSemaphores[m_SwapChainRenderPassComp->m_CurrentFrame];

	// swap chain
	VkSwapchainKHR swapChains[] = {m_swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);

	m_SwapChainRenderPassComp->m_CurrentFrame = imageIndex;

	return true;
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

bool VKRenderingServer::GenerateMipmap(TextureComponent *rhs)
{
	return true;
}

bool VKRenderingServer::Resize()
{
	return true;
}

bool VKRenderingServer::BeginCapture()
{
	return false;
}

bool VKRenderingServer::EndCapture()
{
	return false;
}

void *VKRenderingServer::GetVkInstance()
{
	return m_instance;
}

void *VKRenderingServer::GetVkSurface()
{
	return &m_windowSurface;
}