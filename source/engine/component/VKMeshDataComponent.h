#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "MeshDataComponent.h"

class VKMeshDataComponent : public MeshDataComponent
{
public:
	VKMeshDataComponent() {};
	~VKMeshDataComponent() {};

	VkBuffer m_VBO;
	VkBuffer m_IBO;
};
