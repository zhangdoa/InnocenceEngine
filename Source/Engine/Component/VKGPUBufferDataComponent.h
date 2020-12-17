#pragma once
#include "GPUBufferDataComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKGPUBufferDataComponent : public GPUBufferDataComponent
	{
	public:
		VkBuffer m_DeviceLocalBuffer;
		VkDeviceMemory m_DeviceLocalMemory;
		VkBuffer m_HostStagingBuffer;
		VkDeviceMemory m_HostStagingMemory;
	};
}