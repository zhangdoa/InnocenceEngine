#pragma once

#include "IEventManager.h"
#include "LogManager.h"
#include "WindowManager.h"

class InputManager : public IEventManager
{
public:
	~InputManager();

	static InputManager& getInstance()
	{
		static InputManager instance; 
		return instance;
	}

	int getKey(int keyCode) const;
	int getMouse(int mouseButton) const;
	glm::vec2 getMousePosition() const;
	void setMousePosition(const glm::vec2& mousePosition);

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

private:
	InputManager();

	void init() override;
	void update() override;
	void shutdown() override;

	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;
	std::vector<int>m_lastKeys;
	std::vector<int>m_lastMouse;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;
};
