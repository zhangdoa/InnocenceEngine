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
	bool createMaterialDescriptorPool();
	bool createCommandPool();

	bool createSwapChain();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_CommandQueuePool = 0;
	IObjectPool* m_CommandListPool = 0;
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
			m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
	//	m_objectStatus = ObjectStatus::Suspended;
	//	InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkSampler for deferred pass render target sampling!");
	//	return false;
	//}

	//InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingServerNS::createMaterialDescriptorPool()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 5;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = { l_descriptorPoolSize };

	if (!createDescriptorPool(m_device, l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
	{
		m_objectStatus = ObjectStatus::Suspended;
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
		l_textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_textureLayoutBinding.pImmutableSamplers = nullptr;
		l_textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_textureLayoutBindings[i] = l_textureLayoutBinding;
	}

	if (!createDescriptorSetLayout(m_device, &l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), m_materialDescriptorLayout))
	{
		m_objectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkDescriptorSetLayout for material has been created.");

	return true;
}

bool VKRenderingServerNS::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice, m_windowSurface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create CommandPool!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: CommandPool has been created.");
	return true;
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
		m_objectStatus = ObjectStatus::Suspended;
		InnoLogger::Log(LogLevel::Error, "VKRenderingServer: Failed to create VkSwapChainKHR!");
		return false;
	}

	InnoLogger::Log(LogLevel::Success, "VKRenderingServer: VkSwapChainKHR has been created.");

	// get swap chain VkImages
	// get count
	if (vkGetSwapchainImagesKHR(m_device, m_swapChain, &l_imageCount, nullptr) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::Suspended;
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
		m_objectStatus = ObjectStatus::Suspended;
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
	m_FencePool = InnoMemory::CreateObjectPool(sizeof(VKFence), 256);
	m_ShaderProgramComponentPool = InnoMemory::CreateObjectPool(sizeof(VKShaderProgramComponent), 256);
	m_SamplerDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKSamplerDataComponent), 256);
	m_GPUBufferDataComponentPool = InnoMemory::CreateObjectPool(sizeof(VKGPUBufferDataComponent), 256);

	m_SwapChainRPDC = reinterpret_cast<VKRenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<VKShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<VKSamplerDataComponent*>(AddSamplerDataComponent("SwapChain/"));

	m_objectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer setup finished.");

	return true;
}

bool VKRenderingServer::Initialize()
{
	return true;
}

bool VKRenderingServer::Terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "VKRenderingServer has been terminated.");

	return true;
}

ObjectStatus VKRenderingServer::GetStatus()
{
	return m_objectStatus;
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
	return true;
}

bool VKRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
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

bool VKRenderingServer::SetUserPipelineOutput(IResourceBinder* resourceBinder)
{
	return nullptr;
}

bool VKRenderingServer::Present()
{
	return true;
}

bool VKRenderingServer::DispatchCompute(RenderPassDataComponent * renderPass, unsigned int threadGroupX, unsigned int threadGroupY, unsigned int threadGroupZ)
{
	return true;
}

bool VKRenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool VKRenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
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