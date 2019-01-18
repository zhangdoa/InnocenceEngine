#include "VKRenderingSystem.h"
#include "vulkan/vulkan.h"
#include "../component/WindowSystemComponent.h"
#include "../component/VKWindowSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	VkInstance m_instance;

#ifdef DEBUG
	const bool m_enableValidationLayers = true;
#else
	const bool m_enableValidationLayers = false;
#endif

	const std::vector<const char*> m_validationLayers =
	{
	"VK_LAYER_LUNARG_standard_validation"
	};

	VkDebugUtilsMessengerEXT m_messengerCallback;

	VkSurfaceKHR m_windowSurface;

	VkQueue m_presentQueue;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> m_graphicsFamily;
		std::optional<uint32_t> m_presentFamily;

		bool isComplete()
		{
			return m_graphicsFamily.has_value() && m_presentFamily.has_value();
		}
	};

	VkDevice m_logicalDevice;

	VkQueue m_graphicsQueue;

	const std::vector<const char*> m_deviceExtensions =
	{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkSurfaceFormatKHR> m_formats;
		std::vector<VkPresentModeKHR> m_presentModes;
	};

	VkSwapchainKHR m_swapChain = 0;
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImageView> m_swapChainImageViews;

	bool checkValidationLayerSupport()
	{
		uint32_t l_layerCount;
		vkEnumerateInstanceLayerProperties(&l_layerCount, nullptr);

		std::vector<VkLayerProperties> l_availableLayers(l_layerCount);
		vkEnumerateInstanceLayerProperties(&l_layerCount, l_availableLayers.data());

		for (const char* layerName : m_validationLayers)
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
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Validation Layer£º " + std::string(pCallbackData->pMessage));
		return VK_FALSE;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_windowSurface, &l_presentSupport);

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
			VkExtent2D l_actualExtent = { WindowSystemComponent::get().m_windowResolution.x,  WindowSystemComponent::get().m_windowResolution.y };

			l_actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, l_actualExtent.width));
			l_actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, l_actualExtent.height));

			return l_actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails l_details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_windowSurface, &l_details.m_capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			l_details.m_formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_windowSurface, &formatCount, l_details.m_formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			l_details.m_presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_windowSurface, &presentModeCount, l_details.m_presentModes.data());
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

	bool setup();

	bool createVkInstance();
	bool createDebugCallback();
	bool createWindowSurface();
	bool createPysicalDevice();
	bool createLogicalDevice();
	bool createSwapChain();
	bool createImageViews();

	bool terminate();
}

bool VKRenderingSystemNS::createVkInstance()
{
	// check support for validation layer
	if (m_enableValidationLayers && !checkValidationLayerSupport()) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo l_appInfo = {};
	l_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_appInfo.pApplicationName = WindowSystemComponent::get().m_windowName.c_str();
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
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create VkInstance!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: VkInstance has been created.");
	return true;
}

bool VKRenderingSystemNS::createDebugCallback()
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
	if (glfwCreateWindowSurface(m_instance, VKWindowSystemComponent::get().m_window, nullptr, &m_windowSurface) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create window surface!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: window surface has been created.");
	return true;
}

bool VKRenderingSystemNS::createPysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	std::vector<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, l_devices.data());

	for (const auto& device : l_devices)
	{
		if (isDeviceSuitable(device))
		{
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
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
	QueueFamilyIndices l_indices = findQueueFamilies(m_physicalDevice);

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

	if (vkCreateDevice(m_physicalDevice, &l_createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create logical device!");
		return false;
	}

	vkGetDeviceQueue(m_logicalDevice, l_indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, l_indices.m_presentFamily.value(), 0, &m_presentQueue);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: logical device has been created.");
	return true;
}

bool VKRenderingSystemNS::createSwapChain()
{
	SwapChainSupportDetails l_swapChainSupport = querySwapChainSupport(m_physicalDevice);

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
	l_createInfo.surface = m_windowSurface;

	l_createInfo.minImageCount = l_imageCount;
	l_createInfo.imageFormat = l_surfaceFormat.format;
	l_createInfo.imageColorSpace = l_surfaceFormat.colorSpace;
	l_createInfo.imageExtent = l_extent;
	l_createInfo.imageArrayLayers = 1;
	l_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices l_indices = findQueueFamilies(m_physicalDevice);
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

	if (vkCreateSwapchainKHR(m_logicalDevice, &l_createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create swap chain!");
		return false;
	}

	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &l_imageCount, nullptr);
	m_swapChainImages.resize(l_imageCount);

	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &l_imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = l_surfaceFormat.format;
	m_swapChainExtent = l_extent;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain has been created.");
	return true;
}

bool VKRenderingSystemNS::createImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo l_createInfo = {};
		l_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_createInfo.image = m_swapChainImages[i];
		l_createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_createInfo.format = m_swapChainImageFormat;
		l_createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		l_createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_createInfo.subresourceRange.baseMipLevel = 0;
		l_createInfo.subresourceRange.levelCount = 1;
		l_createInfo.subresourceRange.baseArrayLayer = 0;
		l_createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_logicalDevice, &l_createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create image views!");
			return false;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: swap chain image views has been created.");
	return true;
}

bool VKRenderingSystemNS::setup()
{
	bool result = true;
	result = result && createVkInstance();
	result = result && createDebugCallback();
	result = result && createWindowSurface();
	result = result && createPysicalDevice();
	result = result && createLogicalDevice();
	result = result && createSwapChain();
	result = result && createImageViews();

	m_objectStatus = ObjectStatus::ALIVE;
	return result;
}

bool VKRenderingSystemNS::terminate()
{
	for (auto imageView : m_swapChainImageViews) {
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);

	vkDestroyDevice(m_logicalDevice, nullptr);

	if (m_enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(m_instance, m_messengerCallback, nullptr);
	}

	vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);

	vkDestroyInstance(m_instance, nullptr);

	VKRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::setup()
{
	return VKRenderingSystemNS::setup();
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::terminate()
{
	return VKRenderingSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus VKRenderingSystem::getStatus()
{
	return VKRenderingSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::resize()
{
	return true;
}