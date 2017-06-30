#include "Input.h"


Input::Input()
{
	init();
}


Input::~Input()
{
	shutdown();
}

void Input::init()
{
	std::vector<int>lastKeys(NUM_KEYCODES);
	std::vector<int>lastMouse(NUM_MOUSEBUTTONS);
}

void Input::update()
{
	if (window->getWindow() != NULL) {
		lastKeys.clear();
		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			lastKeys.push_back(getKey(i));
		}

		lastMouse.clear();
		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			lastMouse.push_back(getMouse(i));
		}
		if (getKey(GLFW_KEY_ESCAPE))
		{
			fprintf(stdout, "Esc pressed.\n");
			shutdown();
			window->shutdown();
		}
	}
}

void Input::shutdown()
{
	glfwSetInputMode(window->getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
}

void Input::setWindow(Window* currentWindow)
{
	window = currentWindow;
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window->getWindow(), GLFW_STICKY_KEYS, GL_TRUE);
}

int Input::getKey(int keyCode)
{
	return glfwGetKey(window->getWindow(), keyCode);
}

int Input::getMouse(int mouseButton)
{
	return glfwGetMouseButton(window->getWindow(), mouseButton);
}