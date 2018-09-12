#pragma once
#include "../component/WindowSystemSingletonComponent.h"

namespace InputSystem 
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	vec4 calcMousePositionInWorldSpace();

	void addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor);
	void addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor);
	void addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback);

	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(double mouseXPos, double mouseYPos);
	void scrollCallback(double xoffset, double yoffset);

	objectStatus m_InputSystemStatus = objectStatus::SHUTDOWN;
};
