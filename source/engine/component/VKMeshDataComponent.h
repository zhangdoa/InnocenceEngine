#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"

class VKMeshDataComponent
{
public:
	VKMeshDataComponent() {};
	~VKMeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	VkBuffer m_VBO;
	VkBuffer m_IBO;
};
