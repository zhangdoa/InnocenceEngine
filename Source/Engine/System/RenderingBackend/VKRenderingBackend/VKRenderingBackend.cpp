#include "VKRenderingBackend.h"

#include "VKRenderingBackendUtilities.h"
#include "../../../Component/VKRenderingBackendComponent.h"

#include "VKOpaquePass.h"
#include "VKLightPass.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKRenderingBackendNS
{
	EntityID m_entityID;

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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Validation Layer: " + std::string(pCallbackData->pMessage));
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
	bool createSwapChainCommandBuffers();
	bool createSyncPrimitives();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<VKMeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<VKTextureDataComponent*> m_uninitializedTDC;

	VKTextureDataComponent* m_iconTemplate_OBJ;
	VKTextureDataComponent* m_iconTemplate_PNG;
	VKTextureDataComponent* m_iconTemplate_SHADER;
	VKTextureDataComponent* m_iconTemplate_UNKNOWN;

	VKTextureDataComponent* m_iconTemplate_DirectionalLight;
	VKTextureDataComponent* m_iconTemplate_PointLight;
	VKTextureDataComponent* m_iconTemplate_SphereLight;

	VKMeshDataComponent* m_unitLineMDC;
	VKMeshDataComponent* m_unitQuadMDC;
	VKMeshDataComponent* m_unitCubeMDC;
	VKMeshDataComponent* m_unitSphereMDC;
	VKMeshDataComponent* m_terrainMDC;

	VKTextureDataComponent* m_basicNormalTDC;
	VKTextureDataComponent* m_basicAlbedoTDC;
	VKTextureDataComponent* m_basicMetallicTDC;
	VKTextureDataComponent* m_basicRoughnessTDC;
	VKTextureDataComponent* m_basicAOTDC;

	std::vector<VkImage> m_swapChainImages;
}

bool VKRenderingBackendNS::createVkInstance()
{
	// check support for validation layer
	if (VKRenderingBackendComponent::get().m_enableValidationLayers && !checkValidationLayerSupport()) {
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo l_appInfo = {};
	l_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_appInfo.pApplicationName = g_pCoreSystem->getGameSystem()->getGameName().c_str();
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkInstance!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkInstance has been created.");
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
			m_objectStatus = ObjectStatus::Suspended;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create DebugUtilsMessenger!");
			return false;
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: Validation Layer has been created.");
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to find GPUs with Vulkan support!");
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to find a suitable GPU!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkPhysicalDevice has been created.");
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDevice!");
		return false;
	}

	vkGetDeviceQueue(VKRenderingBackendComponent::get().m_device, l_indices.m_graphicsFamily.value(), 0, &VKRenderingBackendComponent::get().m_graphicsQueue);
	vkGetDeviceQueue(VKRenderingBackendComponent::get().m_device, l_indices.m_presentFamily.value(), 0, &VKRenderingBackendComponent::get().m_presentQueue);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDevice has been created.");
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkSampler for deferred pass render target sampling!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKRenderingBackendNS::createMaterialDescriptorPool()
{
	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = 5;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = { l_descriptorPoolSize };

	auto l_renderingCapability = g_pCoreSystem->getRenderingFrontend()->getRenderingCapability();

	if (!createDescriptorPool(l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, VKRenderingBackendComponent::get().m_materialDescriptorPool))
	{
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkDescriptorPool for material!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkDescriptorPool for material has been created.");
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create CommandPool!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: CommandPool has been created.");
	return true;
}

bool VKRenderingBackendNS::createSwapChain()
{
	// choose device supported formats, modes and maximum back buffers
	SwapChainSupportDetails l_swapChainSupport = querySwapChainSupport(VKRenderingBackendComponent::get().m_physicalDevice);

	VkSurfaceFormatKHR l_surfaceFormat = chooseSwapSurfaceFormat(l_swapChainSupport.m_formats);
	VkPresentModeKHR l_presentMode = chooseSwapPresentMode(l_swapChainSupport.m_presentModes);
	VkExtent2D l_extent = chooseSwapExtent(l_swapChainSupport.m_capabilities);

	uint32_t l_imageCount = l_swapChainSupport.m_capabilities.minImageCount + 1;
	if (l_swapChainSupport.m_capabilities.maxImageCount > 0 && l_imageCount > l_swapChainSupport.m_capabilities.maxImageCount)
	{
		l_imageCount = l_swapChainSupport.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	l_createInfo.surface = VKRenderingBackendComponent::get().m_windowSurface;
	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = l_surfaceFormat.format;
	l_createInfo.imageColorSpace = l_surfaceFormat.colorSpace;
	l_createInfo.imageExtent = l_extent;
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
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create VkSwapChainKHR!");
		return false;
	}

	// get swap chain VkImages
	// get count
	vkGetSwapchainImagesKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, &l_imageCount, nullptr);

	m_swapChainImages.reserve(l_imageCount);
	for (size_t i = 0; i < l_imageCount; i++)
	{
		m_swapChainImages.emplace_back();
	}
	// get real VkImages
	vkGetSwapchainImagesKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, &l_imageCount, m_swapChainImages.data());

	// add shader component
	auto l_VKSPC = addVKShaderProgramComponent(m_entityID);

	ShaderFilePaths l_shaderFilePaths = {};
	l_shaderFilePaths.m_VSPath = "VK//finalBlendPass.vert.spv/";
	l_shaderFilePaths.m_FSPath = "VK//finalBlendPass.frag.spv/";

	initializeVKShaderProgramComponent(l_VKSPC, l_shaderFilePaths);

	// add render pass component
	auto l_VKRPC = addVKRenderPassComponent();

	l_VKRPC->m_renderPassDesc = VKRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_VKRPC->m_renderPassDesc.RTNumber = l_imageCount;
	l_VKRPC->m_renderPassDesc.useMultipleFramebuffers = (l_imageCount > 1);

	VkTextureDataDesc l_VkTextureDataDesc;
	l_VkTextureDataDesc.imageType = VK_IMAGE_TYPE_2D;
	l_VkTextureDataDesc.samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	l_VkTextureDataDesc.magFilterParam = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	l_VkTextureDataDesc.minFilterParam = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	l_VkTextureDataDesc.format = l_surfaceFormat.format;
	l_VkTextureDataDesc.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

	// initialize manually
	bool l_result = true;

	l_result &= reserveRenderTargets(l_VKRPC);

	// use device created swap chain VkImages
	for (size_t i = 0; i < l_imageCount; i++)
	{
		l_VKRPC->m_VKTDCs[i]->m_VkTextureDataDesc = l_VkTextureDataDesc;
		l_VKRPC->m_VKTDCs[i]->m_image = m_swapChainImages[i];
		createImageView(l_VKRPC->m_VKTDCs[i]);
	}

	// create descriptor pool
	VkDescriptorPoolSize l_RTSamplerDescriptorPoolSize = {};
	l_RTSamplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
	l_RTSamplerDescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_RTTextureDescriptorPoolSize = {};
	l_RTTextureDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_RTTextureDescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize l_descriptorPoolSize[] = { l_RTSamplerDescriptorPoolSize , l_RTTextureDescriptorPoolSize };

	l_result &= createDescriptorPool(l_descriptorPoolSize, 2, 1, l_VKRPC->m_descriptorPool);

	// sub-pass
	VkAttachmentReference l_attachmentRef = {};
	l_attachmentRef.attachment = 0;
	l_attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	l_VKRPC->colorAttachmentRefs.emplace_back(l_attachmentRef);

	// render pass
	VkAttachmentDescription attachmentDesc = {};
	attachmentDesc.format = l_surfaceFormat.format;
	attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	l_VKRPC->attachmentDescs.emplace_back(attachmentDesc);

	l_VKRPC->renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	l_VKRPC->renderPassCInfo.subpassCount = 1;

	// set descriptor set layout binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = &VKRenderingBackendComponent::get().m_deferredRTSampler;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	l_VKRPC->descriptorSetLayoutBindings.emplace_back(samplerLayoutBinding);

	VkDescriptorSetLayoutBinding textureLayoutBinding = {};
	textureLayoutBinding.binding = 1;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	textureLayoutBinding.pImmutableSamplers = nullptr;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	l_VKRPC->descriptorSetLayoutBindings.emplace_back(textureLayoutBinding);

	// set descriptor image info
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageInfo.imageView = VKLightPass::getVKRPC()->m_VKTDCs[0]->m_imageView;

	VkWriteDescriptorSet basePassRTWriteDescriptorSet = {};
	basePassRTWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	basePassRTWriteDescriptorSet.dstBinding = 1;
	basePassRTWriteDescriptorSet.dstArrayElement = 0;
	basePassRTWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	basePassRTWriteDescriptorSet.descriptorCount = 1;
	basePassRTWriteDescriptorSet.pImageInfo = &imageInfo;
	l_VKRPC->writeDescriptorSets.emplace_back(basePassRTWriteDescriptorSet);

	// set pipeline fix stages info
	l_VKRPC->inputAssemblyStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	l_VKRPC->inputAssemblyStateCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	l_VKRPC->inputAssemblyStateCInfo.primitiveRestartEnable = VK_FALSE;

	l_VKRPC->viewport.x = 0.0f;
	l_VKRPC->viewport.y = 0.0f;
	l_VKRPC->viewport.width = (float)l_extent.width;
	l_VKRPC->viewport.height = (float)l_extent.height;
	l_VKRPC->viewport.minDepth = 0.0f;
	l_VKRPC->viewport.maxDepth = 1.0f;

	l_VKRPC->scissor.offset = { 0, 0 };
	l_VKRPC->scissor.extent = l_extent;

	l_VKRPC->viewportStateCInfo.viewportCount = 1;
	l_VKRPC->viewportStateCInfo.scissorCount = 1;

	l_VKRPC->rasterizationStateCInfo.depthClampEnable = VK_FALSE;
	l_VKRPC->rasterizationStateCInfo.rasterizerDiscardEnable = VK_FALSE;
	l_VKRPC->rasterizationStateCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	l_VKRPC->rasterizationStateCInfo.lineWidth = 1.0f;
	l_VKRPC->rasterizationStateCInfo.cullMode = VK_CULL_MODE_NONE;
	l_VKRPC->rasterizationStateCInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	l_VKRPC->rasterizationStateCInfo.depthBiasEnable = VK_FALSE;

	l_VKRPC->multisampleStateCInfo.sampleShadingEnable = VK_FALSE;
	l_VKRPC->multisampleStateCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState l_colorBlendAttachmentState = {};
	l_colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	l_colorBlendAttachmentState.blendEnable = VK_FALSE;
	l_VKRPC->colorBlendAttachmentStates.emplace_back(l_colorBlendAttachmentState);

	l_VKRPC->colorBlendStateCInfo.logicOpEnable = VK_FALSE;
	l_VKRPC->colorBlendStateCInfo.logicOp = VK_LOGIC_OP_COPY;
	l_VKRPC->colorBlendStateCInfo.blendConstants[0] = 0.0f;
	l_VKRPC->colorBlendStateCInfo.blendConstants[1] = 0.0f;
	l_VKRPC->colorBlendStateCInfo.blendConstants[2] = 0.0f;
	l_VKRPC->colorBlendStateCInfo.blendConstants[3] = 0.0f;

	l_result &= createRenderPass(l_VKRPC);

	if (l_VKRPC->m_renderPassDesc.useMultipleFramebuffers)
	{
		l_result &= createMultipleFramebuffers(l_VKRPC);
	}
	else
	{
		l_result &= createSingleFramebuffer(l_VKRPC);
	}

	l_result &= createDescriptorSetLayout(l_VKRPC);

	l_result &= createPipelineLayout(l_VKRPC);

	l_result &= createGraphicsPipelines(l_VKRPC, l_VKSPC);

	l_result &= createDescriptorSets(l_VKRPC->m_descriptorPool, l_VKRPC->descriptorSetLayout, l_VKRPC->descriptorSet, 1);

	l_VKRPC->writeDescriptorSets[0].dstSet = l_VKRPC->descriptorSet;

	l_result &= updateDescriptorSet(l_VKRPC);

	VKRenderingBackendComponent::get().m_swapChainVKSPC = l_VKSPC;

	VKRenderingBackendComponent::get().m_swapChainVKRPC = l_VKRPC;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend: VkSwapChainKHR has been created.");
	return true;
}

bool VKRenderingBackendNS::createSwapChainCommandBuffers()
{
	auto l_result = createCommandBuffers(VKRenderingBackendComponent::get().m_swapChainVKRPC);

	for (size_t i = 0; i < VKRenderingBackendComponent::get().m_swapChainVKRPC->m_commandBuffers.size(); i++)
	{
		recordCommand(VKRenderingBackendComponent::get().m_swapChainVKRPC, (unsigned int)i, [&]() {
			vkCmdBindDescriptorSets(VKRenderingBackendComponent::get().m_swapChainVKRPC->m_commandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				VKRenderingBackendComponent::get().m_swapChainVKRPC->m_pipelineLayout,
				0,
				1,
				&VKRenderingBackendComponent::get().m_swapChainVKRPC->descriptorSet, 0, nullptr);
			auto l_MDC = getVKMeshDataComponent(MeshShapeType::QUAD);
			recordDrawCall(VKRenderingBackendComponent::get().m_swapChainVKRPC, (unsigned int)i, l_MDC);
		});
	}

	return l_result;
}

bool VKRenderingBackendNS::createSyncPrimitives()
{
	VKRenderingBackendComponent::get().m_swapChainVKRPC->m_maxFramesInFlight = 2;
	VKRenderingBackendComponent::get().m_imageAvailableSemaphores.resize(VKRenderingBackendComponent::get().m_swapChainVKRPC->m_maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < VKRenderingBackendComponent::get().m_swapChainVKRPC->m_maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(
			VKRenderingBackendComponent::get().m_device,
			&semaphoreInfo,
			nullptr,
			&VKRenderingBackendComponent::get().m_imageAvailableSemaphores[i])
			!= VK_SUCCESS)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Failed to create swap chain image available semaphores!");
			return false;
		}
	}

	auto l_result = createSyncPrimitives(VKRenderingBackendComponent::get().m_swapChainVKRPC);

	return l_result;
}

bool VKRenderingBackendNS::generateGPUBuffers()
{
	auto l_renderingCapability = g_pCoreSystem->getRenderingFrontend()->getRenderingCapability();

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
	m_entityID = InnoMath::createEntityID();

	initializeComponentPool();

	auto l_screenResolution = g_pCoreSystem->getRenderingFrontend()->getScreenResolution();

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

	m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend setup finished.");
	return result;
}

bool VKRenderingBackendNS::initialize()
{
	if (VKRenderingBackendNS::m_objectStatus == ObjectStatus::Created)
	{
		auto l_renderingCapability = g_pCoreSystem->getRenderingFrontend()->getRenderingCapability();

		m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKMeshDataComponent), l_renderingCapability.maxMeshes);
		m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKMaterialDataComponent), l_renderingCapability.maxMaterials);
		m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VKTextureDataComponent), l_renderingCapability.maxTextures);

		bool l_result = true;

		l_result = l_result && createPysicalDevice();
		l_result = l_result && createLogicalDevice();

		l_result = l_result && createTextureSamplers();
		l_result = l_result && createMaterialDescriptorPool();
		l_result = l_result && createCommandPool();

		loadDefaultAssets();

		generateGPUBuffers();

		VKOpaquePass::initialize();
		VKLightPass::initialize();

		l_result = l_result && createSwapChain();
		l_result = l_result && createSwapChainCommandBuffers();
		l_result = l_result && createSyncPrimitives();

		VKRenderingBackendNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend has been initialized.");
		return l_result;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: Object is not created!");
		return false;
	}
}

void VKRenderingBackendNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pCoreSystem->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTDC = reinterpret_cast<VKTextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTDC = reinterpret_cast<VKTextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTDC = reinterpret_cast<VKTextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTDC = reinterpret_cast<VKTextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTDC = reinterpret_cast<VKTextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<VKTextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addVKMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addVKMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	// adjust texture coordinate
	for (auto& i : m_unitQuadMDC->m_vertices)
	{
		i.m_texCoord.y = 1.0f - i.m_texCoord.y;
	}
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addVKMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addVKMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addVKMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	initializeVKMeshDataComponent(m_unitLineMDC);
	initializeVKMeshDataComponent(m_unitQuadMDC);
	initializeVKMeshDataComponent(m_unitCubeMDC);
	initializeVKMeshDataComponent(m_unitSphereMDC);
	initializeVKMeshDataComponent(m_terrainMDC);

	initializeVKTextureDataComponent(m_basicNormalTDC);
	initializeVKTextureDataComponent(m_basicAlbedoTDC);
	initializeVKTextureDataComponent(m_basicMetallicTDC);
	initializeVKTextureDataComponent(m_basicRoughnessTDC);
	initializeVKTextureDataComponent(m_basicAOTDC);

	initializeVKTextureDataComponent(m_iconTemplate_OBJ);
	initializeVKTextureDataComponent(m_iconTemplate_PNG);
	initializeVKTextureDataComponent(m_iconTemplate_SHADER);
	initializeVKTextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeVKTextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeVKTextureDataComponent(m_iconTemplate_PointLight);
	initializeVKTextureDataComponent(m_iconTemplate_SphereLight);
}

bool VKRenderingBackendNS::update()
{
	if (VKRenderingBackendNS::m_uninitializedMDC.size() > 0)
	{
		VKMeshDataComponent* l_MDC;
		VKRenderingBackendNS::m_uninitializedMDC.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeVKMeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: can't create VKMeshDataComponent for " + std::string(l_MDC->m_parentEntity->m_entityName.c_str()) + "!");
			}
		}
	}
	if (VKRenderingBackendNS::m_uninitializedTDC.size() > 0)
	{
		VKTextureDataComponent* l_TDC;
		VKRenderingBackendNS::m_uninitializedTDC.tryPop(l_TDC);

		if (l_TDC)
		{
			auto l_result = initializeVKTextureDataComponent(l_TDC);
			if (!l_result)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingBackend: can't create VKTextureDataComponent for " + std::string(l_TDC->m_parentEntity->m_entityName.c_str()) + "!");
			}
		}
	}

	updateUBO(VKRenderingBackendComponent::get().m_cameraUBOMemory, g_pCoreSystem->getRenderingFrontend()->getCameraGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_sunUBOMemory, g_pCoreSystem->getRenderingFrontend()->getSunGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_pointLightUBOMemory, g_pCoreSystem->getRenderingFrontend()->getPointLightGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_sphereLightUBOMemory, g_pCoreSystem->getRenderingFrontend()->getSphereLightGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_meshUBOMemory, g_pCoreSystem->getRenderingFrontend()->getOpaquePassMeshGPUData());
	updateUBO(VKRenderingBackendComponent::get().m_materialUBOMemory, g_pCoreSystem->getRenderingFrontend()->getOpaquePassMaterialGPUData());

	VKOpaquePass::update();
	VKLightPass::update();

	return true;
}

bool VKRenderingBackendNS::render()
{
	VKOpaquePass::render();
	VKLightPass::render();

	waitForFence(VKRenderingBackendComponent::get().m_swapChainVKRPC);

	// acquire an image from swap chain
	thread_local uint32_t imageIndex;
	vkAcquireNextImageKHR(
		VKRenderingBackendComponent::get().m_device,
		VKRenderingBackendComponent::get().m_swapChain,
		std::numeric_limits<uint64_t>::max(),
		VKRenderingBackendComponent::get().m_imageAvailableSemaphores[VKRenderingBackendComponent::get().m_swapChainVKRPC->m_currentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	// set swap chain image available wait semaphore
	VkSemaphore l_availableSemaphores[] = {
		VKLightPass::getVKRPC()->m_renderFinishedSemaphores[0],
		VKRenderingBackendComponent::get().m_imageAvailableSemaphores[VKRenderingBackendComponent::get().m_swapChainVKRPC->m_currentFrame]
	};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VKRenderingBackendComponent::get().m_swapChainVKRPC->submitInfo.waitSemaphoreCount = 2;
	VKRenderingBackendComponent::get().m_swapChainVKRPC->submitInfo.pWaitSemaphores = l_availableSemaphores;
	VKRenderingBackendComponent::get().m_swapChainVKRPC->submitInfo.pWaitDstStageMask = waitStages;

	submitCommand(VKRenderingBackendComponent::get().m_swapChainVKRPC, imageIndex);

	// present the swap chain image to the front screen
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// wait semaphore
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &VKRenderingBackendComponent::get().m_swapChainVKRPC->m_renderFinishedSemaphores[VKRenderingBackendComponent::get().m_swapChainVKRPC->m_currentFrame];

	// swap chain
	VkSwapchainKHR swapChains[] = { VKRenderingBackendComponent::get().m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(VKRenderingBackendComponent::get().m_presentQueue, &presentInfo);

	VKRenderingBackendComponent::get().m_swapChainVKRPC->m_currentFrame = (VKRenderingBackendComponent::get().m_swapChainVKRPC->m_currentFrame + 1) % VKRenderingBackendComponent::get().m_swapChainVKRPC->m_maxFramesInFlight;

	return true;
}

bool VKRenderingBackendNS::terminate()
{
	vkDeviceWaitIdle(VKRenderingBackendComponent::get().m_device);

	VKOpaquePass::terminate();

	destroyVKShaderProgramComponent(VKRenderingBackendComponent::get().m_swapChainVKSPC);

	for (size_t i = 0; i < VKRenderingBackendComponent::get().m_swapChainVKRPC->m_maxFramesInFlight; i++)
	{
		vkDestroySemaphore(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_inFlightFences[i], nullptr);
	}

	vkDestroyDescriptorPool(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_descriptorPool, nullptr);

	vkFreeCommandBuffers(VKRenderingBackendComponent::get().m_device,
		VKRenderingBackendComponent::get().m_commandPool,
		static_cast<uint32_t>(VKRenderingBackendComponent::get().m_swapChainVKRPC->m_commandBuffers.size()),
		VKRenderingBackendComponent::get().m_swapChainVKRPC->m_commandBuffers.data());

	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_cameraUBO, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_cameraUBOMemory, nullptr);
	vkDestroyBuffer(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_meshUBO, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_meshUBOMemory, nullptr);

	vkDestroySampler(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_deferredRTSampler, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_textureImageMemory, nullptr);

	destroyAllGraphicPrimitiveComponents();

	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_indexBufferMemory, nullptr);
	vkFreeMemory(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_vertexBufferMemory, nullptr);

	vkDestroyPipeline(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_pipeline, nullptr);
	vkDestroyPipelineLayout(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->descriptorSetLayout, nullptr);

	for (auto framebuffer : VKRenderingBackendComponent::get().m_swapChainVKRPC->m_framebuffers)
	{
		vkDestroyFramebuffer(VKRenderingBackendComponent::get().m_device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChainVKRPC->m_renderPass, nullptr);

	for (auto VKTDC : VKRenderingBackendComponent::get().m_swapChainVKRPC->m_VKTDCs)
	{
		vkDestroyImageView(VKRenderingBackendComponent::get().m_device, VKTDC->m_imageView, nullptr);
	}

	vkDestroySwapchainKHR(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_swapChain, nullptr);

	vkDestroyCommandPool(VKRenderingBackendComponent::get().m_device, VKRenderingBackendComponent::get().m_commandPool, nullptr);

	vkDestroyDevice(VKRenderingBackendComponent::get().m_device, nullptr);

	if (VKRenderingBackendComponent::get().m_enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(VKRenderingBackendComponent::get().m_instance, VKRenderingBackendComponent::get().m_messengerCallback, nullptr);
	}

	vkDestroySurfaceKHR(VKRenderingBackendComponent::get().m_instance, VKRenderingBackendComponent::get().m_windowSurface, nullptr);

	vkDestroyInstance(VKRenderingBackendComponent::get().m_instance, nullptr);

	VKRenderingBackendNS::m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingBackend has been terminated.");

	return true;
}

VKMeshDataComponent* VKRenderingBackendNS::addVKMeshDataComponent()
{
	static std::atomic<unsigned int> meshCount = 0;
	meshCount++;
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(VKMeshDataComponent));
	auto l_MDC = new(l_rawPtr)VKMeshDataComponent();
	auto l_parentEntity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(("Mesh_" + std::to_string(meshCount) + "/").c_str()), ObjectSource::Runtime, ObjectUsage::Engine);
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

VKMaterialDataComponent* VKRenderingBackendNS::addVKMaterialDataComponent()
{
	static std::atomic<unsigned int> materialCount = 0;
	materialCount++;
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(VKMaterialDataComponent));
	auto l_MDC = new(l_rawPtr)VKMaterialDataComponent();
	auto l_parentEntity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(("Material_" + std::to_string(materialCount) + "/").c_str()), ObjectSource::Runtime, ObjectUsage::Engine);
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

VKTextureDataComponent* VKRenderingBackendNS::addVKTextureDataComponent()
{
	static std::atomic<unsigned int> textureCount = 0;
	textureCount++;
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(VKTextureDataComponent));
	auto l_TDC = new(l_rawPtr)VKTextureDataComponent();
	auto l_parentEntity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(("Texture_" + std::to_string(textureCount) + "/").c_str()), ObjectSource::Runtime, ObjectUsage::Engine);
	l_TDC->m_parentEntity = l_parentEntity;
	return l_TDC;
}

VKMeshDataComponent* VKRenderingBackendNS::getVKMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return VKRenderingBackendNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return VKRenderingBackendNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return VKRenderingBackendNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return VKRenderingBackendNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return VKRenderingBackendNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackend: wrong MeshShapeType passed to VKRenderingBackend::getMeshDataComponent() !");
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
		return VKRenderingBackendNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return VKRenderingBackendNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return VKRenderingBackendNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return VKRenderingBackendNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return VKRenderingBackendNS::m_basicAOTDC; break;
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

bool VKRenderingBackend::terminate()
{
	return VKRenderingBackendNS::terminate();
}

ObjectStatus VKRenderingBackend::getStatus()
{
	return VKRenderingBackendNS::m_objectStatus;
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
	VKRenderingBackendNS::m_uninitializedMDC.push(reinterpret_cast<VKMeshDataComponent*>(rhs));
}

void VKRenderingBackend::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	VKRenderingBackendNS::m_uninitializedTDC.push(reinterpret_cast<VKTextureDataComponent*>(rhs));
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