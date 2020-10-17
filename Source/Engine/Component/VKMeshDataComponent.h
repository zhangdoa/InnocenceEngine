#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "MeshDataComponent.h"

namespace Inno
{
	class VKMeshDataComponent : public MeshDataComponent
	{
	public:
		VkBuffer m_VBO;
		VkBuffer m_IBO;
		VkDeviceMemory m_VBMemory;
		VkDeviceMemory m_IBMemory;
	};
}
