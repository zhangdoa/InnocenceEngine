#pragma once
#include "../Common/InnoType.h"
#include "vulkan/vulkan.h"
#include "MeshDataComponent.h"

class VKMeshDataComponent : public MeshDataComponent
{
public:
	VkBuffer m_VBO;
	VkBuffer m_IBO;
};
