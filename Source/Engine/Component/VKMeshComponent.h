#pragma once
#include "MeshComponent.h"
#include "../RenderingServer/VK/VKHeaders.h"

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
