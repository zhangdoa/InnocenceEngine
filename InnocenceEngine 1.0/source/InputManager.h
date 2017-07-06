#pragma once
#include "IEventManager.h"
#include "LogManager.h"
class InputManager : public IEventManager
{
public:
	InputManager();
	~InputManager();

	int getKey(int keyCode);
	int getMouse(int mouseButton);
	void setWindow(GLFWwindow* window);
private:
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;
	std::vector<int>m_lastKeys;
	std::vector<int>m_lastMouse;
	GLFWwindow* m_window;

	void init() override;
	void update() override;
	void shutdown() override;
};
