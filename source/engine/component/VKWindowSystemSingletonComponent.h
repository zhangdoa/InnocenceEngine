#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

#include "vulkan/vulkan.h"
#define GLFW_INCLUDE_VULKAN

class VKWindowSystemSingletonComponent
{
public:
	~VKWindowSystemSingletonComponent() {};
	
	static VKWindowSystemSingletonComponent& getInstance()
	{
		static VKWindowSystemSingletonComponent instance;

		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFWwindow* m_window;

	VkInstance m_instance;

private:
	VKWindowSystemSingletonComponent() {};
};
