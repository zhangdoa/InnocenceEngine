#pragma once
#include "../common/InnoType.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include "../component/VKRenderPassComponent.h"
#include "../component/VKShaderProgramComponent.h"
#include "../component/VKMeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VKTextureDataComponent.h"

class VKRenderingBackendComponent
{
public:
	~VKRenderingBackendComponent() {};

	static VKRenderingBackendComponent& get()
	{
		static VKRenderingBackendComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	bool m_vsync_enabled = true;

	VkInstance m_instance;
	VkSurfaceKHR m_windowSurface;
	VkQueue m_presentQueue;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkCommandPool m_commandPool;

	VkSwapchainKHR m_swapChain = 0;
	VKShaderProgramComponent* m_swapChainVKSPC;
	VKRenderPassComponent* m_swapChainVKRPC;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;

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

	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceMemory m_indexBufferMemory;
	VkDeviceMemory m_textureImageMemory;

	VkBuffer m_cameraUBO;
	VkDeviceMemory m_cameraUBOMemory;

	VkBuffer m_meshUBO;
	VkDeviceMemory m_meshUBOMemory;

	VkBuffer m_materialUBO;
	VkDeviceMemory m_materialUBOMemory;

	VkBuffer m_sunUBO;
	VkDeviceMemory m_sunUBOMemory;

	VkBuffer m_pointLightUBO;
	VkDeviceMemory m_pointLightUBOMemory;

	VkBuffer m_sphereLightUBO;
	VkDeviceMemory m_sphereLightUBOMemory;

	VkSampler m_deferredRTSampler;

	RenderPassDesc m_deferredRenderPassDesc = RenderPassDesc();
private:
	VKRenderingBackendComponent() {};
};
