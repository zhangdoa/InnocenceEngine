#pragma once
#include "Math.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "WindowManager.h"

class InputManager : public IEventManager
{
public:
	InputManager();
	~InputManager();

	static InputManager& getInstance()
	{
		static InputManager instance; 
		return instance;
	}

	int getKey(int keyCode);
	int getMouse(int mouseButton);
	Vec2f getMousePosition();
	void setMousePosition(const Vec2f& mousePosition);

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

private:
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;
	std::vector<int>m_lastKeys;
	std::vector<int>m_lastMouse;
	float m_mouseLastX;
	float m_mouseLastY;
	bool isMouseInputFirstTime = true;

	void init() override;
	void update() override;
	void shutdown() override;
};
