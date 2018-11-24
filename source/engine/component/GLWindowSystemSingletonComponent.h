#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

class GLWindowSystemSingletonComponent
{
public:
	~GLWindowSystemSingletonComponent() {};
	
	static GLWindowSystemSingletonComponent& getInstance()
	{
		static GLWindowSystemSingletonComponent instance;

		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFWwindow* m_window;

private:
	GLWindowSystemSingletonComponent() {};
};
