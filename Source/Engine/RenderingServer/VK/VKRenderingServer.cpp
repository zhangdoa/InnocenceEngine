#include "VKRenderingServer.h"
#include "../../Component/VKMeshDataComponent.h"
#include "../../Component/VKTextureDataComponent.h"
#include "../../Component/VKMaterialDataComponent.h"
#include "../../Component/VKRenderPassDataComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerDataComponent.h"
#include "../../Component/VKGPUBufferDataComponent.h"

#include "VKHelper.h"

using namespace VKHelper;

#include "../CommonFunctionDefinationMacro.inl"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"

namespace VKRenderingServerNS
{
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pCallback);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks * pAllocator);
	std::vector<const char*> getRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Validation Layer: ", pCallbackData->pMessage);
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

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_CommandQueuePool = 0;
	IObjectPool* m_CommandListPool = 0;
	IObjectPool* m_SemaphorePool = 0;
	IObjectPool* m_FencePool = 0;
	IObjectPool* m_ShaderProgramComponentPool = 0;
	IObjectPool* m_SamplerDataComponentPool = 0;
	IObjectPool* m_GPUBufferDataComponentPool = 0;

	std::unordered_set<MeshDataComponent*> m_initializedMeshes;
	std::unordered_set<TextureDataComponent*> m_initializedTextures;
	std::unordered_set<MaterialDataComponent*> m_initializedMaterials;

	VkInstance m_instance;
	VkSurfaceKHR m_windowSurface;
	VkSurfaceFormatKHR m_windowSurfaceFormat;
	VkExtent2D m_windowSurfaceExtent;
	std::vector<VkImage> m_swapChainImages;
	VkQueue m_presentQueue;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkCommandPool m_commandPool;

	VkSwapchainKHR m_swapChain = 0;

	const std::vector<const char*> m_deviceExtensions =
	{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef _DEBUG
	const bool m_enableValidationLayers = true;
#else
	const bool m_enableValidationLayers = false;
#endif

	const std::vector<const char*> m_validationLayers =
	{
	"VK_LAYER_LUNARG_standard_validation"
	};

	VkDebugUtilsMessengerEXT m_messengerCallback;

	VkVertexInputBindingDescription m_vertexBindingDescription;
	std::array<VkVertexInputAttributeDescription, 5> m_vertexAttributeDescriptions;

	VkDescriptorPool m_materialDescriptorPool;
	VkDescriptorSetLayout m_materialDescriptorLayout;

	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceMemory m_indexBufferMemory;
	VkDeviceMemory m_textureImageMemory;

	IResourceBinder* m_userPipelineOutput = 0;
	VKRenderPassDataComponent* m_SwapChainRPDC = 0;
	VKShaderProgramComponent* m_SwapChainSPC = 0;
	VKSamplerDataComponent* m_SwapChainSDC = 0;
}

VkResult VKRenderingServerNS::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKRenderingServerNS::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

std::vector<const char*> VKRenderingServerNS::getRequiredExtensions()
{
#if defined INNO_PLATFORM_WIN
	std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#elif  defined INNO_PLATFORM_MAC
	std::vector<const char*> extensions = { "VK_KHR_surface", "VK_MVK_macos_surface" };
#elif  defined INNO_PLATFORM_LINUX
	std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_xcb_surface" };
#endif

	if (m_enableValidationLayers)
	{
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VKRenderingServerNS::createVkInstance()
{
	// check support for validation layer
	if (m_enableValidationLayers && !checkValidationLayerSupport(m_validationLayers))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo l_appInfo = {};
	l_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_appInfo.pApplicationName = g_pModuleManager->getApplicationName().c_str();
	l_appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 7);
	l_appInfo.pEngineName = "Innocence Engine";
	l_appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 7);
	l_appInfo.apiVersion = VK_API_VERSION_1_0;

	// set Vulkan instance create info with app info
	VkInstanceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	l_createInfo.pApplicationInfo = &l_appInfo;

	// set window extension info
	auto extensions = getRequiredExtensions();
	l_createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	l_createInfo.ppEnabledExtensionNames = extensions.data();

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

	if (l_deviceCount == 0) {
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	std::vector<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, l_devices.data());

	for (const auto& device : l_devices)
	{
		if (isDeviceSuitable(device, m_windowSurface, m_deviceExtensions))
		{
			m_physicalDevice = device;
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
	QueueFamilyIndices l_indices = findQueueFamilies(m_physicalDevice, m_windowSurface);

	std::vector<VkDeviceQueueCreateInfo> l_queueCreateInfos;
	std::set<uint32_t> l_uniqueQueueFamilies = { l_indices.m_graphicsFamily.value(), l_indices.m_presentFamily.value() };

	float l_queuePriority = 1.0f;
	for (uint32_t queueFamily : l_uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo l_queueCreateInfo = {};
		l_queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		l_queueCreateInfo.queueFamilyIndex = queueFamily;
		l_queueCreateInfo.queueCount = 1;
		l_queueCreateInfo.pQueuePriorities = &l_queuePriority;
		l_queueCreateInfos.push_back(l_queueCreateInfo);
	}

	VkPhysicalDeviceFeatures l_deviceFeatures = {};
	l_deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	l_createInfo.queueCreateInfoCount = static_cast<uint32_t>(l_queueCreateInfos.size());
	l_createInfo.pQueueCreateInfos = l_queueCreateInfos.data();

	l_createInfo.pEnabledFeatures = &l_deviceFeatures;

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
	samplerInfo.maxAnisotropy = 16;
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
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 5;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = { l_descriptorPoolSize };

	if (!createDescriptorPool(m_device, l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorPool for material!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDescriptorPool for material has been created.");

	std::vector<VkDescriptorSetLayoutBinding> l_textureLayoutBindings(5);
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

	return true;
}

bool VKRenderingServerNS::createGlobalCommandPool()
{
	return createCommandPool(m_physicalDevice, m_windowSurface, m_device, m_commandPool);
}

bool VKRenderingServerNS::createSwapChain()
{
	// choose device supported formats, modes and maximum back buffers
	SwapChainSupportDetails l_swapChainSupport = querySwapChainSupport(m_physicalDevice, m_windowSurface);

	m_windowSurfaceFormat = chooseSwapSurfaceFormat(l_swapChainSupport.m_formats);
	VkPresentModeKHR l_presentMode = chooseSwapPresentMode(l_swapChainSupport.m_presentModes);
	m_windowSurfaceExtent = chooseSwapExtent(l_swapChainSupport.m_capabilities);

	uint32_t l_imageCount = l_swapChainSupport.m_capabilities.minImageCount + 1;
	if (l_swapChainSupport.m_capabilities.maxImageCount > 0 && l_imageCount > l_swapChainSupport.m_capabilities.maxImageCount)
	{
		l_imageCount = l_swapChainSupport.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	l_createInfo.surface = m_windowSurface;
	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = m_windowSurfaceFormat.format;
	l_createInfo.imageColorSpace = m_windowSurfaceFormat.colorSpace;
	l_createInfo.imageExtent = m_windowSurfaceExtent;
	l_createInfo.imageArrayLayers = 1;
	l_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices l_indices = findQueueFamilies(m_physicalDevice, m_windowSurface);
	uint32_t l_queueFamilyIndices[] = { l_indices.m_graphicsFamily.value(), l_indices.m_presentFamily.value() };

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

VKResourceBinder* addResourcesBinder()
{
	auto l_BinderRawPtr = m_ResourcesBinderPool->Spawn();
	auto l_Binder = new(l_BinderRawPtr)VKResourceBinder();
	return l_Binder;
}

VKPipelineStateObject* addPSO()
{
	auto l_PSORawPtr = m_PSOPool->Spawn();
	auto l_PSO = new(l_PSORawPtr)VKPipelineStateObject();
	return l_PSO;
}

VKCommandQueue* addCommandQueue()
{
	auto l_commandQueueRawPtr = m_CommandQueuePool->Spawn();
	auto l_commandQueue = new(l_commandQueueRawPtr)VKCommandQueue();
	return l_commandQueue;
}

VKCommandList* addCommandList()
{
	auto l_commandListRawPtr = m_CommandListPool->Spawn();
	auto l_commandList = new(l_commandListRawPtr)VKCommandList();
	return l_commandList;
}

VKSemaphore* addSemaphore()
{
	auto l_semaphoreRawPtr = m_SemaphorePool->Spawn();
	auto l_semaphore = new(l_semaphoreRawPtr)VKSemaphore();
	return l_semaphore;
}

VKFence* addFence()
{
	auto l_fenceRawPtr = m_FencePool->Spawn();
	auto l_fence = new(l_fenceRawPtr)VKFence();
	return l_fence;
}

bool VKRenderingServer::Setup()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_MeshDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKMeshDataComponent), l_renderingCapability.maxMeshes);
	m_TextureDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKTextureDataComponent), l_renderingCapability.maxTextures);
	m_MaterialDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKMaterialDataComponent), l_renderingCapability.maxMaterials);
	m_RenderPassDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKRenderPassDataComponent), 128);
	m_ResourcesBinderPool = InnoMemory::CreateObjectPool(sizeof(VKResourceBinder), 16384);
	m_PSOPool = InnoMemory::CreateObjectPool(sizeof(VKPipelineStateObject), 128);
	m_CommandQueuePool = InnoMemory::CreateObjectPool(sizeof(VKCommandQueue), 128);
	m_CommandListPool = InnoMemory::CreateObjectPool(sizeof(VKCommandList), 256);
	m_SemaphorePool = InnoMemory::CreateObjectPool(sizeof(VKSemaphore), 512);
	m_FencePool = InnoMemory::CreateObjectPool(sizeof(VKFence), 256);
	m_ShaderProgramComponentPool = InnoMemory::CreateObjectPool(sizeof(VKShaderProgramComponent), 256);
	m_SamplerDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKSamplerDataComponent), 256);
	m_GPUBufferDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKGPUBufferDataComponent), 256);

	m_SwapChainRPDC = reinterpret_cast<VKRenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<VKShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<VKSamplerDataComponent*>(AddSamplerDataComponent("SwapChain/"));

	createVkInstance();
	createDebugCallback();

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer setup finished.");

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

bool VKRenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMeshDataComponent*>(rhs);
	VkDeviceSize l_bufferSize = sizeof(Vertex) * l_rhs->m_vertices.size();

	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;
	createBuffer(m_physicalDevice, m_device, l_bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, l_stagingBuffer, l_stagingBufferMemory);

	void* l_mappedVerticesMemory;
	vkMapMemory(m_device, l_stagingBufferMemory, 0, l_bufferSize, 0, &l_mappedVerticesMemory);
	std::memcpy(l_mappedVerticesMemory, &l_rhs->m_vertices[0], (size_t)l_bufferSize);
	vkUnmapMemory(m_device, l_stagingBufferMemory);

	createBuffer(m_physicalDevice, m_device, l_bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, l_rhs->m_VBO, m_vertexBufferMemory);

	copyBuffer(m_device, m_commandPool, m_graphicsQueue, l_stagingBuffer, l_rhs->m_VBO, l_bufferSize);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "VKRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	l_bufferSize = sizeof(Index) * l_rhs->m_indices.size();

	createBuffer(m_physicalDevice, m_device, l_bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, l_stagingBuffer, l_stagingBufferMemory);

	void* l_mappedIndicesMemory;
	vkMapMemory(m_device, l_stagingBufferMemory, 0, l_bufferSize, 0, &l_mappedIndicesMemory);
	std::memcpy(l_mappedIndicesMemory, &l_rhs->m_indices[0], (size_t)l_bufferSize);
	vkUnmapMemory(m_device, l_stagingBufferMemory);

	createBuffer(m_physicalDevice, m_device, l_bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, l_rhs->m_IBO, m_indexBufferMemory);

	copyBuffer(m_device, m_commandPool, m_graphicsQueue, l_stagingBuffer, l_rhs->m_IBO, l_bufferSize);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "VKRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKTextureDataComponent*>(rhs);
	l_rhs->m_VKTextureDataDesc = getVKTextureDataDesc(rhs->m_textureDataDesc);
	l_rhs->m_ImageCreateInfo = getImageCreateInfo(rhs->m_textureDataDesc, l_rhs->m_VKTextureDataDesc);

	VkBuffer l_stagingBuffer;
	VkDeviceMemory l_stagingBufferMemory;
	createBuffer(m_physicalDevice, m_device, l_rhs->m_VKTextureDataDesc.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, l_stagingBuffer, l_stagingBufferMemory);

	if (l_rhs->m_textureData != nullptr)
	{
		void* l_dstData;
		vkMapMemory(m_device, l_stagingBufferMemory, 0, l_rhs->m_VKTextureDataDesc.imageSize, 0, &l_dstData);
		std::memcpy(l_dstData, l_rhs->m_textureData, static_cast<size_t>(l_rhs->m_VKTextureDataDesc.imageSize));
		vkUnmapMemory(m_device, l_stagingBufferMemory);
	}

	if (vkCreateImage(m_device, &l_rhs->m_ImageCreateInfo, nullptr, &l_rhs->m_image) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "VKRenderingServer: Failed to create VkImage!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, l_rhs->m_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureImageMemory) != VK_SUCCESS)
	{
		g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "VKRenderingServer: Failed to allocate VkDeviceMemory for VkImage!");
		return false;
	}

	vkBindImageMemory(m_device, l_rhs->m_image, m_textureImageMemory, 0);

	VkCommandBuffer l_commandBuffer = beginSingleTimeCommands(m_device, m_commandPool);

	if (rhs->m_textureDataDesc.UsageType == TextureUsageType::ColorAttachment)
	{
		transitionImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		transitionImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else if (rhs->m_textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		transitionImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	else
	{
		transitionImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		if (l_rhs->m_textureData != nullptr)
		{
			copyBufferToImage(l_commandBuffer, l_stagingBuffer, l_rhs->m_image, l_rhs->m_VKTextureDataDesc.aspectFlags, static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.width), static_cast<uint32_t>(l_rhs->m_ImageCreateInfo.extent.height));
		}
		transitionImageLayout(l_commandBuffer, l_rhs->m_image, l_rhs->m_ImageCreateInfo.format, l_rhs->m_VKTextureDataDesc.aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, l_commandBuffer);

	vkDestroyBuffer(m_device, l_stagingBuffer, nullptr);
	vkFreeMemory(m_device, l_stagingBufferMemory, nullptr);

	createImageView(m_device, l_rhs);

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Image;
	l_rhs->m_ResourceBinder = l_resourceBinder;

	g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "VKRenderingServer: VkImage ", l_rhs->m_image, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<VKMaterialDataComponent*>(rhs);
	l_rhs->m_ResourceBinders.resize(5);

	auto f_createWriteDescriptorSet = [&](TextureDataComponent* texture, VkDescriptorImageInfo& imageInfo, uint32_t dstBinding)
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = reinterpret_cast<VKTextureDataComponent*>(texture)->m_imageView;

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstBinding = dstBinding;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = &imageInfo;
		writeDescriptorSet.dstSet = l_rhs->m_descriptorSet;

		return writeDescriptorSet;
	};

	createDescriptorSets(
		m_device,
		m_materialDescriptorPool,
		m_materialDescriptorLayout,
		l_rhs->m_descriptorSet,
		1);

	l_rhs->m_descriptorImageInfos.resize(5);
	l_rhs->m_writeDescriptorSets.resize(5);

	if (l_rhs->m_normalTexture)
	{
		InitializeTextureDataComponent(reinterpret_cast<VKTextureDataComponent*>(l_rhs->m_normalTexture));
		l_rhs->m_writeDescriptorSets[0] = f_createWriteDescriptorSet(l_rhs->m_normalTexture, l_rhs->m_descriptorImageInfos[0], 0);
	}
	else
	{
		auto l_defaultTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Normal);
		l_rhs->m_writeDescriptorSets[0] = f_createWriteDescriptorSet(l_defaultTexture, l_rhs->m_descriptorImageInfos[0], 0);
	}
	if (l_rhs->m_albedoTexture)
	{
		InitializeTextureDataComponent(reinterpret_cast<VKTextureDataComponent*>(l_rhs->m_albedoTexture));
		l_rhs->m_writeDescriptorSets[1] = f_createWriteDescriptorSet(l_rhs->m_albedoTexture, l_rhs->m_descriptorImageInfos[1], 1);
	}
	else
	{
		auto l_defaultTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Albedo);
		l_rhs->m_writeDescriptorSets[1] = f_createWriteDescriptorSet(l_defaultTexture, l_rhs->m_descriptorImageInfos[1], 1);
	}
	if (l_rhs->m_metallicTexture)
	{
		InitializeTextureDataComponent(reinterpret_cast<VKTextureDataComponent*>(l_rhs->m_metallicTexture));
		l_rhs->m_writeDescriptorSets[2] = f_createWriteDescriptorSet(l_rhs->m_metallicTexture, l_rhs->m_descriptorImageInfos[2], 2);
	}
	else
	{
		auto l_defaultTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Metallic);
		l_rhs->m_writeDescriptorSets[2] = f_createWriteDescriptorSet(l_defaultTexture, l_rhs->m_descriptorImageInfos[2], 2);
	}
	if (l_rhs->m_roughnessTexture)
	{
		InitializeTextureDataComponent(reinterpret_cast<VKTextureDataComponent*>(l_rhs->m_roughnessTexture));
		l_rhs->m_writeDescriptorSets[3] = f_createWriteDescriptorSet(l_rhs->m_roughnessTexture, l_rhs->m_descriptorImageInfos[3], 3);
	}
	else
	{
		auto l_defaultTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::Roughness);

		l_rhs->m_writeDescriptorSets[3] = f_createWriteDescriptorSet(l_defaultTexture, l_rhs->m_descriptorImageInfos[3], 3);
	}
	if (l_rhs->m_aoTexture)
	{
		InitializeTextureDataComponent(reinterpret_cast<VKTextureDataComponent*>(l_rhs->m_aoTexture));
		l_rhs->m_writeDescriptorSets[4] = f_createWriteDescriptorSet(l_rhs->m_aoTexture, l_rhs->m_descriptorImageInfos[4], 4);
	}
	else
	{
		auto l_defaultTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(TextureUsageType::AmbientOcclusion);
		l_rhs->m_writeDescriptorSets[4] = f_createWriteDescriptorSet(l_defaultTexture, l_rhs->m_descriptorImageInfos[4], 4);
	}

	updateDescriptorSet(m_device, &l_rhs->m_writeDescriptorSets[0], (uint32_t)l_rhs->m_writeDescriptorSets.size());

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool VKRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<VKRenderPassDataComponent*>(rhs);

	bool l_result = true;

	l_result &= reserveRenderTargets(l_rhs, this);

	l_result &= createRenderTargets(l_rhs, this);

	l_rhs->m_PipelineStateObject = addPSO();

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		l_result &= createRenderPass(m_device, l_rhs);

		if (l_rhs->m_RenderPassDesc.m_UseMultiFrames)
		{
			l_result &= createMultipleFramebuffers(m_device, l_rhs);
		}
		else
		{
			l_result &= createSingleFramebuffer(m_device, l_rhs);
		}
	}

	l_result &= createDescriptorSetLayoutBindings(l_rhs);

	l_result &= createDescriptorSetLayout(m_device, l_rhs->m_DescriptorSetLayoutBindings.data(), static_cast<uint32_t>(l_rhs->m_DescriptorSetLayoutBindings.size()), l_rhs->m_DescriptorSetLayout);

	l_result &= createPipelineLayout(m_device, l_rhs);

	if (l_rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		l_result &= createGraphicsPipelines(m_device, l_rhs);
	}
	else
	{
		l_result &= createComputePipelines(m_device, l_rhs);
	}

	l_result &= createCommandPool(m_physicalDevice, m_windowSurface, m_device, l_rhs->m_CommandPool);

	l_rhs->m_CommandLists.resize(l_rhs->m_Framebuffers.size());
	for (size_t i = 0; i < l_rhs->m_CommandLists.size(); i++)
	{
		l_rhs->m_CommandLists[i] = addCommandList();
	}

	l_result &= createCommandBuffers(m_device, l_rhs);

	l_rhs->m_SignalSemaphores.resize(l_rhs->m_Framebuffers.size());
	l_rhs->m_WaitSemaphores.resize(l_rhs->m_Framebuffers.size());
	l_rhs->m_Fences.resize(l_rhs->m_Framebuffers.size());

	for (size_t i = 0; i < l_rhs->m_SignalSemaphores.size(); i++)
	{
		l_rhs->m_SignalSemaphores[i] = addSemaphore();
		l_rhs->m_WaitSemaphores[i] = addSemaphore();
		l_rhs->m_Fences[i] = addFence();
	}

	l_result &= createSyncPrimitives(m_device, l_rhs);

	return l_result;
}

bool VKRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<VKShaderProgramComponent*>(rhs);

	bool l_result = true;

	l_rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	l_rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 1;
	l_rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size());
	l_rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_vertexBindingDescription;
	l_rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_VSHandle, l_rhs->m_ShaderFilePaths.m_VSPath);
		l_rhs->m_VSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_VSCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_rhs->m_VSCInfo.module = l_rhs->m_VSHandle;
		l_rhs->m_VSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_HSHandle, l_rhs->m_ShaderFilePaths.m_HSPath);
		l_rhs->m_HSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_HSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		l_rhs->m_HSCInfo.module = l_rhs->m_HSHandle;
		l_rhs->m_HSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_DSHandle, l_rhs->m_ShaderFilePaths.m_DSPath);
		l_rhs->m_DSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_DSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		l_rhs->m_DSCInfo.module = l_rhs->m_DSHandle;
		l_rhs->m_DSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_GSHandle, l_rhs->m_ShaderFilePaths.m_GSPath);
		l_rhs->m_GSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_GSCInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		l_rhs->m_GSCInfo.module = l_rhs->m_GSHandle;
		l_rhs->m_GSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_PSHandle, l_rhs->m_ShaderFilePaths.m_PSPath);
		l_rhs->m_PSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_PSCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_rhs->m_PSCInfo.module = l_rhs->m_PSHandle;
		l_rhs->m_PSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		l_result &= createShaderModule(m_device, l_rhs->m_CSHandle, l_rhs->m_ShaderFilePaths.m_CSPath);
		l_rhs->m_CSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_CSCInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		l_rhs->m_CSCInfo.module = l_rhs->m_CSHandle;
		l_rhs->m_CSCInfo.pName = "main";
	}

	return l_result;
}

bool VKRenderingServer::InitializeSamplerDataComponent(SamplerDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteSamplerDataComponent(SamplerDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue, size_t startOffset, size_t range)
{
	return true;
}

bool VKRenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool VKRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool VKRenderingServer::DispatchDrawCall(RenderPassDataComponent * renderPass, MeshDataComponent* mesh, size_t instanceCount)
{
	return true;
}

bool VKRenderingServer::DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool VKRenderingServer::CommandListEnd(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::WaitForFrame(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::SetUserPipelineOutput(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::Present()
{
	return true;
}

bool VKRenderingServer::DispatchCompute(RenderPassDataComponent * renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return true;
}

bool VKRenderingServer::CopyDepthStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool VKRenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

Vec4 VKRenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> VKRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<Vec4>();
}

bool VKRenderingServer::Resize()
{
	return true;
}

void * VKRenderingServer::GetVkInstance()
{
	return m_instance;
}

void * VKRenderingServer::GetVkSurface()
{
	return &m_windowSurface;
}