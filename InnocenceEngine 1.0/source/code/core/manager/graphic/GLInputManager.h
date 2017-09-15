#pragma once

#include "../../interface/IEventManager.h"
#include "../LogManager.h"
#include "GLWindowManager.h"

class GLInputManager : public IEventManager
{
public:
	~GLInputManager();

	static GLInputManager& getInstance()
	{
		static GLInputManager instance; 
		return instance;
	}

	void getKey(int keyCode, int& result) const;
	void getMouse(int mouseButton, int& result) const;
	void getMousePosition(glm::vec2& mousePosition) const;
	void setMousePosition(const glm::vec2& mousePosition);

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

private:
	GLInputManager();

	void initialize() override;
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
