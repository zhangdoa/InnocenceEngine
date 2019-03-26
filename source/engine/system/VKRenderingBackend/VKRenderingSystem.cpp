#include "VKRenderingSystem.h"

#include "VKRenderingSystemUtilities.h"
#include "../../component/VKRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

#define INSTANCE_FUNC_PTR(instance, entrypoint){											\
    fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(instance, "vk"#entrypoint); \
    if (fp##entrypoint == NULL) {															\
        std::cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

#define DEVICE_FUNC_PTR(dev, entrypoint){													\
    fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);		\
    if (fp##entrypoint == NULL) {															\
        std::cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	EntityID m_entityID;

	bool checkValidationLayerSupport()
	{
		uint32_t l_layerCount;
		vkEnumerateInstanceLayerProperties(&l_layerCount, nullptr);

		std::vector<VkLayerProperties> l_availableLayers(l_layerCount);
		vkEnumerateInstanceLayerProperties(&l_layerCount, l_availableLayers.data());

		for (const char* layerName : VKRenderingSystemComponent::get().m_validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : l_availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

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

	std::vector<const char*> getRequiredExtensions()
	{
#if defined INNO_PLATFORM_WIN
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#elif  defined INNO_PLATFORM_MAC
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_MVK_macos_surface" };
#elif  defined INNO_PLATFORM_LINUX
		std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_xcb_surface" };
#endif

		if (VKRenderingSystemComponent::get().m_enableValidationLayers)
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Validation Layer: " + std::string(pCallbackData->pMessage));
		return VK_FALSE;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(VKRenderingSystemComponent::get().m_deviceExtensions.begin(), VKRenderingSystemComponent::get().m_deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_graphicsFamily = i;
			}

			VkBool32 l_presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VKRenderingSystemComponent::get().m_windowSurface, &l_presentSupport);

			if (queueFamily.queueCount > 0 && l_presentSupport)
			{
				indices.m_presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
	{
		VkPresentModeKHR l_bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				l_bestMode = availablePresentMode;
			}
		}

		return l_bestMode;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();

			VkExtent2D l_actualExtent;
			l_actualExtent.width = l_screenResolution.x;
			l_actualExtent.height = l_screenResolution.y;

			l_actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, l_actualExtent.width));
			l_actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, l_actualExtent.height));

			return l_actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails l_details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VKRenderingSystemComponent::get().m_windowSurface, &l_details.m_capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKRenderingSystemComponent::get().m_windowSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			l_details.m_formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKRenderingSystemComponent::get().m_windowSurface, &formatCount, l_details.m_formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKRenderingSystemComponent::get().m_windowSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			l_details.m_presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKRenderingSystemComponent::get().m_windowSurface, &presentModeCount, l_details.m_presentModes.data());
		}

		return l_details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	bool setup(IRenderingFrontendSystem* renderingFrontend);

	bool createVkInstance();
	bool createDebugCallback();
	bool createWindowSurface();
	bool createPysicalDevice();
	bool createLogicalDevice();
	bool createSwapChain();
	bool createSwapChainImageViews();
	bool createSwapChainRenderPass();
	bool createSwapChainFramebuffers();
	bool createCommandPool();
	bool createCommandBuffers();
	bool createSyncPrimitives();

	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	IRenderingFrontendSystem* m_renderingFrontendSystem;

	VKShaderProgramComponent* m_swapChainVKSPC;
	VKRenderPassComponent* m_swapChainVKRPC;

	size_t m_maxFramesInFlight = 2;
	size_t m_currentFrame = 0;
}

bool VKRenderingSystemNS::createVkInstance()
{
	// check support for validation layer
	if (VKRenderingSystemComponent::get().m_enableValidationLayers && !checkValidationLayerSupport()) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: validation layers requested, but not available!");
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

	if (VKRenderingSystemComponent::get().m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(VKRenderingSystemComponent::get().m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = VKRenderingSystemComponent::get().m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	// create Vulkan instance
	if (vkCreateInstance(&l_createInfo, nullptr, &VKRenderingSystemComponent::get().m_instance) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create VkInstance!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: VkInstance has been created.");
	return true;
}

bool VKRenderingSystemNS::createDebugCallback()
{
	if (VKRenderingSystemComponent::get().m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		l_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		l_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		l_createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(VKRenderingSystemComponent::get().m_instance, &l_createInfo, nullptr, &VKRenderingSystemComponent::get().m_messengerCallback) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create DebugUtilsMessenger!");
			return false;
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: validation layer has been created.");
		return true;
	}
	else
	{
		return true;
	}
}

bool VKRenderingSystemNS::createWindowSurface()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: window surface has been created.");
	return true;
}

bool VKRenderingSystemNS::createPysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(VKRenderingSystemComponent::get().m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	std::vector<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(VKRenderingSystemComponent::get().m_instance, &l_deviceCount, l_devices.data());

	for (const auto& device : l_devices)
	{
		if (isDeviceSuitable(device))
		{
			VKRenderingSystemComponent::get().m_physicalDevice = device;
			break;
		}
	}

	if (VKRenderingSystemComponent::get().m_physicalDevice == VK_NULL_HANDLE)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to find a suitable GPU!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: physical device has been created.");
	return true;
}

bool VKRenderingSystemNS::createLogicalDevice()
{
	QueueFamilyIndices l_indices = findQueueFamilies(VKRenderingSystemComponent::get().m_physicalDevice);

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

	VkDeviceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	l_createInfo.queueCreateInfoCount = static_cast<uint32_t>(l_queueCreateInfos.size());
	l_createInfo.pQueueCreateInfos = l_queueCreateInfos.data();

	l_createInfo.pEnabledFeatures = &l_deviceFeatures;

	l_createInfo.enabledExtensionCount = static_cast<uint32_t>(VKRenderingSystemComponent::get().m_deviceExtensions.size());
	l_createInfo.ppEnabledExtensionNames = VKRenderingSystemComponent::get().m_deviceExtensions.data();

	if (VKRenderingSystemComponent::get().m_enableValidationLayers)
	{
		l_createInfo.enabledLayerCount = static_cast<uint32_t>(VKRenderingSystemComponent::get().m_validationLayers.size());
		l_createInfo.ppEnabledLayerNames = VKRenderingSystemComponent::get().m_validationLayers.data();
	}
	else
	{
		l_createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(VKRenderingSystemComponent::get().m_physicalDevice, &l_createInfo, nullptr, &VKRenderingSystemComponent::get().m_device) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create logical device!");
		return false;
	}

	vkGetDeviceQueue(VKRenderingSystemComponent::get().m_device, l_indices.m_graphicsFamily.value(), 0, &VKRenderingSystemComponent::get().m_graphicsQueue);
	vkGetDeviceQueue(VKRenderingSystemComponent::get().m_device, l_indices.m_presentFamily.value(), 0, &VKRenderingSystemComponent::get().m_presentQueue);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: logical device has been created.");
	return true;
}

bool VKRenderingSystemNS::createSwapChain()
{
	SwapChainSupportDetails l_swapChainSupport = querySwapChainSupport(VKRenderingSystemComponent::get().m_physicalDevice);

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
	l_createInfo.surface = VKRenderingSystemComponent::get().m_windowSurface;

	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = l_surfaceFormat.format;
	l_createInfo.imageColorSpace = l_surfaceFormat.colorSpace;
	l_createInfo.imageExtent = l_extent;
	l_createInfo.imageArrayLayers = 1;
	l_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices l_indices = findQueueFamilies(VKRenderingSystemComponent::get().m_physicalDevice);
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

	if (vkCreateSwapchainKHR(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &VKRenderingSystemComponent::get().m_swapChain) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create swap chain!");
		return false;
	}

	vkGetSwapchainImagesKHR(VKRenderingSystemComponent::get().m_device, VKRenderingSystemComponent::get().m_swapChain, &l_imageCount, nullptr);
	VKRenderingSystemComponent::get().m_swapChainImages.resize(l_imageCount);

	vkGetSwapchainImagesKHR(VKRenderingSystemComponent::get().m_device, VKRenderingSystemComponent::get().m_swapChain, &l_imageCount, VKRenderingSystemComponent::get().m_swapChainImages.data());

	VKRenderingSystemComponent::get().m_swapChainImageFormat = l_surfaceFormat.format;
	VKRenderingSystemComponent::get().m_swapChainExtent = l_extent;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain has been created.");
	return true;
}

bool VKRenderingSystemNS::createSwapChainImageViews()
{
	VKRenderingSystemComponent::get().m_swapChainImageViews.resize(VKRenderingSystemComponent::get().m_swapChainImages.size());

	for (size_t i = 0; i < VKRenderingSystemComponent::get().m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_createInfo.image = VKRenderingSystemComponent::get().m_swapChainImages[i];
		l_createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_createInfo.format = VKRenderingSystemComponent::get().m_swapChainImageFormat;
		l_createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_createInfo.subresourceRange.baseMipLevel = 0;
		l_createInfo.subresourceRange.levelCount = 1;
		l_createInfo.subresourceRange.baseArrayLayer = 0;
		l_createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &VKRenderingSystemComponent::get().m_swapChainImageViews[i]) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create swap chain image views!");
			return false;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain image views has been created.");
	return true;
}

bool VKRenderingSystemNS::createSwapChainRenderPass()
{
	auto l_VKSPC = addVKShaderProgramComponent(m_entityID);

	ShaderFilePaths l_shaderFilePaths;
	l_shaderFilePaths.m_VSPath = "..//res//shaders//VK//finalBlendPass.vert.spv";
	l_shaderFilePaths.m_FSPath = "..//res//shaders//VK//finalBlendPass.frag.spv";

	initializeVKShaderProgramComponent(l_VKSPC, l_shaderFilePaths);

	auto l_VKRPC = addVKRenderPassComponent(1, TextureDataDesc(), l_VKSPC);

	m_swapChainVKSPC = l_VKSPC;
	m_swapChainVKRPC = l_VKRPC;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain render pass has been created.");
	return true;
}

bool VKRenderingSystemNS::createSwapChainFramebuffers()
{
	VKRenderingSystemComponent::get().m_swapChainFramebuffers.resize(VKRenderingSystemComponent::get().m_swapChainImageViews.size());

	for (size_t i = 0; i < VKRenderingSystemComponent::get().m_swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			VKRenderingSystemComponent::get().m_swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_swapChainVKRPC->m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = VKRenderingSystemComponent::get().m_swapChainExtent.width;
		framebufferInfo.height = VKRenderingSystemComponent::get().m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VKRenderingSystemComponent::get().m_device, &framebufferInfo, nullptr, &VKRenderingSystemComponent::get().m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create swap chain framebuffer!");
			return false;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain framebuffer has been created.");
	return true;
}

bool VKRenderingSystemNS::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(VKRenderingSystemComponent::get().m_physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();

	if (vkCreateCommandPool(VKRenderingSystemComponent::get().m_device, &poolInfo, nullptr, &VKRenderingSystemComponent::get().m_commandPool) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create command pool!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: command pool has been created.");
	return true;
}

bool VKRenderingSystemNS::createCommandBuffers()
{
	VKRenderingSystemComponent::get().m_commandBuffers.resize(VKRenderingSystemComponent::get().m_swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = VKRenderingSystemComponent::get().m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)VKRenderingSystemComponent::get().m_commandBuffers.size();

	if (vkAllocateCommandBuffers(VKRenderingSystemComponent::get().m_device, &allocInfo, VKRenderingSystemComponent::get().m_commandBuffers.data()) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to allocate command buffers!");
		return false;
	}

	for (size_t i = 0; i < VKRenderingSystemComponent::get().m_commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(VKRenderingSystemComponent::get().m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to begin recording command buffer!");
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_swapChainVKRPC->m_renderPass;
		renderPassInfo.framebuffer = VKRenderingSystemComponent::get().m_swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = VKRenderingSystemComponent::get().m_swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(VKRenderingSystemComponent::get().m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(VKRenderingSystemComponent::get().m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_swapChainVKRPC->m_pipeline);

		vkCmdDraw(VKRenderingSystemComponent::get().m_commandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(VKRenderingSystemComponent::get().m_commandBuffers[i]);

		if (vkEndCommandBuffer(VKRenderingSystemComponent::get().m_commandBuffers[i]) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to record command buffer!");
			return false;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: command buffers has been created.");
	return true;
}

bool VKRenderingSystemNS::createSyncPrimitives()
{
	m_maxFramesInFlight = 2;
	VKRenderingSystemComponent::get().m_imageAvailableSemaphores.resize(m_maxFramesInFlight);
	VKRenderingSystemComponent::get().m_renderFinishedSemaphores.resize(m_maxFramesInFlight);
	VKRenderingSystemComponent::get().m_inFlightFences.resize(m_maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < m_maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(VKRenderingSystemComponent::get().m_device, &semaphoreInfo, nullptr, &VKRenderingSystemComponent::get().m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(VKRenderingSystemComponent::get().m_device, &semaphoreInfo, nullptr, &VKRenderingSystemComponent::get().m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(VKRenderingSystemComponent::get().m_device, &fenceInfo, nullptr, &VKRenderingSystemComponent::get().m_inFlightFences[i]) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create synchronization primitives for a frame!");
			return false;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: synchronization primitives has been created.");
	return true;
}

bool VKRenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_renderingFrontendSystem = renderingFrontend;

	m_entityID = InnoMath::createEntityID();

	initializeComponentPool();

	bool result = true;
	result = result && createVkInstance();
	result = result && createDebugCallback();

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem setup finished.");
	return result;
}
bool VKRenderingSystemNS::initialize()
{
	bool result = true;
	result = result && createPysicalDevice();
	result = result && createWindowSurface();
	result = result && createLogicalDevice();
	result = result && createSwapChain();
	result = result && createSwapChainImageViews();
	result = result && createSwapChainRenderPass();
	result = result && createSwapChainFramebuffers();
	result = result && createCommandPool();
	result = result && createCommandBuffers();
	result = result && createSyncPrimitives();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem has been initialized.");
	return result;
}

bool VKRenderingSystemNS::update()
{
	vkWaitForFences(VKRenderingSystemComponent::get().m_device, 1, &VKRenderingSystemComponent::get().m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(VKRenderingSystemComponent::get().m_device, 1, &VKRenderingSystemComponent::get().m_inFlightFences[m_currentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(VKRenderingSystemComponent::get().m_device, VKRenderingSystemComponent::get().m_swapChain, std::numeric_limits<uint64_t>::max(), VKRenderingSystemComponent::get().m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { VKRenderingSystemComponent::get().m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &VKRenderingSystemComponent::get().m_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { VKRenderingSystemComponent::get().m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(VKRenderingSystemComponent::get().m_graphicsQueue, 1, &submitInfo, VKRenderingSystemComponent::get().m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to submit draw command buffer!");
		return false;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { VKRenderingSystemComponent::get().m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(VKRenderingSystemComponent::get().m_presentQueue, &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
	return true;
}

bool VKRenderingSystemNS::terminate()
{
	//vkDestroyPipeline(VKRenderingSystemComponent::get().m_device, m_graphicsPipeline, nullptr);
	//vkDestroyPipelineLayout(VKRenderingSystemComponent::get().m_device, m_pipelineLayout, nullptr);
	//vkDestroyRenderPass(VKRenderingSystemComponent::get().m_device, m_renderPass, nullptr);

	for (auto imageView : VKRenderingSystemComponent::get().m_swapChainImageViews)
	{
		vkDestroyImageView(VKRenderingSystemComponent::get().m_device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(VKRenderingSystemComponent::get().m_device, VKRenderingSystemComponent::get().m_swapChain, nullptr);

	vkDestroyDevice(VKRenderingSystemComponent::get().m_device, nullptr);

	if (VKRenderingSystemComponent::get().m_enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(VKRenderingSystemComponent::get().m_instance, VKRenderingSystemComponent::get().m_messengerCallback, nullptr);
	}

	vkDestroySurfaceKHR(VKRenderingSystemComponent::get().m_instance, VKRenderingSystemComponent::get().m_windowSurface, nullptr);

	vkDestroyInstance(VKRenderingSystemComponent::get().m_instance, nullptr);

	VKRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

bool VKRenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	return VKRenderingSystemNS::setup(renderingFrontend);
}

bool VKRenderingSystem::initialize()
{
	return VKRenderingSystemNS::initialize();
}

bool VKRenderingSystem::update()
{
	return VKRenderingSystemNS::update();
}

bool VKRenderingSystem::terminate()
{
	return VKRenderingSystemNS::terminate();
}

ObjectStatus VKRenderingSystem::getStatus()
{
	return VKRenderingSystemNS::m_objectStatus;
}

bool VKRenderingSystem::resize()
{
	return true;
}

bool VKRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	return true;
}

bool VKRenderingSystem::bakeGI()
{
	return true;
}