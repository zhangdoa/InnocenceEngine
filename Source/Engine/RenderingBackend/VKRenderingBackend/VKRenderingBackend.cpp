#include "VKRenderingBackend.h"

#include "VKRenderingBackendUtilities.h"
#include "../../Component/VKRenderingBackendComponent.h"

#include "VKOpaquePass.h"
#include "VKLightPass.h"
#include "VKFinalBlendPass.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace VKRenderingBackendNS
{
	EntityID m_EntityID;

	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Validation Layer: " + std::string(pCallbackData->pMessage));
		return VK_FALSE;
	}

	std::vector<const char*> getRequiredExtensions()
	{
#if defined INNO_PLATFORM_WIN
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#elif  defined INNO_PLATFORM_MAC
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_MVK_macos_surface" };
#elif  defined INNO_PLATFORM_LINUX
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_xcb_surface" };
#endif

		if (VKRenderingBackendComponent::get().m_enableValidationLayers)
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool createVkInstance();
	bool createDebugCallback();

	bool createPysicalDevice();
	bool createLogicalDevice();

	bool createTextureSamplers();
	bool createMaterialDescriptorPool();
	bool createCommandPool();

	bool createSwapChain();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<VKMeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<VKMaterialDataComponent*> m_uninitializedMaterials;

	VKTextureDataComponent* m_iconTemplate_OBJ;
	VKTextureDataComponent* m_iconTemplate_PNG;
	VKTextureDataComponent* m_iconTemplate_SHADER;
	VKTextureDataComponent* m_iconTemplate_UNKNOWN;

	VKTextureDataComponent* m_iconTemplate_DirectionalLight;
	VKTextureDataComponent* m_iconTemplate_PointLight;
	VKTextureDataComponent* m_iconTemplate_SphereLight;

	VKMeshDataComponent* m_unitLineMesh;
	VKMeshDataComponent* m_unitQuadMesh;
	VKMeshDataComponent* m_unitCubeMesh;
	VKMeshDataComponent* m_unitSphereMesh;
	VKMeshDataComponent* m_terrainMesh;

	VKTextureDataComponent* m_basicNormalTexture;
	VKTextureDataComponent* m_basicAlbedoTexture;
	VKTextureDataComponent* m_basicMetallicTexture;
	VKTextureDataComponent* m_basicRoughnessTexture;
	VKTextureDataComponent* m_basicAOTexture;

	VKMaterialDataComponent* m_basicMaterial;
}

bool VKRenderingBackendNS::createVkInstance()
{
	// check support for validation layer
	if (VKRenderingBackendComponent::get().m_enableValidationLayers && !checkValidationLayerSupport()) {
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Validation layers requested, but not available!");
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

	if (VKRenderingBackendComponent::get().m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(VKRenderingBackendComponent::get().m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = VKRenderingBackendComponent::get().m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	// create Vulkan instance
	if (vkCreateInstance(&l_createInfo, nullptr, &VKRenderingBackendComponent::get().m_instance) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkInstance!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkInstance has been created.");
	return true;
}

bool VKRenderingBackendNS::createDebugCallback()
{
	if (VKRenderingBackendComponent::get().m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		l_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		l_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		l_createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(VKRenderingBackendComponent::get().m_instance, &l_createInfo, nullptr, &VKRenderingBackendComponent::get().m_messengerCallback) != VK_SUCCESS)
		{
			m_ObjectStatus = ObjectStatus::Suspended;
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create DebugUtilsMessenger!");
			return false;
		}

		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Validation Layer has been created.");
		return true;
	}
	else
	{
		return true;
	}
}

bool VKRenderingBackendNS::createPysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(VKRenderingBackendComponent::get().m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0) {
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	std::vector<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(VKRenderingBackendComponent::get().m_instance, &l_deviceCount, l_devices.data());

	for (const auto& device : l_devices)
	{
		if (isDeviceSuitable(device))
		{
			VKRenderingBackendComponent::get().m_physicalDevice = device;
			break;
		}
	}

	if (VKRenderingBackendComponent::get().m_physicalDevice == VK_NULL_HANDLE)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to find a suitable GPU!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkPhysicalDevice has been created.");
	return true;
}

bool VKRenderingBackendNS::createLogicalDevice()
{
	QueueFamilyIndices l_indices = findQueueFamilies(VKRenderingBackendComponent::get().m_physicalDevice);

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

	l_createInfo.enabledExtensionCount = static_cast<uint32_t>(VKRenderingBackendComponent::get().m_deviceExtensions.size());
	l_createInfo.ppEnabledExtensionNames = VKRenderingBackendComponent::get().m_deviceExtensions.data();

	if (VKRenderingBackendComponent::get().m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(VKRenderingBackendComponent::get().m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = VKRenderingBackendComponent::get().m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(VKRenderingBackendComponent::get().m_physicalDevice, &l_createInfo, nullptr, &VKRenderingBackendComponent::get().m_device) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDevice!");
		return false;
	}

	vkGetDeviceQueue(VKRenderingBackendComponent::get().m_device, l_indices.m_graphicsFamily.value(), 0, &VKRenderingBackendComponent::get().m_graphicsQueue);
	vkGetDeviceQueue(VKRenderingBackendComponent::get().m_device, l_indices.m_presentFamily.value(), 0, &VKRenderingBackendComponent::get().m_presentQueue);

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDevice has been created.");
	return true;
}

bool VKRenderingBackendNS::createTextureSamplers()
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

	if (vkCreateSampler(VKRenderingBackendComponent::get().m_device, &samplerInfo, nullptr, &VKRenderingBackendComponent::get().m_deferredRTSampler) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkSampler for deferred pass render target sampling!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingBackendNS::createMaterialDescriptorPool()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 5;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = { l_descriptorPoolSize };

	if (!createDescriptorPool(l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, VKRenderingBackendComponent::get().m_materialDescriptorPool))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDescriptorPool for material!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorPool for material has been created.");

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

	if (!createDescriptorSetLayout(&l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), VKRenderingBackendComponent::get().m_materialDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorSetLayout for material has been created.");

	return true;
}

bool VKRenderingBackendNS::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(VKRenderingBackendComponent::get().m_physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(VKRenderingBackendComponent::get().m_device, &poolInfo, nullptr, &VKRenderingBackendComponent::get().m_commandPool) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create CommandPool!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: CommandPool has been created.");
	return true;
}

bool VKRenderingBackendNS::createSwapChain()
{
	// choose device supported formats, modes and maximum back buffers
	SwapChainSupportDetails l_swapChainSupport = querySwapChainSupport(VKRenderingBackendComponent::get().m_physicalDevice);

	VKRenderingBackendComponent::get().m_windowSurfaceFormat = chooseSwapSurfaceFormat(l_swapChainSupport.m_formats);
	VkPresentModeKHR l_presentMode = chooseSwapPresentMode(l_swapChainSupport.m_presentModes);
	VKRenderingBackendComponent::get().m_windowSurfaceExtent = chooseSwapExtent(l_swapChainSupport.m_capabilities);

	uint32_t l_imageCount = l_swapChainSupport.m_capabilities.minImageCount + 1;
	if (l_swapChainSupport.m_capabilities.maxImageCount > 0 && l_imageCount > l_swapChainSupport.m_capabilities.maxImageCount)
	{
		l_imageCount = l_swapChainSupport.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	l_createInfo.surface = VKRenderingBackendComponent::get().m_windowSurface;
	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = VKRenderingBackendComponent::get().m_windowSurfaceFormat.format;
	l_createInfo.imageColorSpace = VKRenderingBackendComponent::get().m_windowSurfaceFormat.colorSpace;
	l_createInfo.imageExtent = VKRenderingBackendComponent::get().m_windowSurfaceExtent;
	l_createInfo.imageArrayLayers = 1;
	l_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices l_indices = findQueueFamilies(VKRenderingBackendComponent::get().m_physicalDevice);
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

	if (vkCreateSwapchainKHR(VKRenderingBackendComponent::get().m_device, &l_createInfo, nullptr, &VKRenderingBackendComponent::get().m_swapChain) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkSwapChainKHR!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkSwapChainKHR has been created.");

	// get swap chain VkImages
	// get count
	if (vkGetSwapchainImagesKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, &l_imageCount, nullptr) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to query swap chain image count!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Swap chain has " + std::to_string(l_imageCount) + " image(s).");

	VKRenderingBackendComponent::get().m_swapChainImages.reserve(l_imageCount);
	for (size_t i = 0; i < l_imageCount; i++)
	{
		VKRenderingBackendComponent::get().m_swapChainImages.emplace_back();
	}

	// get real VkImages
	if (vkGetSwapchainImagesKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, &l_imageCount, VKRenderingBackendComponent::get().m_swapChainImages.data()) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to acquire swap chain images!");
		return false;
	}

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Swap chain images has been acquired.");

	return true;
}

bool VKRenderingBackendNS::generateGPUBuffers()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	generateUBO(VKRenderingBackendComponent::get().m_cameraUBO, sizeof(CameraGPUData), VKRenderingBackendComponent::get().m_cameraUBOMemory);
	generateUBO(VKRenderingBackendComponent::get().m_meshUBO, sizeof(MeshGPUData) * l_renderingCapability.maxMeshes, VKRenderingBackendComponent::get().m_meshUBOMemory);
	generateUBO(VKRenderingBackendComponent::get().m_materialUBO, sizeof(MaterialGPUData) * l_renderingCapability.maxMaterials, VKRenderingBackendComponent::get().m_materialUBOMemory);
	generateUBO(VKRenderingBackendComponent::get().m_sunUBO, sizeof(SunGPUData), VKRenderingBackendComponent::get().m_sunUBOMemory);
	generateUBO(VKRenderingBackendComponent::get().m_pointLightUBO, sizeof(PointLightGPUData) * l_renderingCapability.maxPointLights, VKRenderingBackendComponent::get().m_pointLightUBOMemory);
	generateUBO(VKRenderingBackendComponent::get().m_sphereLightUBO, sizeof(SphereLightGPUData) * l_renderingCapability.maxSphereLights, VKRenderingBackendComponent::get().m_sphereLightUBOMemory);

	return true;
}

bool VKRenderingBackendNS::setup()
{
	m_EntityID = InnoMath::createEntityID();

	initializeComponentPool();

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	// general render pass desc
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTNumber = 1;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.width = l_screenResolution.x;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.height = l_screenResolution.y;
	VKRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	bool result = true;
	result = result && createVkInstance();
	result = result && createDebugCallback();

	m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend setup finished.");
	return result;
}

bool VKRenderingBackendNS::initialize()
{
	if (VKRenderingBackendNS::m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

		m_MeshDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(VKMeshDataComponent), l_renderingCapability.maxMeshes);
		m_MaterialDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(VKMaterialDataComponent), l_renderingCapability.maxMaterials);
		m_TextureDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(VKTextureDataComponent), l_renderingCapability.maxTextures);

		bool l_result = true;

		l_result &= createPysicalDevice();
		l_result &= createLogicalDevice();

		l_result &= createTextureSamplers();
		l_result &= createMaterialDescriptorPool();
		l_result &= createCommandPool();

		loadDefaultAssets();

		generateGPUBuffers();

		VKOpaquePass::initialize();
		VKLightPass::initialize();

		l_result &= createSwapChain();

		VKFinalBlendPass::initialize();

		VKRenderingBackendNS::m_ObjectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend has been initialized.");
		return l_result;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Object is not created!");
		return false;
	}
}

void VKRenderingBackendNS::loadDefaultAssets()
{
	auto l_basicNormalTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTexture = reinterpret_cast<VKTextureDataComponent*>(l_basicNormalTexture);
	m_basicAlbedoTexture = reinterpret_cast<VKTextureDataComponent*>(l_basicAlbedoTexture);
	m_basicMetallicTexture = reinterpret_cast<VKTextureDataComponent*>(l_basicMetallicTexture);
	m_basicRoughnessTexture = reinterpret_cast<VKTextureDataComponent*>(l_basicRoughnessTexture);
	m_basicAOTexture = reinterpret_cast<VKTextureDataComponent*>(l_basicAOTexture);

	m_iconTemplate_OBJ = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMesh = addVKMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMesh);
	m_unitLineMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMesh->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMesh->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMesh);

	m_unitQuadMesh = addVKMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMesh);
	// adjust texture coordinate
	for (auto& i : m_unitQuadMesh->m_vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}
	m_unitQuadMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMesh->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMesh->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMesh);

	m_unitCubeMesh = addVKMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMesh);
	m_unitCubeMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMesh->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMesh);

	m_unitSphereMesh = addVKMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMesh);
	m_unitSphereMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMesh->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMesh);

	m_terrainMesh = addVKMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMesh);
	m_terrainMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMesh->m_ObjectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMesh);

	m_basicMaterial = addVKMaterialDataComponent();
	m_basicMaterial->m_normalTexture = m_basicNormalTexture;
	m_basicMaterial->m_albedoTexture = m_basicAlbedoTexture;
	m_basicMaterial->m_metallicTexture = m_basicMetallicTexture;
	m_basicMaterial->m_roughnessTexture = m_basicRoughnessTexture;
	m_basicMaterial->m_aoTexture = m_basicAOTexture;

	initializeVKMeshDataComponent(m_unitLineMesh);
	initializeVKMeshDataComponent(m_unitQuadMesh);
	initializeVKMeshDataComponent(m_unitCubeMesh);
	initializeVKMeshDataComponent(m_unitSphereMesh);
	initializeVKMeshDataComponent(m_terrainMesh);

	initializeVKTextureDataComponent(m_basicNormalTexture);
	initializeVKTextureDataComponent(m_basicAlbedoTexture);
	initializeVKTextureDataComponent(m_basicMetallicTexture);
	initializeVKTextureDataComponent(m_basicRoughnessTexture);
	initializeVKTextureDataComponent(m_basicAOTexture);

	initializeVKTextureDataComponent(m_iconTemplate_OBJ);
	initializeVKTextureDataComponent(m_iconTemplate_PNG);
	initializeVKTextureDataComponent(m_iconTemplate_SHADER);
	initializeVKTextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeVKTextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeVKTextureDataComponent(m_iconTemplate_PointLight);
	initializeVKTextureDataComponent(m_iconTemplate_SphereLight);

	initializeVKMaterialDataComponent(m_basicMaterial);
}

bool VKRenderingBackendNS::update()
{
	while (VKRenderingBackendNS::m_uninitializedMeshes.size() > 0)
	{
		VKMeshDataComponent* l_MDC;
		VKRenderingBackendNS::m_uninitializedMeshes.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeVKMeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: can't initialize VKMeshDataComponent for " + std::string(l_MDC->m_ParentEntity->m_EntityName.c_str()) + "!");
			}
		}
	}
	while (VKRenderingBackendNS::m_uninitializedMaterials.size() > 0)
	{
		VKMaterialDataComponent* l_MDC;
		VKRenderingBackendNS::m_uninitializedMaterials.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeVKMaterialDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: can't initialize VKTextureDataComponent for " + std::string(l_MDC->m_ParentEntity->m_EntityName.c_str()) + "!");
			}
		}
	}

	updateUBO(VKRenderingBackendComponent::get().m_cameraUBOMemory, g_pModuleManager->getRenderingFrontend()->getCameraGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_sunUBOMemory, g_pModuleManager->getRenderingFrontend()->getSunGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_pointLightUBOMemory, g_pModuleManager->getRenderingFrontend()->getPointLightGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_sphereLightUBOMemory, g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_meshUBOMemory, g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_materialUBOMemory, g_pModuleManager->getRenderingFrontend()->getOpaquePassMaterialGPUData());

	VKOpaquePass::update();
	VKLightPass::update();
	VKFinalBlendPass::update();

	return true;
}

bool VKRenderingBackendNS::render()
{
	VKOpaquePass::render();
	VKLightPass::render();
	VKFinalBlendPass::render();

	return true;
}

bool VKRenderingBackendNS::terminate()
{
	vkDeviceWaitIdle(VKRenderingBackendComponent::get().m_device);

	VKFinalBlendPass::terminate();
	VKLightPass::terminate();
	VKOpaquePass::terminate();

	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_cameraUBO, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_cameraUBOMemory, nullptr);
	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_meshUBO, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_meshUBOMemory, nullptr);

	vkDestroySampler(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_deferredRTSampler, nullptr);

	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_indexBufferMemory, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_vertexBufferMemory, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_textureImageMemory, nullptr);

	vkDestroySwapchainKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, nullptr);

	vkDestroyCommandPool(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_commandPool, nullptr);

	vkDestroyDevice(VKRenderingBackendComponent::get().m_device, nullptr);

	if (VKRenderingBackendComponent::get().m_enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(VKRenderingBackendComponent::get().m_instance, VKRenderingBackendComponent::get().m_messengerCallback, nullptr);
	}

	vkDestroySurfaceKHR(VKRenderingBackendComponent::get().m_instance, VKRenderingBackendComponent::get().m_windowSurface, nullptr);

	vkDestroyInstance(VKRenderingBackendComponent::get().m_instance, nullptr);

	VKRenderingBackendNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend has been terminated.");

	return true;
}

VKMeshDataComponent* VKRenderingBackendNS::addVKMeshDataComponent()
{
	static std::atomic<uint32_t> meshCount = 0;
	meshCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(VKMeshDataComponent));
	auto l_Mesh = new(l_rawPtr)VKMeshDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Mesh_" + std::to_string(meshCount) + "/").c_str());
	l_Mesh->m_ParentEntity = l_parentEntity;
	return l_Mesh;
}

VKMaterialDataComponent* VKRenderingBackendNS::addVKMaterialDataComponent()
{
	static std::atomic<uint32_t> materialCount = 0;
	materialCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(VKMaterialDataComponent));
	auto l_Material = new(l_rawPtr)VKMaterialDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Material_" + std::to_string(materialCount) + "/").c_str());
	l_Material->m_ParentEntity = l_parentEntity;
	return l_Material;
}

VKTextureDataComponent* VKRenderingBackendNS::addVKTextureDataComponent()
{
	static std::atomic<uint32_t> textureCount = 0;
	textureCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(VKTextureDataComponent));
	auto l_Texture = new(l_rawPtr)VKTextureDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Texture_" + std::to_string(textureCount) + "/").c_str());
	l_Texture->m_ParentEntity = l_parentEntity;
	return l_Texture;
}

VKMeshDataComponent* VKRenderingBackendNS::getVKMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return VKRenderingBackendNS::m_unitLineMesh; break;
	case MeshShapeType::QUAD:
		return VKRenderingBackendNS::m_unitQuadMesh; break;
	case MeshShapeType::CUBE:
		return VKRenderingBackendNS::m_unitCubeMesh; break;
	case MeshShapeType::SPHERE:
		return VKRenderingBackendNS::m_unitSphereMesh; break;
	case MeshShapeType::TERRAIN:
		return VKRenderingBackendNS::m_terrainMesh; break;
	case MeshShapeType::CUSTOM:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackend: wrong MeshShapeType passed to VKRenderingBackend::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

VKTextureDataComponent * VKRenderingBackendNS::getVKTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return VKRenderingBackendNS::m_basicNormalTexture; break;
	case TextureUsageType::ALBEDO:
		return VKRenderingBackendNS::m_basicAlbedoTexture; break;
	case TextureUsageType::METALLIC:
		return VKRenderingBackendNS::m_basicMetallicTexture; break;
	case TextureUsageType::ROUGHNESS:
		return VKRenderingBackendNS::m_basicRoughnessTexture; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return VKRenderingBackendNS::m_basicAOTexture; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

VKTextureDataComponent * VKRenderingBackendNS::getVKTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return VKRenderingBackendNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return VKRenderingBackendNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return VKRenderingBackendNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return VKRenderingBackendNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

VKTextureDataComponent * VKRenderingBackendNS::getVKTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return VKRenderingBackendNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return VKRenderingBackendNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return VKRenderingBackendNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

VKMaterialDataComponent * VKRenderingBackendNS::getDefaultMaterialDataComponent()
{
	return VKRenderingBackendNS::m_basicMaterial;
}

bool VKRenderingBackendNS::resize()
{
	return true;
}

bool VKRenderingBackend::setup()
{
	return VKRenderingBackendNS::setup();
}

bool VKRenderingBackend::initialize()
{
	return VKRenderingBackendNS::initialize();
}

bool VKRenderingBackend::update()
{
	return VKRenderingBackendNS::update();
}

bool VKRenderingBackend::render()
{
	return VKRenderingBackendNS::render();
}

bool VKRenderingBackend::present()
{
	return true;
}

bool VKRenderingBackend::terminate()
{
	return VKRenderingBackendNS::terminate();
}

ObjectStatus VKRenderingBackend::getStatus()
{
	return VKRenderingBackendNS::m_ObjectStatus;
}

MeshDataComponent * VKRenderingBackend::addMeshDataComponent()
{
	return VKRenderingBackendNS::addVKMeshDataComponent();
}

MaterialDataComponent * VKRenderingBackend::addMaterialDataComponent()
{
	return VKRenderingBackendNS::addVKMaterialDataComponent();
}

TextureDataComponent * VKRenderingBackend::addTextureDataComponent()
{
	return VKRenderingBackendNS::addVKTextureDataComponent();
}

MeshDataComponent * VKRenderingBackend::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return VKRenderingBackendNS::getVKMeshDataComponent(MeshShapeType);
}

TextureDataComponent * VKRenderingBackend::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return VKRenderingBackendNS::getVKTextureDataComponent(TextureUsageType);
}

TextureDataComponent * VKRenderingBackend::getTextureDataComponent(FileExplorerIconType iconType)
{
	return VKRenderingBackendNS::getVKTextureDataComponent(iconType);
}

TextureDataComponent * VKRenderingBackend::getTextureDataComponent(WorldEditorIconType iconType)
{
	return VKRenderingBackendNS::getVKTextureDataComponent(iconType);
}

void VKRenderingBackend::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	VKRenderingBackendNS::m_uninitializedMeshes.push(reinterpret_cast<VKMeshDataComponent*>(rhs));
}

void VKRenderingBackend::registerUninitializedMaterialDataComponent(MaterialDataComponent * rhs)
{
	VKRenderingBackendNS::m_uninitializedMaterials.push(reinterpret_cast<VKMaterialDataComponent*>(rhs));
}

bool VKRenderingBackend::resize()
{
	return VKRenderingBackendNS::resize();
}

bool VKRenderingBackend::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		break;
	case RenderPassType::Opaque:
		break;
	case RenderPassType::Light:
		break;
	case RenderPassType::Transparent:
		break;
	case RenderPassType::Terrain:
		break;
	case RenderPassType::PostProcessing:
		break;
	default: break;
	}

	return true;
}

bool VKRenderingBackend::bakeGI()
{
	return true;
}