#pragma once
#include "../Common/Type.h"
#include "vulkan/vulkan.h"
#include "MeshComponent.h"

namespace Inno
{
	class VKMeshComponent : public MeshComponent
	{
	public:
		VkBuffer m_VBO;
		VkBuffer m_IBO;
		VkDeviceMemory m_VBMemory;
		VkDeviceMemory m_IBMemory;
	};
}
