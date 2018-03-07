#pragma once
#include "manager/BaseManager.h"
#include "interface/ILogManager.h"
#include "GLWindowManager.h"

extern ILogManager* g_pLogManager;

class GLInputManager : public BaseManager
{
public:
	~GLInputManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static GLInputManager& getInstance()
	{
		static GLInputManager instance; 
		return instance;
	}
	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

private:
	GLInputManager() {};

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;
};
