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

bool VKGraphicsService::CreatePhysicalDevice()
{
	// check if there is any suitable physical GPU
	uint32_t l_deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, nullptr);

	if (l_deviceCount == 0)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to find GPUs with Vulkan support!");
		return false;
	}

	// assign the handle
	Inno::Array<VkPhysicalDevice> l_devices(l_deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &l_deviceCount, l_devices.data());

	for (const auto &l_device : l_devices)
	{
		if (IsDeviceSuitable(l_device, m_windowSurface, m_deviceExtensions))
		{
			m_physicalDevice = l_device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to find a suitable GPU!");
		return false;
	}

	Log(Success, "VkPhysicalDevice has been created.");
	return true;
}

bool VKGraphicsService::CreateLogicalDevice()
{
	QueueFamilyIndices l_indices = FindQueueFamilies(m_physicalDevice, m_windowSurface);

	Inno::Array<VkDeviceQueueCreateInfo> l_queueCreateInfos;
	std::set<uint32_t> l_uniqueQueueFamilies = {l_indices.m_graphicsFamily.value(), l_indices.m_presentFamily.value()};

	float l_queuePriority = 1.0f;
	for (uint32_t l_queueFamily : l_uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo l_queueCreateInfo = {};
		l_queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		l_queueCreateInfo.queueFamilyIndex = l_queueFamily;
		l_queueCreateInfo.queueCount = 1;
		l_queueCreateInfo.pQueuePriorities = &l_queuePriority;
		l_queueCreateInfos.push_back(l_queueCreateInfo);
	}

	VkPhysicalDeviceFeatures l_deviceFeatures = {};
	l_deviceFeatures.depthBiasClamp = VK_TRUE;
	l_deviceFeatures.depthClamp = VK_TRUE;
	l_deviceFeatures.dualSrcBlend = VK_TRUE;
	l_deviceFeatures.fillModeNonSolid = VK_TRUE;
	l_deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
	l_deviceFeatures.geometryShader = VK_TRUE;
	l_deviceFeatures.samplerAnisotropy = VK_TRUE;
	l_deviceFeatures.tessellationShader = VK_TRUE;
	l_deviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;

	VkDeviceCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	l_createInfo.queueCreateInfoCount = static_cast<uint32_t>(l_queueCreateInfos.size());
	l_createInfo.pQueueCreateInfos = l_queueCreateInfos.data();

	l_createInfo.pEnabledFeatures = &l_deviceFeatures;
	VkPhysicalDeviceImagelessFramebufferFeatures l_imagelessFramebufferFeatures = {};
	l_imagelessFramebufferFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
	l_imagelessFramebufferFeatures.imagelessFramebuffer = true;
	l_createInfo.pNext = &l_imagelessFramebufferFeatures;

	VkPhysicalDeviceCustomBorderColorFeaturesEXT l_CustomBorderColorFeaturesEXT = {};
	l_CustomBorderColorFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
	l_CustomBorderColorFeaturesEXT.customBorderColors = true;
	l_CustomBorderColorFeaturesEXT.customBorderColorWithoutFormat = true;
	l_imagelessFramebufferFeatures.pNext = &l_CustomBorderColorFeaturesEXT;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR l_TimelineSemaphoreFeaturesKHR = {};
	l_TimelineSemaphoreFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
	l_TimelineSemaphoreFeaturesKHR.timelineSemaphore = true;
	l_CustomBorderColorFeaturesEXT.pNext = &l_TimelineSemaphoreFeaturesKHR;

	VkPhysicalDeviceSynchronization2Features l_Synchronization2Features = {};
	l_Synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	l_Synchronization2Features.synchronization2 = true;
	l_TimelineSemaphoreFeaturesKHR.pNext = &l_Synchronization2Features;

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
		Log(Error, "Failed to create VkDevice!");
		return false;
	}

	vkGetDeviceQueue(m_device, l_indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, l_indices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, l_indices.m_computeFamily.value(), 0, &m_computeQueue);

	Log(Success, "VkDevice has been created.");
	return true;
}

bool VKGraphicsService::CreateTextureSamplers()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	// if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_deferredRTSampler) != VK_SUCCESS)
	//{
	//	m_ObjectStatus = ObjectStatus::Suspended;
	//	Log(Error, "Failed to create VkSampler for deferred pass render target sampling!");
	//	return false;
	// }

	// Log(Success, "VkSampler for deferred pass render target sampling has been created.");
	return true;
}

bool VKGraphicsService::CreateVertexInputAttributions()
{
	m_vertexBindingDescription = {};
	m_vertexBindingDescription.binding = 0;
	m_vertexBindingDescription.stride = sizeof(Vertex);
	m_vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttributeDescriptions = {};

	m_vertexAttributeDescriptions[0].binding = 0;
	m_vertexAttributeDescriptions[0].location = 0;
	m_vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[0].offset = offsetof(Vertex, m_pos);

	m_vertexAttributeDescriptions[1].binding = 0;
	m_vertexAttributeDescriptions[1].location = 1;
	m_vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[1].offset = offsetof(Vertex, m_normal);

	m_vertexAttributeDescriptions[2].binding = 0;
	m_vertexAttributeDescriptions[2].location = 2;
	m_vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttributeDescriptions[2].offset = offsetof(Vertex, m_tangent);

	m_vertexAttributeDescriptions[3].binding = 0;
	m_vertexAttributeDescriptions[3].location = 3;
	m_vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttributeDescriptions[3].offset = offsetof(Vertex, m_texCoord);

	m_vertexAttributeDescriptions[4].binding = 0;
	m_vertexAttributeDescriptions[4].location = 4;
	m_vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttributeDescriptions[4].offset = offsetof(Vertex, m_pad1);

	m_vertexAttributeDescriptions[5].binding = 0;
	m_vertexAttributeDescriptions[5].location = 5;
	m_vertexAttributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
	m_vertexAttributeDescriptions[5].offset = offsetof(Vertex, m_pad2);

	return true;
}

bool VKGraphicsService::CreateMaterialDescriptorPool()
{
	auto l_renderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	VkDescriptorPoolSize l_descriptorPoolSize = {};
	l_descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	l_descriptorPoolSize.descriptorCount = l_renderingCapability.maxMaterials * 8;

	VkDescriptorPoolSize l_descriptorPoolSizes[] = {l_descriptorPoolSize};

	if (!CreateDescriptorPool(l_descriptorPoolSizes, 1, l_renderingCapability.maxMaterials, m_materialDescriptorPool))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkDescriptorPool for material!");
		return false;
	}

	Log(Success, "VkDescriptorPool for material has been created.");

	Inno::Array<VkDescriptorSetLayoutBinding> l_textureLayoutBindings(8);
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

	if (!CreateDescriptorSetLayout(&l_textureLayoutBindings[0], (uint32_t)l_textureLayoutBindings.size(), m_materialDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create VkDescriptorSetLayout for material!");
		return false;
	}

	Log(Success, "VkDescriptorSetLayout for material has been created.");

	if (!CreateDescriptorSetLayout(nullptr, 0, m_dummyEmptyDescriptorLayout))
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create DummyEmptyDescriptorLayout!");
		return false;
	}

	return true;
}

bool VKGraphicsService::CreateGlobalCommandPool()
{
	return CreateCommandPool(m_windowSurface, GPUEngineType::Graphics, m_globalCommandPool);
}
