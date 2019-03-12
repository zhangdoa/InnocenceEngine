#pragma once
#include "../common/InnoType.h"
#include "../system/GLRenderingBackend/GLHeaders.h"

class GLWindowSystemComponent
{
public:
	~GLWindowSystemComponent() {};

	static GLWindowSystemComponent& get()
	{
		static GLWindowSystemComponent instance;

		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFWwindow* m_window;

private:
	GLWindowSystemComponent() {};
};
