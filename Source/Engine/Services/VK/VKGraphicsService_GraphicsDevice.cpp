#include "VKGraphicsService.h"
#include "../../Common/Array.h"
#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

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
#include "../../Services/TemplateAssetService.h"

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

	g_Engine->Get<LogService>()->Print(l_logLevel, "VKGraphicsService: Validation Layer: ", pCallbackData->pMessage);
	return VK_FALSE;
}

bool VKGraphicsService::CreateHardwareResources()
{
    bool l_result = true;

    l_result &= CreateVkInstance();
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
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

bool VKGraphicsService::ReleaseHardwareResources()
{
	return true;
}

Inno::Array<const char *> VKGraphicsService::GetRequiredExtensions()
{
#if defined INNO_PLATFORM_WIN
	Inno::Array<const char *> l_extensions = {"VK_KHR_surface", "VK_KHR_win32_surface"};
#elif defined INNO_PLATFORM_MAC
	Inno::Array<const char *> extensions = {"VK_KHR_surface", "VK_MVK_macos_surface"};
#elif defined INNO_PLATFORM_LINUX
	Inno::Array<const char *> extensions = {"VK_KHR_surface", "VK_KHR_xcb_surface"};
#endif

	if (m_enableValidationLayers)
	{
		l_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		l_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return l_extensions;
}

bool VKGraphicsService::CreateVkInstance()
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

bool VKGraphicsService::CreateDebugCallback()
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
