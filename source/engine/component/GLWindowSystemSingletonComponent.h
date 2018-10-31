#pragma once
#include "../common/InnoType.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLWindowSystemSingletonComponent
{
public:
	~GLWindowSystemSingletonComponent() {};
	
	static GLWindowSystemSingletonComponent& getInstance()
	{
		static GLWindowSystemSingletonComponent instance;

		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFWwindow* m_window;

private:
	GLWindowSystemSingletonComponent() {};
};
