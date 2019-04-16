#pragma once
#include "../common/InnoType.h"
#include "../common/stl17.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include "../component/VKRenderPassComponent.h"
#include "../component/VKShaderProgramComponent.h"
#include "../component/VKMeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VKTextureDataComponent.h"

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;

	bool isComplete()
	{
		return m_graphicsFamily.has_value() && m_presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

class VKRenderingSystemComponent
{
public:
	~VKRenderingSystemComponent() {};

	static VKRenderingSystemComponent& get()
	{
		static VKRenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
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

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceMemory m_indexBufferMemory;

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();

	std::unordered_map<EntityID, VKMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, VKTextureDataComponent*> m_textureMap;

	VKMeshDataComponent* m_UnitLineVKMDC;
	VKMeshDataComponent* m_UnitQuadVKMDC;
	VKMeshDataComponent* m_UnitCubeVKMDC;
	VKMeshDataComponent* m_UnitSphereVKMDC;

	VKTextureDataComponent* m_iconTemplate_OBJ;
	VKTextureDataComponent* m_iconTemplate_PNG;
	VKTextureDataComponent* m_iconTemplate_SHADER;
	VKTextureDataComponent* m_iconTemplate_UNKNOWN;

	VKTextureDataComponent* m_iconTemplate_DirectionalLight;
	VKTextureDataComponent* m_iconTemplate_PointLight;
	VKTextureDataComponent* m_iconTemplate_SphereLight;

	VKTextureDataComponent* m_basicNormalVKTDC;
	VKTextureDataComponent* m_basicAlbedoVKTDC;
	VKTextureDataComponent* m_basicMetallicVKTDC;
	VKTextureDataComponent* m_basicRoughnessVKTDC;
	VKTextureDataComponent* m_basicAOVKTDC;

private:
	VKRenderingSystemComponent() {};
};
