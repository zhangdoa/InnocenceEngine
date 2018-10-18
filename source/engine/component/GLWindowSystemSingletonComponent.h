#pragma once
#include "BaseComponent.h"

#include "../system/HighLevelSystem/GLHeaders.h"

class GLWindowSystemSingletonComponent : public BaseComponent
{
public:
	~GLWindowSystemSingletonComponent() {};
	
	static GLWindowSystemSingletonComponent& getInstance()
	{
		static GLWindowSystemSingletonComponent instance;

		return instance;
	}

	GLFWwindow* m_window;

private:
	GLWindowSystemSingletonComponent() {};
};
