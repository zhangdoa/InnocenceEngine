#pragma once
#include "GPUBufferDataComponent.h"
#include "vulkan/vulkan.h"

namespace Inno
{
	class VKGPUBufferDataComponent : public GPUBufferDataComponent
	{
	public:
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;
	};
}