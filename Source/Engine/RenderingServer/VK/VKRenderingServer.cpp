#include "VKRenderingServer.h"
#include "../../Component/VKMeshDataComponent.h"
#include "../../Component/VKTextureDataComponent.h"
#include "../../Component/VKMaterialDataComponent.h"
#include "../../Component/VKRenderPassDataComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerDataComponent.h"
#include "../../Component/VKGPUBufferDataComponent.h"

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
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks *pAllocator);
	std::vector<const char *> getRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
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

	bool createVkInstance();
	bool createDebugCallback();

	bool createPysicalDevice();
	bool createLogicalDevice();

	bool createTextureSamplers();
	bool createVertexInputAttributions();
	bool createMaterialDescriptorPool();
	bool createGlobalCommandPool();

	bool createSwapChain();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	TObjectPool<VKMeshDataComponent> *m_MeshDataComponentPool = 0;
	TObjectPool<VKMaterialDataComponent> *m_MaterialDataComponentPool = 0;
	TObjectPool<VKTextureDataComponent> *m_TextureDataComponentPool = 0;
	TObjectPool<VKRenderPassDataComponent> *m_RenderPassDataComponentPool = 0;
	TObjectPool<VKPipelineStateObject> *m_PSOPool = 0;
	TObjectPool<VKCommandQueue> *m_CommandQueuePool = 0;
	TObjectPool<VKCommandList> *m_CommandListPool = 0;
	TObjectPool<VKSemaphore> *m_SemaphorePool = 0;
	TObjectPool<VKFence> *m_FencePool = 0;
	TObjectPool<VKShaderProgramComponent> *m_ShaderProgramComponentPool = 0;
	TObjectPool<VKSamplerDataComponent> *m_SamplerDataComponentPool = 0;
	TObjectPool<VKGPUBufferDataComponent> *m_GPUBufferDataComponentPool = 0;

	std::unordered_set<MeshDataComponent *> m_initializedMeshes;
	std::unordered_set<TextureDataComponent *> m_initializedTextures;
	std::unordered_set<MaterialDataComponent *> m_initializedMaterials;

	VkInstance m_instance;
	VkSurfaceKHR m_windowSurface;
	std::vector<VkImage> m_swapChainImages;
	VkQueue m_presentQueue;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkCommandPool m_commandPool;

	VkSwapchainKHR m_swapChain = 0;

	const std::vector<const char *> m_deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
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
	VKRenderPassDataComponent *m_SwapChainRPDC = 0;
	VKShaderProgramComponent *m_SwapChainSPC = 0;
	VKSamplerDataComponent *m_SwapChainSDC = 0;
} // namespace VKRenderingServerNS

VkResult VKRenderingServerNS::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback)
{
	auto l_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (l_func != nullptr)
	{
		return l_func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKRenderingServerNS::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks *pAllocator)
{
	auto l_func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (l_func != nullptr)
	{
		l_func(instance, callback, pAllocator);
	}
}

std::vector<const char *> VKRenderingServerNS::getRequiredExtensions()
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

bool VKRenderingServerNS::createVkInstance()
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
	l_appInfo.apiVersion = VK_API_VERSION_1_0;

	// set Vulkan instance create info with app info
	VkInstanceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	l_createInfo.pApplicationInfo = &l_appInfo;

	// set window extension info
	auto l_extensions = getRequiredExtensions();
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

bool VKRenderingServerNS::createDebugCallback()
{
	if (m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		l_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		l_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		l_createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(m_instance, &l_createInfo, nullptr, &m_messengerCallback) != VK_SUCCESS)
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

bool VKRenderingServerNS::createPysicalDevice()
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

bool VKRenderingServerNS::createLogicalDevice()
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

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDevice has been created.");
	return true;
}

bool VKRenderingServerNS::createTextureSamplers()
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

	//if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_deferredRTSampler) != VK_SUCCESS)
	//{
	//	m_ObjectStatus = ObjectStatus::Suspended;
	//	InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkSampler for deferred pass render target sampling!");
	//	return false;
	//}

	//InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingServerNS::createVertexInputAttributions()
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

bool VKRenderingServerNS::createMaterialDescriptorPool()
{
	auto l_renderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 8;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = {l_descriptorPoolSize};

	if (!createDescriptorPool(m_device, l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
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

	if (!createDescriptorSetLayout(m_device, &l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), m_materialDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDescriptorSetLayout for material has been created.");

	if (!createDescriptorSetLayout(m_device, nullptr, 0, m_dummyEmptyDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create DummyEmptyDescriptorLayout!");
		return false;
	}

	return true;
}

bool VKRenderingServerNS::createGlobalCommandPool()
{
	return CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, m_commandPool);
}

bool VKRenderingServerNS::createSwapChain()
{
	// choose device supported formats, modes and maximum back buffers
	auto l_swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_windowSurface);
	auto l_windowSurfaceExtent = ChooseSwapExtent(l_swapChainSupport.m_capabilities);
	auto l_windowSurfaceFormat = ChooseSwapSurfaceFormat(l_swapChainSupport.m_formats);
	auto l_presentMode = ChooseSwapPresentMode(l_swapChainSupport.m_presentModes);

	uint32_t l_imageCount = l_swapChainSupport.m_capabilities.minImageCount + 1;
	if (l_swapChainSupport.m_capabilities.maxImageCount > 0 && l_imageCount > l_swapChainSupport.m_capabilities.maxImageCount)
	{
		l_imageCount = l_swapChainSupport.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	l_createInfo.surface = m_windowSurface;
	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = l_windowSurfaceFormat.format;
	l_createInfo.imageColorSpace = l_windowSurfaceFormat.colorSpace;
	l_createInfo.imageExtent = l_windowSurfaceExtent;
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

VKCommandQueue *addCommandQueue()
{
	return m_CommandQueuePool->Spawn();
}

VKCommandList *addCommandList()
{
	return m_CommandListPool->Spawn();
}

VKSemaphore *addSemaphore()
{
	return m_SemaphorePool->Spawn();
}

VKFence *addFence()
{
	return m_FencePool->Spawn();
}

bool VKRenderingServer::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	m_MeshDataComponentPool = TObjectPool<VKMeshDataComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureDataComponentPool = TObjectPool<VKTextureDataComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialDataComponentPool = TObjectPool<VKMaterialDataComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassDataComponentPool = TObjectPool<VKRenderPassDataComponent>::Create(128);
	m_PSOPool = TObjectPool<VKPipelineStateObject>::Create(128);
	m_CommandQueuePool = TObjectPool<VKCommandQueue>::Create(128);
	m_CommandListPool = TObjectPool<VKCommandList>::Create(256);
	m_SemaphorePool = TObjectPool<VKSemaphore>::Create(512);
	m_FencePool = TObjectPool<VKFence>::Create(256);
	m_ShaderProgramComponentPool = TObjectPool<VKShaderProgramComponent>::Create(256);
	m_SamplerDataComponentPool = TObjectPool<VKSamplerDataComponent>::Create(256);
	m_GPUBufferDataComponentPool = TObjectPool<VKGPUBufferDataComponent>::Create(256);

	m_SwapChainRPDC = reinterpret_cast<VKRenderPassDataComponent *>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<VKShaderProgramComponent *>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<VKSamplerDataComponent *>(AddSamplerDataComponent("SwapChain/"));

	createVkInstance();
	createDebugCallback();

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer Setup finished.");

	return true;
}

bool VKRenderingServer::Initialize()
{
	createPysicalDevice();
	createLogicalDevice();

	createVertexInputAttributions();
	createTextureSamplers();
	createMaterialDescriptorPool();
	createGlobalCommandPool();

	createSwapChain();

	return true;
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

AddComponent(VK, MeshData);
AddComponent(VK, TextureData);
AddComponent(VK, MaterialData);
AddComponent(VK, RenderPassData);
AddComponent(VK, ShaderProgram);
AddComponent(VK, SamplerData);
AddComponent(VK, GPUBufferData);

bool createHostStagingBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBufferCreateInfo l_stagingBufferCInfo = {};
	l_stagingBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_stagingBufferCInfo.size = bufferSize;
	l_stagingBufferCInfo.usage = usageFlags;
	l_stagingBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(m_physicalDevice, m_device, l_stagingBufferCInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMemory);
}

bool createDeviceLocalBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBufferCreateInfo l_localBufferCInfo = {};
	l_localBufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_localBufferCInfo.size = bufferSize;
	l_localBufferCInfo.usage = usageFlags;
	l_localBufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	return CreateBuffer(m_physicalDevice, m_device, l_localBufferCInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deviceMemory);
}

bool copyHostMemoryToDeviceMemory(void *hostMemory, size_t bufferSize, VkDeviceMemory &deviceMemory)
{
	void *l_mappedMemory;
	vkMapMemory(m_device, deviceMemory, 0, bufferSize, 0, &l_mappedMemory);
	std::memcpy(l_mappedMemory, hostMemory, (size_t)bufferSize);
	vkUnmapMemory(m_device, deviceMemory);

	return true;
}
bool initializeDeviceLocalBuffer(void *hostMemory, size_t bufferSize, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;

	createHostStagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

	copyHostMemoryToDeviceMemory(hostMemory, bufferSize, l_stagingBufferMemory);

	CopyBuffer(m_device, m_commandPool, m_graphicsQueue, l_stagingBuffer, buffer, bufferSize);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	return true;
}

template <typename T>
bool initializeDeviceLocalBuffer(Array<T> &hostMemory, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
	auto l_bufferSize = sizeof(T) * hostMemory.size();

	return initializeDeviceLocalBuffer(&hostMemory[0], l_bufferSize, buffer, deviceMemory);
}

bool VKRenderingServer::InitializeMeshDataComponent(MeshDataComponent *rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMeshDataComponent *>(rhs);

	auto l_VBSize = sizeof(Vertex) * l_rhs->m_vertices.size();
	auto l_IBSize = sizeof(Index) * l_rhs->m_indices.size();

	createDeviceLocalBuffer(l_VBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_VBO, l_rhs->m_VBMemory);
	createDeviceLocalBuffer(l_IBSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_IBO, l_rhs->m_IBMemory);

	initializeDeviceLocalBuffer(l_rhs->m_vertices, l_rhs->m_VBO, l_rhs->m_VBMemory);
	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	initializeDeviceLocalBuffer(l_rhs->m_indices, l_rhs->m_IBO, l_rhs->m_IBMemory);
	InnoLogger::Log(LogLevel::Verbose, "VKRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeTextureDataComponent(TextureDataComponent *rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKTextureDataComponent *>(rhs);
	l_rhs->m_VKTextureDesc = GetVKTextureDesc(rhs->m_TextureDesc);
	l_rhs->m_ImageCreateInfo = GetImageCreateInfo(rhs->m_TextureDesc, l_rhs->m_VKTextureDesc);

	CreateImage(m_physicalDevice, m_device, l_rhs->m_ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, l_rhs->m_image, l_rhs->m_imageMemory);

	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;
	if (l_rhs->m_TextureData != nullptr)
	{
		createHostStagingBuffer(l_rhs->m_VKTextureDesc.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, l_stagingBuffer, l_stagingBufferMemory);

		void *l_mappedMemory;
		vkMapMemory(m_device, l_stagingBufferMemory, 0, l_rhs->m_VKTextureDesc.imageSize, 0, &l_mappedMemory);
		std::memcpy(l_mappedMemory, l_rhs->m_TextureData, static_cast<size_t>(l_rhs->m_VKTextureDesc.imageSize));
		vkUnmapMemory(m_device, l_stagingBufferMemory);
	}

	VkCommandBuffer l_commandBuffer = OpenTemporaryCommandBuffer(m_device, m_commandPool);

	if (rhs->m_TextureDesc.Usage == TextureUsage::ColorAttachment)
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_TextureDesc.Usage == TextureUsage::DepthAttachment)
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_TextureDesc.Usage == TextureUsage::DepthStencilAttachment)
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else
	{
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		if (l_rhs->m_TextureData != nullptr)
		{
			copyBufferToImage(l_commandBuffer, l_stagingBuffer, l_rhs->m_image, l_rhs->m_VKTextureDesc.aspectFlags, static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.width), static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.height));
		}
		TransitImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDesc.aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	CloseTemporaryCommandBuffer(m_device, m_commandPool, m_graphicsQueue, l_commandBuffer);

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

bool VKRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent *rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMaterialDataComponent *>(rhs);

	auto l_defaultMaterial = g_Engine->getRenderingFrontend()->getDefaultMaterialDataComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<VKTextureDataComponent *>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureDataComponent(l_texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
			l_rhs->m_TextureSlots[i].m_Activate = true;
		}
		else
		{
			auto l_texture = reinterpret_cast<VKTextureDataComponent *>(l_defaultMaterial->m_TextureSlots[i].m_Texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
		}
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassDataComponent *>(rhs);

	bool l_result = true;

	l_result &= ReserveRenderTargets(l_rhs, this);

	l_result &= CreateRenderTargets(l_rhs, this);

	l_rhs->m_PipelineStateObject = addPSO();

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

	if (l_rhs->m_ResourceBinderLayoutDescs.size())
	{
		l_result &= CreateDescriptorSetLayoutBindings(l_rhs);

		auto l_descriptorLayoutsSize = l_rhs->m_DescriptorSetLayoutBindingIndices.size();
		auto l_maximumSetIndex = l_rhs->m_DescriptorSetLayoutBindingIndices[l_descriptorLayoutsSize - 1].m_SetIndex;

		l_rhs->m_DescriptorSetLayouts.resize(l_maximumSetIndex + 1);
		for (size_t i = 0; i < l_rhs->m_DescriptorSetLayouts.size(); i++)
		{
			l_rhs->m_DescriptorSetLayouts[i] = m_dummyEmptyDescriptorLayout;
		}
		l_rhs->m_DescriptorSets.resize(l_maximumSetIndex + 1);

		for (size_t i = 0; i < l_descriptorLayoutsSize; i++)
		{
			auto l_descriptorSetLayoutBindingIndex = l_rhs->m_DescriptorSetLayoutBindingIndices[i];
			l_result &= createDescriptorSetLayout(m_device,
												  &l_rhs->m_DescriptorSetLayoutBindings[l_descriptorSetLayoutBindingIndex.m_LayoutBindingOffset],
												  static_cast<uint32_t>(l_descriptorSetLayoutBindingIndex.m_BindingCount),
												  l_rhs->m_DescriptorSetLayouts[l_descriptorSetLayoutBindingIndex.m_SetIndex]);
		}
	}
	else
	{
		l_rhs->m_DescriptorSetLayouts.resize(1);
		l_rhs->m_DescriptorSetLayouts[0] = m_dummyEmptyDescriptorLayout;
		l_rhs->m_DescriptorSets.resize(1);
	}

	l_result &= createDescriptorPool(m_device, l_rhs);

	for (size_t i = 0; i < l_rhs->m_DescriptorSetLayouts.size(); i++)
	{
		l_result &= createDescriptorSets(m_device, l_rhs->m_DescriptorPool, &l_rhs->m_DescriptorSetLayouts[i], l_rhs->m_DescriptorSets[i], 1);
	}
	
	l_result &= CreatePipelineLayout(m_device, l_rhs);

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Graphics)
	{
		l_result &= CreateGraphicsPipelines(m_device, l_rhs);
	}
	else
	{
		l_result &= CreateComputePipelines(m_device, l_rhs);
	}

	l_result &= CreateCommandPool(m_physicalDevice, m_windowSurface, m_device, l_rhs->m_CommandPool);

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

	l_rhs->m_SignalSemaphores.resize(l_rhs->m_Framebuffers.size());
	l_rhs->m_WaitSemaphores.resize(l_rhs->m_Framebuffers.size());
	l_rhs->m_Fences.resize(l_rhs->m_Framebuffers.size());

	for (size_t i = 0; i < l_rhs->m_SignalSemaphores.size(); i++)
	{
		l_rhs->m_SignalSemaphores[i] = addSemaphore();
		l_rhs->m_WaitSemaphores[i] = addSemaphore();
		l_rhs->m_Fences[i] = addFence();
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

bool VKRenderingServer::InitializeSamplerDataComponent(SamplerDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKSamplerDataComponent *>(rhs);

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

bool VKRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferDataComponent *>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	createHostStagingBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_HostStagingBuffer, l_rhs->m_HostStagingMemory);

	if (l_rhs->m_InitialData != nullptr)
	{
		copyHostMemoryToDeviceMemory(l_rhs->m_InitialData, l_rhs->m_TotalSize, l_rhs->m_HostStagingMemory);
	}

	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		if (l_rhs->m_CPUAccessibility == Accessibility::Immutable || l_rhs->m_CPUAccessibility == Accessibility::WriteOnly)
		{
			createDeviceLocalBuffer(l_rhs->m_TotalSize, VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), l_rhs->m_DeviceLocalBuffer, l_rhs->m_DeviceLocalMemory);
			if (l_rhs->m_InitialData != nullptr)
			{
				CopyBuffer(m_device, m_commandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_rhs->m_TotalSize);
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

bool VKRenderingServer::DeleteMeshDataComponent(MeshDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteTextureDataComponent(TextureDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteSamplerDataComponent(SamplerDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::ClearTextureDataComponent(TextureDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::CopyTextureDataComponent(TextureDataComponent *lhs, TextureDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent *rhs, const void *GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferDataComponent *>(rhs);

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
		CopyBuffer(m_device, m_commandPool, m_graphicsQueue, l_rhs->m_HostStagingBuffer, l_rhs->m_DeviceLocalBuffer, l_size);
	}

	return true;
}

bool VKRenderingServer::ClearGPUBufferDataComponent(GPUBufferDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKGPUBufferDataComponent *>(rhs);

	auto l_commandBuffer = OpenTemporaryCommandBuffer(m_device, m_commandPool);

	vkCmdFillBuffer(l_commandBuffer, l_rhs->m_HostStagingBuffer, 0, 0, 0);
	if (l_rhs->m_GPUAccessibility != Accessibility::ReadOnly)
	{
		vkCmdFillBuffer(l_commandBuffer, l_rhs->m_DeviceLocalBuffer, 0, 0, 0);
	}

	CloseTemporaryCommandBuffer(m_device, m_commandPool, m_graphicsQueue, l_commandBuffer);

	return true;
}

bool VKRenderingServer::CommandListBegin(RenderPassDataComponent *rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<VKRenderPassDataComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);

	VkCommandBufferBeginInfo l_beginInfo = {};
	l_beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	l_beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(l_commandList->m_CommandBuffer, &l_beginInfo) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassDataComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

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

	vkCmdBeginRenderPass(l_commandList->m_CommandBuffer, &l_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Graphics)
	{
		vkCmdBindPipeline(l_commandList->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_PSO->m_Pipeline);
	}
	else if (l_rhs->m_RenderPassDesc.m_RenderPassUsage == RenderPassUsage::Compute)
	{
		vkCmdBindPipeline(l_commandList->m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, l_PSO->m_Pipeline);
	}

	return true;
}

bool VKRenderingServer::CleanRenderTargets(RenderPassDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::BindGPUResource(RenderPassDataComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassDataComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	if (resource == nullptr)
	{
		InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Empty GPU resource in render pass: ", renderPass->m_InstanceName.c_str(), ", global slot: ", globalSlot, ", local slot: ", localSlot);
		return false;
	}

	VkWriteDescriptorSet l_writeDescriptorSet = {};
	VkDescriptorImageInfo l_descriptorImageInfo = {};
	VkDescriptorBufferInfo l_descriptorBufferInfo = {};
	switch (resource->m_GPUResourceType)
	{
	case GPUResourceType::Sampler:
		l_writeDescriptorSet = GetWriteDescriptorSet((uint32_t)localSlot, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, l_renderPass->m_DescriptorSets[globalSlot]);
		UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
		break;
	case GPUResourceType::Image:	
		l_descriptorImageInfo.imageView = reinterpret_cast<VKTextureDataComponent *>(resource)->m_imageView;
		if (accessibility != Accessibility::ReadOnly)
		{
			l_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, (uint32_t)localSlot, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, l_renderPass->m_DescriptorSets[globalSlot]);
		}
		else
		{
			l_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorImageInfo, (uint32_t)localSlot, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, l_renderPass->m_DescriptorSets[globalSlot]);
		}
		UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
		break;
	case GPUResourceType::Buffer:
		if (resource->m_GPUAccessibility == Accessibility::ReadOnly)
		{
			if (accessibility != Accessibility::ReadOnly)
			{
				InnoLogger::Log(LogLevel::Warning, "VKRenderingServer: Not allow GPU write to Constant Buffer!");
			}
			else
			{
				l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferDataComponent *>(resource)->m_HostStagingBuffer;
				l_descriptorBufferInfo.offset = startOffset;
				l_descriptorBufferInfo.range = elementCount;
				l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, (uint32_t)localSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[globalSlot]);
				UpdateDescriptorSet(m_device, &l_writeDescriptorSet, 1);
			}
		}
		else
		{
			//if (accessibility != Accessibility::ReadOnly)
			//{
			VkDescriptorBufferInfo l_descriptorBufferInfo = {};
			l_descriptorBufferInfo.buffer = reinterpret_cast<VKGPUBufferDataComponent *>(resource)->m_DeviceLocalBuffer;
			l_descriptorBufferInfo.offset = startOffset;
			l_descriptorBufferInfo.range = elementCount;
			l_writeDescriptorSet = GetWriteDescriptorSet(l_descriptorBufferInfo, (uint32_t)localSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, l_renderPass->m_DescriptorSets[globalSlot]);
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

	return true;
}

bool VKRenderingServer::DrawIndexedInstanced(RenderPassDataComponent *renderPass, MeshDataComponent *mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassDataComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);
	auto l_mesh = reinterpret_cast<VKMeshDataComponent *>(mesh);

	VkBuffer vertexBuffers[] = {l_mesh->m_VBO};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(l_commandList->m_CommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(l_commandList->m_CommandBuffer, l_mesh->m_IBO, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(l_commandList->m_CommandBuffer, static_cast<uint32_t>(l_mesh->m_indicesSize), 1, 0, 0, 0);

	return true;
}

bool VKRenderingServer::DrawInstanced(RenderPassDataComponent *renderPass, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassDataComponent *>(renderPass);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_renderPass->m_CommandLists[l_renderPass->m_CurrentFrame]);

	vkCmdDraw(l_commandList->m_CommandBuffer, 1, static_cast<uint32_t>(instanceCount), 0, 0);

	return true;
}

bool VKRenderingServer::UnbindGPUResource(RenderPassDataComponent *renderPass, ShaderStage shaderStage, GPUResourceComponent *resource, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool VKRenderingServer::CommandListEnd(RenderPassDataComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassDataComponent *>(rhs);
	auto l_commandList = reinterpret_cast<VKCommandList *>(l_rhs->m_CommandLists[l_rhs->m_CurrentFrame]);
	auto l_PSO = reinterpret_cast<VKPipelineStateObject *>(l_rhs->m_PipelineStateObject);

	vkCmdEndRenderPass(l_commandList->m_CommandBuffer);

	if (vkEndCommandBuffer(l_commandList->m_CommandBuffer) != VK_SUCCESS)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to end recording command buffer!");
		return false;
	}

	return true;
}

bool VKRenderingServer::ExecuteCommandList(RenderPassDataComponent *rhs)
{
	return true;
}

bool VKRenderingServer::WaitForFrame(RenderPassDataComponent *rhs)
{
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
	return true;
}

bool VKRenderingServer::Dispatch(RenderPassDataComponent *renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return true;
}

Vec4 VKRenderingServer::ReadRenderTargetSample(RenderPassDataComponent *rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> VKRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent *canvas, TextureDataComponent *TDC)
{
	return std::vector<Vec4>();
}

bool VKRenderingServer::GenerateMipmap(TextureDataComponent *rhs)
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