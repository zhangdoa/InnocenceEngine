#pragma once
#include "GPUBufferComponent.h"
#include "../RenderingServer/VK/VKHeaders.h"

namespace Inno
{
	class VKGPUBufferComponent : public GPUBufferComponent
	{
	public:
		VkBuffer m_DeviceLocalBuffer;
		VkDeviceMemory m_DeviceLocalMemory;
		VkBuffer m_HostStagingBuffer;
		VkDeviceMemory m_HostStagingMemory;
	};
}