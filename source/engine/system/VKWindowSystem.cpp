#include "VKWindowSystem.h"

#include "../component/WindowSystemComponent.h"
#include "../component/VKWindowSystemComponent.h"
#include "GLFWWrapper.h"

#include "InputSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	IInputSystem* m_inputSystem;

	static WindowSystemComponent* g_WindowSystemComponent;
	static VKWindowSystemComponent* g_VKWindowSystemComponent;

#ifdef DEBUG
	const bool m_enableValidationLayers = true;
#else
	const bool m_enableValidationLayers = false;
#endif

	const std::vector<const char*> m_validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
	};

	VkDebugUtilsMessengerEXT m_messengerCallback;

	bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_validationLayers) 
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
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
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Validation Layer£º " + std::string(pCallbackData->pMessage));
		return VK_FALSE;
	}

	void hideMouseCursor();
	void showMouseCursor();
}

bool VKWindowSystemNS::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	m_inputSystem = new InnoInputSystem();

	g_WindowSystemComponent = &WindowSystemComponent::get();
	g_VKWindowSystemComponent = &VKWindowSystemComponent::get();

	g_WindowSystemComponent->m_windowName = g_pCoreSystem->getGameSystem()->getGameName();

	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to initialize GLFW.");
		return false;
	}

	// bind Vulkan API hook later 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Open a window
	g_VKWindowSystemComponent->m_window = glfwCreateWindow((int)g_WindowSystemComponent->m_windowResolution.x, (int)g_WindowSystemComponent->m_windowResolution.y, g_WindowSystemComponent->m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(g_VKWindowSystemComponent->m_window);
	if (g_VKWindowSystemComponent->m_window == nullptr) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to open GLFW window.");
		glfwTerminate();
		return false;
	}

	// check support for validation layer
	if (m_enableValidationLayers && !checkValidationLayerSupport()) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: validation layers requested, but not available!");
		return false;
	}

	// set Vulkan app info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = g_WindowSystemComponent->m_windowName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Innocence Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// set Vulkan instance create info with app info
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// set window extension info
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (m_enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
	}

	// create Vulkan instance
	if (vkCreateInstance(&createInfo, nullptr, &g_VKWindowSystemComponent->m_instance) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to create VkInstance.");
		return false;
	}

	// create debug callback
	if (m_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(g_VKWindowSystemComponent->m_instance, &createInfo, nullptr, &m_messengerCallback) != VK_SUCCESS) 
		{
			m_objectStatus = ObjectStatus::STANDBY;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to create DebugUtilsMessenger.");
			return false;
		}
	}

	//setup input
	glfwSetInputMode(g_VKWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_TRUE);

	m_inputSystem->setup();

	m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	return VKWindowSystemNS::setup(hInstance, hPrevInstance, pScmdline, nCmdshow);
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::get().initialize(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, VKWindowSystemNS::m_inputSystem);

	//initialize input	
	VKWindowSystemNS::m_inputSystem->initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::update()
{
	//update window
	if (VKWindowSystemNS::g_VKWindowSystemComponent->m_window == nullptr || glfwWindowShouldClose(VKWindowSystemNS::g_VKWindowSystemComponent->m_window) != 0)
	{
		VKWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VKWindowSystem: Input error or Window closed.");
	}
	else
	{
		glfwPollEvents();

		//update input
		//keyboard
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemComponent->NUM_KEYCODES; i++)
		{
			if (glfwGetKey(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
		//mouse
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemComponent->NUM_MOUSEBUTTONS; i++)
		{
			if (glfwGetMouseButton(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
	}

	VKWindowSystemNS::m_inputSystem->update();
	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::terminate()
{
	if (VKWindowSystemNS::m_enableValidationLayers) 
	{
		VKWindowSystemNS::destroyDebugUtilsMessengerEXT(VKWindowSystemNS::g_VKWindowSystemComponent->m_instance, VKWindowSystemNS::m_messengerCallback, nullptr);
	}

	vkDestroyInstance(VKWindowSystemNS::g_VKWindowSystemComponent->m_instance, nullptr);

	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(VKWindowSystemNS::g_VKWindowSystemComponent->m_window);
	glfwTerminate();

	VKWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been terminated.");
	return true;
}

void VKWindowSystem::swapBuffer()
{
	glfwSwapBuffers(VKWindowSystemNS::g_VKWindowSystemComponent->m_window);
}

void VKWindowSystemNS::hideMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VKWindowSystemNS::showMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

INNO_SYSTEM_EXPORT ObjectStatus VKWindowSystem::getStatus()
{
	return VKWindowSystemNS::m_objectStatus;
}