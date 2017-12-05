#pragma once

#include "../../interface/IEventManager.h"
#include "../LogManager.h"
#include "GLWindowManager.h"

class GLInputManager : public IEventManager
{
public:
	~GLInputManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	static GLInputManager& getInstance()
	{
		static GLInputManager instance; 
		return instance;
	}

	void setKeyboardInputCallback(std::multimap<int, std::function<void()>>& keyboardInputCallback);
	void setMouseMovementCallback(std::multimap<int, std::function<void(float)>>& mouseMovementCallback);

private:
	GLInputManager();

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	std::multimap<int, std::function<void()>> m_keyboardInputCallback;
	std::multimap<int, std::function<void(float)>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;
};
