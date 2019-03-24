#pragma once
#include "../common/InnoType.h"
#include "../system/GLRenderingBackend/GLHeaders.h"

#include "vulkan/vulkan.h"
#define GLFW_INCLUDE_VULKAN

class VKWindowSystemComponent
{
public:
	~VKWindowSystemComponent() {};

	static VKWindowSystemComponent& get()
	{
		static VKWindowSystemComponent instance;

		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

private:
	VKWindowSystemComponent() {};
};
